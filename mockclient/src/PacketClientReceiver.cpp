#include "PacketClient.hpp"
#include "proto/echo.pb.h"
#include "proto/matchmaking.pb.h"
#include <iostream>
#include <stdexcept>

template <typename Message> Message PacketClient::ParseMessage(const std::vector<char>& payload)
{
    Message message;
    if (!message.ParseFromArray(payload.data(), static_cast<int>(payload.size())))
        throw std::runtime_error("Response/notification parsing failed.");
    return message;
}

void PacketClient::HandleServerPacket(const PacketHeader& header, const std::vector<char>& payload)
{
    std::lock_guard lock(m_mutex);
    if (header.code == MessageCode::MatchFound) {
        const auto message = ParseMessage<protocol::MatchFound>(payload);
        std::set<std::uint64_t> members(message.player_ids().begin(), message.player_ids().end());
        if (header.requestId != 0 || header.error != ErrorCode::None || !m_queued ||
            m_sessionId || !message.session_id() || members.size() < 2 ||
            members.size() != static_cast<std::size_t>(message.player_ids_size()) ||
            members.contains(0) || !members.contains(m_playerId))
            throw std::runtime_error("Invalid match notification.");
        m_sessionId = message.session_id();
        m_members = std::move(members);
        m_queued = false;
        std::cout << "Match found. Session: " << m_sessionId << '\n';
        ShowStatus();
        return;
    }
    if (header.code == MessageCode::SessionPlayerLeft) {
        const auto message = ParseMessage<protocol::SessionPlayerLeft>(payload);
        if (header.requestId != 0 || header.error != ErrorCode::None || !m_sessionId ||
            message.session_id() != m_sessionId || message.player_id() == m_playerId ||
            !m_members.contains(message.player_id()))
            throw std::runtime_error("Invalid session leave notification.");
        m_members.erase(message.player_id());
        std::cout << "Player left session: " << message.player_id() << '\n';
        ShowStatus();
        return;
    }
    const auto it = m_pending.find(header.requestId);
    if (it == m_pending.end() || header.requestId == 0)
        throw std::runtime_error("Unknown response message or request ID.");
    const auto pending = it->second;
    const bool echo = pending.code == MessageCode::PingRequest || pending.code == MessageCode::EchoRequest;
    const auto expectedCode = echo ? pending.code : ResponseCodeFor(pending.code);
    if (header.code != expectedCode || (echo && header.error != ErrorCode::None))
        throw std::runtime_error("Unknown response message or request ID.");
    if (header.error != ErrorCode::None) {
        if (!payload.empty())
            throw std::runtime_error("Error response body must be empty.");
        std::cout << "Request " << header.requestId << " failed: server error "
                  << static_cast<unsigned int>(header.error);
        if (header.error == ErrorCode::InvalidState)
            std::cout << " (invalid state; use /status to check queue/session)";
        std::cout << std::endl;
        m_pending.erase(it);
        return;
    }
    switch (pending.code) {
    case MessageCode::PingRequest:
        if (!payload.empty())
            throw std::runtime_error("Unexpected Ping body.");
        std::cout << "Pong" << std::endl;
        break;
    case MessageCode::EchoRequest: {
        const auto reply = ParseMessage<protocol::EchoRequest>(payload);
        if (reply.data() != pending.expectedEchoText)
            throw std::runtime_error("Echo payload mismatch.");
        pending.batch->response += reply.data();
        if (--pending.batch->remainingResponses == 0)
            std::cout << "Echo: " << pending.batch->response << std::endl;
        break;
    }
    case MessageCode::MatchRequest: {
        const auto message = ParseMessage<protocol::MatchResponse>(payload);
        if (!message.player_id() || m_queued || m_sessionId ||
            (m_playerId && m_playerId != message.player_id()))
            throw std::runtime_error("Invalid match response.");
        m_playerId = message.player_id();
        m_queued = true;
        std::cout << "Queued for matchmaking. Player: " << m_playerId << std::endl;
        break;
    }
    case MessageCode::CancelMatchRequest:
        ParseMessage<protocol::CancelMatchResponse>(payload);
        if (!m_queued || m_sessionId)
            throw std::runtime_error("Invalid cancel response.");
        m_queued = false;
        std::cout << "Matchmaking cancelled." << std::endl;
        break;
    case MessageCode::LeaveSessionRequest: {
        const auto message = ParseMessage<protocol::LeaveSessionResponse>(payload);
        if (!m_sessionId || message.session_id() != m_sessionId)
            throw std::runtime_error("Invalid leave response.");
        m_sessionId = 0;
        m_members.clear();
        std::cout << "Left session." << std::endl;
        break;
    }
    default:
        throw std::runtime_error("Unknown server message.");
    }
    m_pending.erase(it);
}

void PacketClient::ReceivePackets()
{
    try {
        while (m_running) {
            char bytes[ProtocolLimits::HeaderSize];
            if (!ReceiveExact(bytes, sizeof(bytes))) {
                if (m_running)
                    CloseWithError("Server disconnected.");
                break;
            }
            const auto header = PacketHeader::Read(bytes);
            if (header.packetSize < ProtocolLimits::HeaderSize ||
                header.packetSize > ProtocolLimits::MaxPacketBytes)
                throw std::runtime_error("Invalid server packet length.");
            std::vector<char> payload(header.packetSize - ProtocolLimits::HeaderSize);
            if (!ReceiveExact(payload.data(), payload.size()))
                throw std::runtime_error("Server disconnected during a packet.");
            HandleServerPacket(header, payload);
        }
    } catch (const std::exception& error) {
        CloseWithError(error.what());
    }
    m_running = false;
    std::lock_guard lock(m_mutex);
    for (const auto& [id, request] : m_pending)
        std::cout << "Request " << id << " failed: connection closed.\n";
    m_pending.clear();
    m_members.clear();
    m_sessionId = 0;
    m_queued = false;
}
