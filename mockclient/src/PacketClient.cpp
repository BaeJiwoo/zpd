#include "PacketClient.hpp"
#include <iostream>
#include <utility>

PacketClient::PacketClient(SOCKET peer) : m_socket(peer)
{
}

PacketClient::~PacketClient()
{
    Stop();
}

void PacketClient::Start()
{
    m_receiver = std::thread([this] { ReceivePackets(); });
}

bool PacketClient::IsRunning() const
{
    return m_running;
}

bool PacketClient::HasFailed() const
{
    return m_failed;
}

void PacketClient::Stop()
{
    m_running = false;
    shutdown(m_socket, SD_BOTH);
    if (m_receiver.joinable())
        m_receiver.join();
}

void PacketClient::PrintStatus(const std::string& text)
{
    std::lock_guard lock(m_mutex);
    std::cout << text << std::endl;
}

void PacketClient::CloseWithError(const std::string& text)
{
    m_failed = true;
    m_running = false;
    shutdown(m_socket, SD_BOTH);
    PrintStatus(text);
}

void PacketClient::ShowStatus()
{
    std::cout << "Player: " << m_playerId << ", state: "
              << (m_sessionId ? "in session" : m_queued ? "queued" : "idle")
              << ", session: " << m_sessionId << ", members:";
    for (auto id : m_members)
        std::cout << ' ' << id;
    std::cout << std::endl;
}

bool PacketClient::SendRequest(Packet packet, PendingRequest pending)
{
    {
        std::lock_guard lock(m_mutex);
        if (!m_running)
            return false;
        do {
            packet.requestId = m_nextRequestId++;
        } while (packet.requestId == 0 || m_pending.contains(packet.requestId));
        m_pending.emplace(packet.requestId, std::move(pending));
    }
    const auto bytes = packet.Serialize();
    std::size_t sent = 0;
    while (sent < bytes.size()) {
        const int count =
            send(m_socket, bytes.data() + sent, static_cast<int>(bytes.size() - sent), 0);
        if (count <= 0) {
            CloseWithError("Send failed. Connection closed.");
            return false;
        }
        sent += static_cast<std::size_t>(count);
    }
    return true;
}

bool PacketClient::ReceiveExact(char* bytes, std::size_t size)
{
    std::size_t offset = 0;
    while (offset < size) {
        const int count = recv(m_socket, bytes + offset, static_cast<int>(size - offset), 0);
        if (count <= 0)
            return false;
        offset += static_cast<std::size_t>(count);
    }
    return true;
}
