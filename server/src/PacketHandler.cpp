#include "PacketHandler.hpp"

#include <iostream>
#include <stdexcept>
#include <syncstream>

bool PacketHandler::Start(SendCallback sendPacket, DisconnectCallback disconnect)
{
    std::lock_guard lock(m_mutex);
    if (m_logicRunning || !sendPacket || !disconnect)
        return false;

    m_sendPacket = std::move(sendPacket);
    m_disconnect = std::move(disconnect);
    m_logicRunning = true;
    try {
        m_logicThread = std::thread(&PacketHandler::LogicWorker, this);
    } catch (...) {
        m_logicRunning = false;
        m_sendPacket = {};
        m_disconnect = {};
        return false;
    }
    return true;
}

void PacketHandler::Stop()
{
    {
        std::lock_guard lock(m_mutex);
        if (!m_logicRunning)
            return;
        m_logicRunning = false;
    }
    m_queueReady.notify_all();
    if (m_logicThread.joinable())
        m_logicThread.join();

    std::lock_guard lock(m_mutex);
    m_eventQueue = {};
    m_pendingBytesByConnection.clear();
    m_liveConnections.clear();
    m_players.clear();
    m_sessions.clear();
    m_matchQueue.clear();
    m_nextPlayerId = 1;
    m_nextSessionId = 1;
    m_sendPacket = {};
    m_disconnect = {};
}

void PacketHandler::LogicWorker()
{
    while (true) {
        ServerEvent event;
        {
            std::unique_lock lock(m_mutex);
            m_queueReady.wait(lock, [this] { return !m_logicRunning || !m_eventQueue.empty(); });
            if (!m_logicRunning)
                return;

            event = std::move(m_eventQueue.front());
            m_eventQueue.pop();
            if (event.type == ServerEventType::PacketReceived &&
                !m_liveConnections.contains(event.connection))
                continue;
        }

        try {
            switch (event.type) {
            case ServerEventType::Connected:
                HandleConnected(event.connection);
                break;
            case ServerEventType::PacketReceived:
                HandlePacketReceived(event.connection, event.packet);
                break;
            case ServerEventType::Disconnected:
                HandleDisconnected(event.connection);
                break;
            }
        } catch (const std::exception& error) {
            std::osyncstream(std::cerr) << "[logic failed] client=" << event.connection.slotIndex
                                        << " reason=" << error.what() << '\n';
            DisconnectFailedConnection(event.connection);
        } catch (...) {
            std::osyncstream(std::cerr) << "[logic failed] client=" << event.connection.slotIndex
                                        << " reason=unknown error\n";
            DisconnectFailedConnection(event.connection);
        }
    }
}

void PacketHandler::HandleConnected(ConnectionKey connection)
{
    if (m_nextPlayerId == 0)
        throw std::runtime_error("player ID exhausted");
    m_players.emplace(connection, Player{m_nextPlayerId++, 0, false});
    std::osyncstream(std::cout) << "[logic connected] client=" << connection.slotIndex
                                << " generation=" << connection.generation << '\n';
}

void PacketHandler::HandlePacketReceived(ConnectionKey connection, const Packet& packet)
{
    const bool matchRequest = packet.code == MessageCode::MatchRequest ||
                              packet.code == MessageCode::CancelMatchRequest ||
                              packet.code == MessageCode::LeaveSessionRequest;
    if (matchRequest) {
        if (packet.requestId == 0 || packet.error != ErrorCode::None) {
            SendError(connection, packet, ErrorCode::InvalidRequestStatus);
            return;
        }
        if (!m_players.contains(connection))
            throw std::runtime_error("player not found");
        switch (packet.code) {
        case MessageCode::MatchRequest:
            HandleMatchRequest(connection, packet);
            break;
        case MessageCode::CancelMatchRequest:
            HandleCancelMatch(connection, packet);
            break;
        case MessageCode::LeaveSessionRequest:
            HandleLeaveSession(connection, packet);
            break;
        default:
            break;
        }
        return;
    }
    std::osyncstream(std::cout) << "[logic echo] client=" << connection.slotIndex
                                << " code=" << static_cast<unsigned>(packet.code)
                                << " requestId=" << packet.requestId
                                << " payloadBytes=" << packet.payload.size() << '\n';
    const auto bytes = packet.Serialize();
    if (!m_sendPacket(connection, bytes.data(), static_cast<std::uint32_t>(bytes.size())))
        throw std::runtime_error("echo send failed");
}

