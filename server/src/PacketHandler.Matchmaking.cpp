#include "PacketHandler.hpp"
#include "ProtobufCodec.hpp"
#include "proto/matchmaking.pb.h"

#include <algorithm>
#include <stdexcept>

bool PacketHandler::SendMessage(ConnectionKey connection, MessageCode code, std::uint32_t requestId,
                                const google::protobuf::MessageLite& message)
{
    Packet response;
    response.code = code;
    response.requestId = requestId;
    ProtobufCodec::SerializePayload(response, message);
    const auto bytes = response.Serialize();
    if (m_sendPacket(connection, bytes.data(), static_cast<std::uint32_t>(bytes.size())))
        return true;
    DisconnectFailedConnection(connection);
    return false;
}

void PacketHandler::SendError(ConnectionKey connection, const Packet& request, ErrorCode error)
{
    Packet response;
    response.code = ResponseCodeFor(request.code);
    response.requestId = request.requestId;
    response.error = error;
    const auto bytes = response.Serialize();
    if (!m_sendPacket(connection, bytes.data(), static_cast<std::uint32_t>(bytes.size())))
        DisconnectFailedConnection(connection);
}

void PacketHandler::HandleMatchRequest(ConnectionKey connection, const Packet& packet)
{
    protocol::MatchRequest request;
    if (!ProtobufCodec::ParsePayload(packet, request)) {
        SendError(connection, packet, ErrorCode::InvalidPayload);
        return;
    }
    auto& player = m_players.at(connection);
    if (player.queued || player.sessionId != 0 || m_nextSessionId == 0) {
        SendError(connection, packet, ErrorCode::InvalidState);
        return;
    }
    m_matchQueue.push_back(connection);
    player.queued = true;
    protocol::MatchResponse response;
    response.set_player_id(player.playerId);
    if (!SendMessage(connection, MessageCode::MatchResponse, packet.requestId, response)) {
        RemoveFromMatchQueue(connection);
        return;
    }
    TryMatchPlayers();
}

void PacketHandler::HandleCancelMatch(ConnectionKey connection, const Packet& packet)
{
    protocol::CancelMatchRequest request;
    if (!ProtobufCodec::ParsePayload(packet, request)) {
        SendError(connection, packet, ErrorCode::InvalidPayload);
        return;
    }
    if (!m_players.at(connection).queued) {
        SendError(connection, packet, ErrorCode::InvalidState);
        return;
    }
    RemoveFromMatchQueue(connection);
    SendMessage(connection, MessageCode::CancelMatchResponse, packet.requestId,
                protocol::CancelMatchResponse{});
}

void PacketHandler::HandleLeaveSession(ConnectionKey connection, const Packet& packet)
{
    protocol::LeaveSessionRequest request;
    if (!ProtobufCodec::ParsePayload(packet, request)) {
        SendError(connection, packet, ErrorCode::InvalidPayload);
        return;
    }
    const auto sessionId = m_players.at(connection).sessionId;
    if (sessionId == 0) {
        SendError(connection, packet, ErrorCode::InvalidState);
        return;
    }
    RemoveFromSession(connection);
    protocol::LeaveSessionResponse response;
    response.set_session_id(sessionId);
    SendMessage(connection, MessageCode::LeaveSessionResponse, packet.requestId, response);
}

void PacketHandler::TryMatchPlayers()
{
    {
        std::lock_guard lock(m_mutex);
        std::erase_if(m_matchQueue, [this](ConnectionKey connection) {
            if (m_liveConnections.contains(connection))
                return false;
            const auto player = m_players.find(connection);
            if (player != m_players.end())
                player->second.queued = false;
            return true;
        });
    }
    while (m_matchQueue.size() >= ServerLimits::PlayersPerSession && m_nextSessionId != 0) {
        const auto sessionId = m_nextSessionId++;
        Session session;
        protocol::MatchFound notification;
        notification.set_session_id(sessionId);
        for (std::size_t index = 0; index < ServerLimits::PlayersPerSession; ++index) {
            const auto connection = m_matchQueue.front();
            m_matchQueue.pop_front();
            auto& player = m_players.at(connection);
            player.queued = false;
            player.sessionId = sessionId;
            session.members.insert(connection);
            notification.add_player_ids(player.playerId);
        }
        const auto [position, inserted] = m_sessions.emplace(sessionId, std::move(session));
        for (const auto member : position->second.members)
            SendMessage(member, MessageCode::MatchFound, 0, notification);
    }
}

void PacketHandler::RemoveFromMatchQueue(ConnectionKey connection)
{
    std::erase(m_matchQueue, connection);
    const auto player = m_players.find(connection);
    if (player != m_players.end())
        player->second.queued = false;
}

void PacketHandler::RemoveFromSession(ConnectionKey connection)
{
    const auto player = m_players.find(connection);
    if (player == m_players.end() || player->second.sessionId == 0)
        return;
    const auto sessionId = player->second.sessionId;
    player->second.sessionId = 0;
    const auto session = m_sessions.find(sessionId);
    if (session == m_sessions.end())
        return;
    session->second.members.erase(connection);
    if (session->second.members.empty()) {
        m_sessions.erase(session);
        return;
    }
    protocol::SessionPlayerLeft notification;
    notification.set_session_id(sessionId);
    notification.set_player_id(player->second.playerId);
    for (const auto member : session->second.members)
        SendMessage(member, MessageCode::SessionPlayerLeft, 0, notification);
}
