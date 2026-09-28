using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using Google.Protobuf;
using Protocol;

namespace Zpd.Networking
{
    public enum MatchState
    {
        Disconnected,
        Ready,
        Waiting,
        InSession
    }

    public sealed class MatchmakingClient
    {
        private readonly NetworkClient network_client;
        private readonly List<ulong> session_player_ids = new List<ulong>();
        private readonly Stopwatch request_timer = new Stopwatch();
        private uint pending_request_id;
        private byte expected_response_code;

        public MatchState State { get; private set; }
        public ulong PlayerId { get; private set; }
        public ulong SessionId { get; private set; }
        public IReadOnlyList<ulong> Players => session_player_ids.AsReadOnly();
        public bool IsBusy => pending_request_id != 0;

        public event Action<string> MessageReceived;

        public MatchmakingClient(NetworkClient client)
        {
            network_client = client ?? throw new ArgumentNullException(nameof(client));
        }

        public void HandleConnected()
        {
            if (!network_client.IsConnected)
            {
                return;
            }

            State = MatchState.Ready;
            RequestMatch();
        }

        public void HandleDisconnected()
        {
            State = MatchState.Disconnected;
            PlayerId = 0;
            SessionId = 0;
            session_player_ids.Clear();
            CompleteRequest();
        }

        public void RequestMatch()
        {
            if (State != MatchState.Ready)
            {
                throw new InvalidOperationException("Leave the current session before requesting a match.");
            }

            SendRequest(MessageCode.MatchRequest, MessageCode.MatchResponse, new MatchRequest());
            MessageReceived?.Invoke("Match requested.");
        }

        public void CancelMatch()
        {
            if (State != MatchState.Waiting)
            {
                throw new InvalidOperationException("There is no waiting match to cancel.");
            }

            SendRequest(
                MessageCode.CancelMatchRequest,
                MessageCode.CancelMatchResponse,
                new CancelMatchRequest());
        }

        public void LeaveSession()
        {
            if (State != MatchState.InSession)
            {
                throw new InvalidOperationException("There is no session to leave.");
            }

            SendRequest(
                MessageCode.LeaveSessionRequest,
                MessageCode.LeaveSessionResponse,
                new LeaveSessionRequest());
        }

        public void Tick()
        {
            if (IsBusy && request_timer.ElapsedMilliseconds >= 5000)
            {
                MessageReceived?.Invoke("Match request timed out. Check that the updated server is running.");
                network_client.Disconnect();
                HandleDisconnected();
            }
        }

        public bool HandlePacketReceived(Packet packet)
        {
            if (!IsMatchPacket(packet.Code))
            {
                return false;
            }

            try
            {
                if (packet.Code == MessageCode.MatchFound || packet.Code == MessageCode.SessionPlayerLeft)
                {
                    if (packet.RequestId != 0 || packet.Error != 0)
                    {
                        throw new InvalidDataException("Invalid match notification header.");
                    }

                    HandleNotification(packet);
                    return true;
                }

                if (!IsBusy || packet.RequestId != pending_request_id || packet.Code != expected_response_code)
                {
                    throw new InvalidDataException("Unexpected match response. Check the server version.");
                }

                CompleteRequest();

                if (packet.Error != 0)
                {
                    if (packet.Payload.Length != 0)
                    {
                        throw new InvalidDataException("Error response must have an empty payload.");
                    }

                    MessageReceived?.Invoke(
                        "Match request rejected: " + packet.Error + (packet.Error == 23
                        ? " (state changed; check the current session)."
                        : "."));
                    return true;
                }

                switch (packet.Code)
                {
                    case MessageCode.MatchResponse:
                        var entered = MatchResponse.Parser.ParseFrom(packet.Payload);
                        if (entered.PlayerId == 0)
                        {
                            throw new InvalidDataException("Invalid player ID.");
                        }

                        PlayerId = entered.PlayerId;
                        State = MatchState.Waiting;
                        MessageReceived?.Invoke("Waiting for other players. Player " + PlayerId);
                        break;
                    case MessageCode.CancelMatchResponse:
                        CancelMatchResponse.Parser.ParseFrom(packet.Payload);
                        State = MatchState.Ready;
                        MessageReceived?.Invoke("Match cancelled.");
                        break;
                    case MessageCode.LeaveSessionResponse:
                        var left = LeaveSessionResponse.Parser.ParseFrom(packet.Payload);
                        if (left.SessionId != SessionId || SessionId == 0)
                        {
                            throw new InvalidDataException("Unexpected session leave response.");
                        }

                        SessionId = 0;
                        session_player_ids.Clear();
                        State = MatchState.Ready;
                        MessageReceived?.Invoke("Left the session.");
                        break;
                }
            }
            catch (Exception error) when (error is InvalidDataException || error is InvalidProtocolBufferException)
            {
                MessageReceived?.Invoke(error.Message);
                network_client.Disconnect();
                HandleDisconnected();
            }

            return true;
        }

        public static bool IsMatchPacket(byte code)
        {
            return code == MessageCode.MatchRequest || code == MessageCode.CancelMatchRequest || code == MessageCode.LeaveSessionRequest || code == MessageCode.MatchResponse || code == MessageCode.CancelMatchResponse || code == MessageCode.LeaveSessionResponse || code == MessageCode.MatchFound || code == MessageCode.SessionPlayerLeft;
        }

        private void HandleNotification(Packet packet)
        {
            if (packet.Code == MessageCode.MatchFound)
            {
                var matched = MatchFound.Parser.ParseFrom(packet.Payload);
                var players = new HashSet<ulong>(matched.PlayerIds);

                if (State != MatchState.Waiting || matched.SessionId == 0 || players.Count < 2 || players.Count > 16 || players.Count != matched.PlayerIds.Count || players.Contains(0) || !players.Contains(PlayerId))
                {
                    throw new InvalidDataException("Invalid matched session.");
                }

                SessionId = matched.SessionId;
                session_player_ids.Clear();
                session_player_ids.AddRange(matched.PlayerIds);
                State = MatchState.InSession;
                MessageReceived?.Invoke("Matched! Session " + SessionId + " / players: " + string.Join(", ", session_player_ids));
                return;
            }

            var left = SessionPlayerLeft.Parser.ParseFrom(packet.Payload);

            if (State != MatchState.InSession || left.SessionId != SessionId || left.PlayerId == PlayerId || !session_player_ids.Remove(left.PlayerId))
            {
                throw new InvalidDataException("Invalid session departure.");
            }

            MessageReceived?.Invoke("Player " + left.PlayerId + " left the session.");
        }

        private void SendRequest(byte code, byte expectedResponse, IMessage message)
        {
            if (IsBusy)
            {
                throw new InvalidOperationException("A match request is already pending.");
            }

            pending_request_id = network_client.SendRequest(code, message.ToByteArray());
            expected_response_code = expectedResponse;
            request_timer.Restart();
        }

        private void CompleteRequest()
        {
            pending_request_id = 0;
            expected_response_code = 0;
            request_timer.Reset();
        }
    }
}