void PacketHandler::HandleDisconnected(ConnectionKey connection)
{
    RemoveFromMatchQueue(connection);
    RemoveFromSession(connection);
    m_players.erase(connection);
    std::osyncstream(std::cout) << "[logic disconnected] client=" << connection.slotIndex
                                << " generation=" << connection.generation << '\n';
}

void PacketHandler::DisconnectFailedConnection(ConnectionKey connection)
{
    try {
        m_disconnect(connection);
    } catch (...) {
    }
    EnqueueDisconnected(connection);
}

bool PacketHandler::EnqueueConnected(ConnectionKey connection)
{
    std::lock_guard lock(m_mutex);

    if (!m_logicRunning) {
        return false;
    }

    if (m_liveConnections.contains(connection)) {
        return true;
    }

    const auto used = m_eventQueue.size() + m_liveConnections.size();

    if (used + 2 > ServerLimits::MaxQueuedEvents) {
        return false;
    }

    m_liveConnections.insert(connection);

    try {
        m_eventQueue.push({ServerEventType::Connected, connection, Packet{}});
    } catch (...) {
        m_liveConnections.erase(connection);
        throw;
    }

    m_queueReady.notify_one();
    return true;
}

void PacketHandler::EnqueueDisconnected(ConnectionKey connection)
{
    {
        std::lock_guard lock(m_mutex);

        m_pendingBytesByConnection.erase(connection);

        if (!m_liveConnections.contains(connection)) {
            return;
        }

        if (!m_logicRunning) {
            m_liveConnections.erase(connection);
            return;
        }

        m_eventQueue.push({ServerEventType::Disconnected, connection, Packet{}});

        m_liveConnections.erase(connection);
    }

    m_queueReady.notify_one();
    return;
}

bool PacketHandler::ReceiveBytes(ConnectionKey connection, const char* data, std::size_t size)
{
    if (size == 0)
        return true;
    if (data == nullptr || size > ProtocolLimits::MaxPacketBytes)
        return false;

    {
        std::lock_guard lock(m_mutex);

        if (!m_logicRunning || !m_liveConnections.contains(connection))
            return false;

        auto& pending = m_pendingBytesByConnection[connection];
        pending.insert(pending.end(), data, data + size);

        std::size_t consumed = 0;
        while (pending.size() - consumed >= ProtocolLimits::HeaderSize) {
            const auto header = PacketHeader::Read(pending.data() + consumed);
            if (header.packetSize < ProtocolLimits::HeaderSize ||
                header.packetSize > ProtocolLimits::MaxPacketBytes) {
                m_pendingBytesByConnection.erase(connection);
                return false;
            }
            if (pending.size() - consumed < header.packetSize)
                break;

            const char* payload = pending.data() + consumed + ProtocolLimits::HeaderSize;

            Packet request;
            request.code = header.code;
            request.error = header.error;
            request.requestId = header.requestId;
            request.payload.assign(payload,
                                   payload + (header.packetSize - ProtocolLimits::HeaderSize));

            if (m_eventQueue.size() + m_liveConnections.size() >= ServerLimits::MaxQueuedEvents) {
                m_pendingBytesByConnection.erase(connection);
                return false;
            }

            m_eventQueue.push({ServerEventType::PacketReceived, connection, std::move(request)});

            m_queueReady.notify_one();

            consumed += header.packetSize;
        }

        pending.erase(pending.begin(),
                      pending.begin() + static_cast<std::vector<char>::difference_type>(consumed));
        if (pending.empty())
            m_pendingBytesByConnection.erase(connection);
    }

    return true;
}
