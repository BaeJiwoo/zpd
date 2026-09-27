#ifndef ZPD_PACKETHANDLER_HPP
#define ZPD_PACKETHANDLER_HPP

#include "Packet.hpp"
#include "ConnectionKey.hpp"

#include "ServerLimits.hpp"
#include "ServerEvent.hpp"

#include <functional>
#include <mutex>
#include <map>
#include <thread>
#include <queue>

#include <utility>
#include <condition_variable>
#include <set>
#include <deque>
#include <google/protobuf/message_lite.h>

class PacketHandler
{
  public:
    ~PacketHandler()
    {
        Stop();
    }
    using SendCallback = std::function<bool(ConnectionKey, const char*, std::uint32_t)>;
    using DisconnectCallback = std::function<void(ConnectionKey)>;

    bool Start(SendCallback sendPacket, DisconnectCallback disconnect);
    void Stop();

    bool ReceiveBytes(ConnectionKey connection, const char* data, std::size_t size);

    bool EnqueueConnected(ConnectionKey connection);

    void EnqueueDisconnected(ConnectionKey connection);

  private:
    void LogicWorker();
    void HandleConnected(ConnectionKey connection);
    void HandlePacketReceived(ConnectionKey connection, const Packet& packet);
    void HandleDisconnected(ConnectionKey connection);
    void DisconnectFailedConnection(ConnectionKey connection);
    void HandleMatchRequest(ConnectionKey connection, const Packet& packet);
    void HandleCancelMatch(ConnectionKey connection, const Packet& packet);
    void HandleLeaveSession(ConnectionKey connection, const Packet& packet);
    void TryMatchPlayers();
    void RemoveFromSession(ConnectionKey connection);
    void RemoveFromMatchQueue(ConnectionKey connection);
    bool SendMessage(ConnectionKey connection, MessageCode code, std::uint32_t requestId,
                     const google::protobuf::MessageLite& message);
    void SendError(ConnectionKey connection, const Packet& request, ErrorCode error);

    struct Player
    {
        std::uint64_t playerId = 0;
        std::uint64_t sessionId = 0;
        bool queued = false;
    };

    struct Session
    {
        std::set<ConnectionKey> members;
    };

    std::map<ConnectionKey, Player> m_players;
    std::map<std::uint64_t, Session> m_sessions;
    std::deque<ConnectionKey> m_matchQueue;
    std::uint64_t m_nextPlayerId = 1;
    std::uint64_t m_nextSessionId = 1;

    SendCallback m_sendPacket;
    DisconnectCallback m_disconnect;

    std::mutex m_mutex;
    std::map<ConnectionKey, std::vector<char>> m_pendingBytesByConnection;
    bool m_logicRunning = false;
    std::queue<ServerEvent> m_eventQueue;
    std::thread m_logicThread;
    std::condition_variable m_queueReady;

    std::set<ConnectionKey> m_liveConnections;
};

#endif // ZPD_PACKETHANDLER_HPP
