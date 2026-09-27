#include "ZPDServer.hpp"

#include <future>
#include <iostream>
#include <stdexcept>
#include <vector>

void Check(bool condition, const char* message)
{
    if (!condition)
        throw std::runtime_error(message);
}

void TestPacketHandler()
{
    PacketHandler handler;
    const auto sendPacket = [](ConnectionKey, const char*, std::uint32_t) { return true; };
    const auto disconnect = [](ConnectionKey) {};
    const ConnectionKey first{0, 1};
    Packet packet;
    packet.requestId = 1;
    packet.payload = {'a', 'b', 'c', 'd'};
    const auto bytes = packet.Serialize();

    Check(!handler.EnqueueConnected(first), "connection accepted before start");
    Check(handler.Start(sendPacket, disconnect), "handler start failed");
    Check(!handler.Start(sendPacket, disconnect), "duplicate start accepted");
    Check(!handler.ReceiveBytes(first, bytes.data(), bytes.size()), "unknown connection accepted");
    Check(handler.EnqueueConnected(first), "connection enqueue failed");
    Check(handler.ReceiveBytes(first, bytes.data(), 3), "fragmented header rejected");
    Check(handler.ReceiveBytes(first, bytes.data() + 3, 6), "partial payload rejected");
    Check(handler.ReceiveBytes(first, bytes.data() + 9, bytes.size() - 9), "payload tail rejected");

    auto combined = bytes;
    combined.insert(combined.end(), bytes.begin(), bytes.end());
    combined.insert(combined.end(), bytes.begin(), bytes.begin() + 2);
    Check(handler.ReceiveBytes(first, combined.data(), combined.size()), "coalesced packets rejected");
    Check(handler.ReceiveBytes(first, bytes.data() + 2, bytes.size() - 2), "remaining packet rejected");
    Check(!handler.ReceiveBytes(first, nullptr, 1), "null input accepted");
    Check(!handler.ReceiveBytes(first, bytes.data(), ProtocolLimits::MaxPacketBytes + 1),
          "oversized input accepted");

    // A disconnect must discard an incomplete packet before the slot is reused.
    Check(handler.ReceiveBytes(first, bytes.data(), 3), "partial packet rejected");
    handler.EnqueueDisconnected(first);
    Check(!handler.ReceiveBytes(first, bytes.data(), bytes.size()), "disconnected key accepted");
    const ConnectionKey next{0, 2};
    Check(handler.EnqueueConnected(next), "reconnected slot rejected");
    Check(handler.ReceiveBytes(next, bytes.data(), bytes.size()), "reconnected packet rejected");
    char invalid[ProtocolLimits::HeaderSize]{};
    invalid[1] = 7;
    Check(!handler.ReceiveBytes(next, invalid, sizeof(invalid)), "undersized frame accepted");
    invalid[0] = 0x10;
    invalid[1] = 1;
    Check(!handler.ReceiveBytes(next, invalid, sizeof(invalid)), "oversized frame accepted");
    handler.Stop();
    handler.Stop();
    Check(handler.Start(sendPacket, disconnect), "handler restart failed");
    Check(!handler.ReceiveBytes(next, bytes.data(), bytes.size()), "restart retained connection");
    Check(handler.EnqueueConnected(next), "connection after restart rejected");
    Check(handler.ReceiveBytes(next, bytes.data(), bytes.size()), "packet after restart rejected");
}

struct Peer
{
    SOCKET socket = INVALID_SOCKET;
    ~Peer() { if (socket != INVALID_SOCKET) closesocket(socket); }
};

