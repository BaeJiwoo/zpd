# zpd-server

A Windows C++20 packet server skeleton using WinSock2, IOCP, Protobuf, CMake and vcpkg.

See the current [wire protocol](../contracts/realtime/README.md). The [MO requirements](../docs/archive/socket-server/MO_서버_단계별_개발_요구사항.md) are a historical design reference.

## Requirements

- Visual Studio 2026 or Build Tools 2026 with Desktop development with C++ and the Windows SDK
- CMake 4.2 or later for the Visual Studio 2026 generator
- vcpkg with the `VCPKG_ROOT` environment variable set to its installation directory
- VS Code extensions: C/C++ (`ms-vscode.cpptools`) and CMake Tools (`ms-vscode.cmake-tools`)

This computer uses `C:\vcpkg`. Adjust the path for other installations. Restart VS Code after changing environment variables.

## Build and run

Run from the project root in PowerShell:

```powershell
$env:VCPKG_ROOT = 'C:\vcpkg'
cmake --preset windows-x64
cmake --build --preset debug
.\out\build\windows-x64\Debug\zpd-server.exe

cmake --build --preset release
.\out\build\windows-x64\Release\zpd-server.exe
```

In VS Code, select the `windows-x64` configure preset. Press Ctrl+Shift+B to build Debug or F5 to build and debug. Use Tasks: Run Task > CMake: Build Release for Release builds.

The server listens on all IPv4 interfaces at TCP port 20000 and accepts up to 1000 connections. Supply a port argument to override the default, such as `zpd-server.exe 9100`. Press Enter or close standard input to stop the server.

Both executables print their exit status after releasing network resources. In an interactive
console, press Enter at `Press Enter to exit...` to close the program, including after startup
errors. For the server, the first Enter stops the server and the second Enter closes the program.
Redirected input and automated runs exit without this additional pause.

## Tests

```powershell
ctest --preset debug
ctest --preset release
```

The server tests cover packet framing, fragmented and coalesced input, connection reuse,
handler restart, binary packet echo, send-failure recovery, and disconnection on malformed frames.

The console client follows the current packet server: FIFO matchmaking (request codes 16-18)
and raw packet echo. Room/chat/game commands are not supported by this client.

| Command | Behavior |
| --- | --- |
| `/match` | Join the matchmaking queue; print the player ID and eventual session/members. |
| `/cancel` | Cancel a queued matchmaking request. |
| `/leave` | Leave the current session. |
| `/status` | Show the locally tracked player ID, queue state, session and members. |
| `/ping` | Send an empty packet and print `Pong` when it is echoed. |
| `/echo <text>` | Send text and verify the echoed data; long text is split into packets. |
| `/help` | Show supported commands. |
| `/quit` | Disconnect, then wait for Enter in an interactive console. |

Connections start idle; send `/match` in two clients to form a session with the default
server configuration. Match and player-leave notifications arrive while waiting for input.
Invalid state requests print a server error without disconnecting. Plain text and unsupported
commands are rejected locally. There is no authentication, room chat, movement or game command UI.
The client integration test covers two-client matching, cancellation, leaving, disconnection
notifications, ping/echo and rejection of unsupported commands and invalid replies.

## Project structure

```text
root/
├── mockclient/
│   ├── include/
│   ├── src/
│   ├── tests/
│   └── CMakeLists.txt
├── common/
│   ├── include/
│   ├── src/
│   ├── README.md
│   └── CMakeLists.txt
├── server/
│   ├── include/
│   ├── src/
│   ├── tests/
│   └── CMakeLists.txt
├── docs/
├── CMakeLists.txt
├── CMakePresets.json
└── vcpkg.json
```

- `mockclient`: the console packet/matchmaking client and its process regression tests.
- `common`: C++ message/error codes, packet framing, protocol limits, network defaults and serialization helpers. The schema source is `../contracts/realtime/proto`; wire codes are generated from `../contracts/realtime/codes.json`. See [Unity sharing](common/README.md).
- `server`: IOCP transport, connection management, packet framing, a basic event worker and server tests.
- `docs`: current navigation and C++ coding style. Historical designs are under `../docs/archive/socket-server`.

Each module owns its CMake targets. The root configures shared build options and includes the modules. Headers are exposed through target dependencies; the mock client does not include server headers. Add implementation files to the owning module's `CMakeLists.txt`.

Executables remain under `out/build/windows-x64/Debug` or `Release`, so existing run commands and VS Code tasks continue to work. Run build and test commands from the repository root.

## Server behavior

The 8-byte big-endian packet header carries length, message code, error and request ID. Protobuf bodies are limited to 4088 bytes. One logic worker waits for connected, packet-received and disconnected events and removes them from the queue. Each branch calls a separate handler. Connection and disconnection handlers maintain player and session state. Matchmaking requests are dispatched to dedicated handlers; other packets are logged and echoed with their code, error, request ID and binary payload unchanged. Transport send/disconnect callbacks are supplied at startup, and send failure disconnects that connection without stopping the worker. Matchmaking owns a FIFO waiting queue and session membership on the logic worker. Every two queued players receive a MatchFound notification; cancellation and disconnect remove queued players, and leaving or disconnecting notifies remaining session members. Empty sessions are deleted. There are no periodic game ticks or automatic session backfill. This raw echo does not implement the request/response codes expected by the room client. The old room/chat/position design documents remain reference behavior. Matchmaking uses ../contracts/realtime/proto/matchmaking.proto; request/response codes are 16/144, 17/145, 18/146 and notifications are 208 (MatchFound) and 209 (SessionPlayerLeft). Requests require a nonzero request ID and zero error. Duplicate queue requests or invalid state return error 23. PlayersPerSession in server/include/ServerLimits.hpp configures 2-16 players; TryMatchPlayers in server/src/PacketHandler.Matchmaking.cpp controls the FIFO policy. A TCP connection is queued only after MatchRequest; the Unity client sends it automatically on connection. IDs are temporary, with no authentication or reconnect recovery.

Sends are serialized per connection. Each connection allows up to 256 queued sends of at most 4096 bytes each. A full queue or send failure disconnects that client. Pending I/O completions are drained before connection slots are reused or released. Peer FIN and server shutdown close the connection without guaranteeing delivery of queued responses.

Call `Start`, `Stop`, and `Port` from the owner thread. Callbacks are serialized per connection but may run concurrently across connections. Callbacks must not call `Stop` or allow exceptions to escape. Use `Send` and `Disconnect` from callbacks for the same connection. Client IDs are reusable slot numbers, not persistent session identifiers. Derived servers must call `Stop()` in their destructors while their callback implementations are still alive.

WinSock2 is provided by the Windows SDK. Protobuf is installed through the vcpkg manifest and generated during the build.

For Visual Studio 2022, change the preset generator to `Visual Studio 17 2022`, set the preset minimum CMake version to 3.25 or later, and use a separate build directory.

References: [vcpkg CMake integration](https://learn.microsoft.com/en-us/vcpkg/users/buildsystems/cmake-integration), [VS Code CMake presets](https://github.com/microsoft/vscode-cmake-tools/blob/main/docs/cmake-presets.md), [asynchronous socket closure](https://learn.microsoft.com/en-us/windows/win32/api/winsock/nf-winsock-closesocket).

