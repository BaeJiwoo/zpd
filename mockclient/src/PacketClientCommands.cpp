#include "PacketClient.hpp"
#include "ClientSettings.hpp"
#include "EchoRequestBuilder.hpp"
#include <iostream>

bool PacketClient::HandleInputLine(const std::string& line)
{
    if (line == "/quit")
        return false;
    if (line.empty())
        return true;
    if (line == "/help") {
        PrintStatus(ClientSettings::CommandHelp);
        return true;
    }
    if (line == "/status") {
        std::lock_guard lock(m_mutex);
        ShowStatus();
        return true;
    }
    if (line == "/echo" || line.starts_with("/echo ")) {
        const auto text = line == "/echo" ? std::string{} : line.substr(6);
        std::vector<std::pair<Packet, std::string>> chunks;
        std::size_t offset = 0;
        do {
            Packet chunk;
            std::size_t length = 0;
            if (!BuildEchoRequest(text, offset, chunk, length)) {
                CloseWithError("Echo serialization failed.");
                return false;
            }
            chunks.emplace_back(std::move(chunk), text.substr(offset, length));
            offset += length;
        } while (offset < text.size());
        auto batch = std::make_shared<EchoResponseBatch>();
        batch->remainingResponses = chunks.size();
        for (auto& [chunk, expected] : chunks)
            if (!SendRequest(std::move(chunk),
                             {MessageCode::EchoRequest, std::move(expected), batch}))
                return false;
        return true;
    }
    Packet packet;
    if (line == "/ping")
        packet.code = MessageCode::PingRequest;
    else if (line == "/match")
        packet.code = MessageCode::MatchRequest;
    else if (line == "/cancel")
        packet.code = MessageCode::CancelMatchRequest;
    else if (line == "/leave")
        packet.code = MessageCode::LeaveSessionRequest;
    else {
        PrintStatus("Unsupported command. Use /help for commands or /echo <text> for text.");
        return true;
    }
    // Match, cancel and leave requests are empty Protobuf messages.
    return SendRequest(packet, {packet.code, {}, {}});
}