void TestBasicServer()
{
    ZPDServer server;
    Check(server.Start(0, 4, 2), "server start failed");
    Peer peer;
    peer.socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    Check(peer.socket != INVALID_SOCKET, "socket creation failed");
    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    address.sin_port = htons(server.Port());
    Check(connect(peer.socket, reinterpret_cast<sockaddr*>(&address), sizeof(address)) == 0,
          "connect failed");
    DWORD timeout = 3000;
    Check(setsockopt(peer.socket, SOL_SOCKET, SO_RCVTIMEO,
                    reinterpret_cast<const char*>(&timeout), sizeof(timeout)) == 0,
          "receive timeout failed");

    const auto sendBytes = [&](const char* data, std::size_t size) {
        for (std::size_t offset = 0; offset < size;) {
            const int sent = send(peer.socket, data + offset, static_cast<int>(size - offset), 0);
            Check(sent > 0, "send failed");
            offset += sent;
        }
    };
    const auto expectEcho = [&](const std::vector<char>& expected) {
        std::vector<char> actual(expected.size());
        for (std::size_t offset = 0; offset < actual.size();) {
            const int count = recv(peer.socket, actual.data() + offset,
                                   static_cast<int>(actual.size() - offset), 0);
            Check(count > 0, "echo receive failed");
            offset += count;
        }
        Check(actual == expected, "echo changed header, payload or packet order");
    };

    Packet packet;
    packet.code = MessageCode::EchoRequest;
    packet.requestId = 123;
    packet.payload = {'a', '\0', static_cast<char>(0xff), 'b'};
    const auto bytes = packet.Serialize();
    sendBytes(bytes.data(), 3);
    fd_set readers;
    FD_ZERO(&readers);
    FD_SET(peer.socket, &readers);
    timeval wait{0, 100000};
    Check(select(0, &readers, nullptr, nullptr, &wait) == 0,
          "incomplete header was echoed");
    sendBytes(bytes.data() + 3, bytes.size() - 3);
    expectEcho(bytes);

    Packet empty;
    empty.code = MessageCode::PingRequest;
    empty.requestId = 456;
    auto combined = bytes;
    const auto emptyBytes = empty.Serialize();
    combined.insert(combined.end(), emptyBytes.begin(), emptyBytes.end());
    sendBytes(combined.data(), combined.size());
    expectEcho(combined);

    packet.payload.resize(ProtocolLimits::MaxPayloadBytes, 'x');
    const auto maximum = packet.Serialize();
    sendBytes(maximum.data(), maximum.size());
    expectEcho(maximum);

    char invalid[ProtocolLimits::HeaderSize]{};
    Check(send(peer.socket, invalid, sizeof(invalid), 0) == sizeof(invalid), "invalid send failed");
    char received = 0;
    const int result = recv(peer.socket, &received, 1, 0);
    const int error = result == SOCKET_ERROR ? WSAGetLastError() : 0;
    Check(result == 0 || (result == SOCKET_ERROR && error == WSAECONNRESET),
          "invalid frame did not disconnect the peer");
    server.Stop();
}

void TestSendFailure(bool throwOnSend)
{
    // Verify failed service work does not terminate the worker or block later connections.
    std::promise<void> disconnected;
    std::promise<void> echoed;
    auto disconnectedResult = disconnected.get_future();
    auto echoResult = echoed.get_future();
    PacketHandler handler;
    Check(handler.Start(
        [&](ConnectionKey key, const char*, std::uint32_t) {
            if (key.slotIndex == 0) {
                if (throwOnSend)
                    throw std::runtime_error("test send exception");
                return false;
            }
            echoed.set_value();
            return true;
        },
        [&](ConnectionKey key) {
            Check(key.slotIndex == 0, "wrong connection disconnected");
            disconnected.set_value();
        }), "failure handler start failed");
    Packet packet;
    packet.requestId = 1;
    const auto bytes = packet.Serialize();
    Check(handler.EnqueueConnected({0, 1}), "first connection failed");
    Check(handler.ReceiveBytes({0, 1}, bytes.data(), bytes.size()), "first packet failed");
    Check(disconnectedResult.wait_for(std::chrono::seconds(3)) == std::future_status::ready,
          "send failure did not disconnect");
    Check(handler.EnqueueConnected({1, 1}), "second connection failed");
    Check(handler.ReceiveBytes({1, 1}, bytes.data(), bytes.size()), "second packet failed");
    Check(echoResult.wait_for(std::chrono::seconds(3)) == std::future_status::ready,
          "worker stopped after send failure");
    Check(!handler.ReceiveBytes({0, 1}, bytes.data(), bytes.size()),
          "failed connection remained active");
}

int main()
{
    try {
        TestPacketHandler();
        TestBasicServer();
        TestSendFailure(false);
        TestSendFailure(true);
        std::cout << "Packet handler and basic server tests passed.\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
