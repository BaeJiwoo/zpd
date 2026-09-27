#ifndef ZPD_PACKETCLIENT_HPP
#define ZPD_PACKETCLIENT_HPP
#include "Packet.hpp"
#include "PendingRequest.hpp"
#include <WinSock2.h>
#include <atomic>
#include <map>
#include <mutex>
#include <set>
#include <string>
#include <thread>

class PacketClient
{
  public:
    explicit PacketClient(SOCKET peer);
    ~PacketClient();

    void Start();
    bool IsRunning() const;
    bool HasFailed() const;
    void Stop();

    bool HandleInputLine(const std::string& line);

  private:
    void PrintStatus(const std::string& text);
    void CloseWithError(const std::string& text);
    void ShowStatus();
    bool SendRequest(Packet packet, PendingRequest pending);
    bool ReceiveExact(char* bytes, std::size_t size);
    template <typename Message> Message ParseMessage(const std::vector<char>& payload);
    void HandleServerPacket(const PacketHeader& header, const std::vector<char>& payload);
    void ReceivePackets();
    SOCKET m_socket;
    std::atomic<bool> m_running{true};
    std::atomic<bool> m_failed{false};
    std::thread m_receiver;
    std::mutex m_mutex;
    std::uint32_t m_nextRequestId = 1;
    std::map<std::uint32_t, PendingRequest> m_pending;
    std::uint64_t m_playerId = 0;
    std::uint64_t m_sessionId = 0;
    bool m_queued = false;
    std::set<std::uint64_t> m_members;
};
#endif // ZPD_PACKETCLIENT_HPP
