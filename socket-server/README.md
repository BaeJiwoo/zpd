# zpd-server

A Windows C++20 packet server skeleton using WinSock2, IOCP, Protobuf, CMake and vcpkg.

See the current [wire protocol](../contracts/realtime/README.md). The [MO requirements](../docs/archive/socket-server/MO_서버_단계별_개발_요구사항.md) are a historical design reference.

## Requirements

- Visual Studio 2026 or Build Tools 2026 with Desktop development with C++ and the Windows SDK
- CMake 4.2 or later for the Visual Studio 2026 generator
- vcpkg with the `VCPKG_ROOT` environment variable set to its installation directory
- VS Code extensions: C/C++ (`ms-vscode.cpptools`) and CMake Tools (`ms-vscode.cmake-tools`)

The examples use `C:\vcpkg`. Adjust the path for your installation. Restart VS Code after changing environment variables.

## Build and run

Run from the monorepo's `socket-server` folder in PowerShell. Keep the sibling `contracts` folder available:

```powershell
$env:VCPKG_ROOT = 'C:\vcpkg'
cmake --preset windows-x64
cmake --build --preset debug -- /p:VcpkgEnabled=false
.\out\build\windows-x64\Debug\zpd-server.exe

cmake --build --preset release -- /p:VcpkgEnabled=false
.\out\build\windows-x64\Release\zpd-server.exe
```

In VS Code, select the `windows-x64` configure preset. Press Ctrl+Shift+B to build Debug or F5 to build and debug. Use Tasks: Run Task > CMake: Build Release for Release builds.

The server listens on all IPv4 interfaces at TCP port 30000 and accepts up to 1000 connections. Supply a port argument to override the default, such as `zpd-server.exe 20000`. Press Enter or close standard input to stop the server.

The console client also defaults to port 30000 and connects to loopback only. Unity's saved `ConnectionTest` scene defaults to port 20000, so change its Port field to 30000 or explicitly start the server on port 20000.

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
socket-server/
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

Executables are under `socket-server/out/build/windows-x64/Debug` or `Release` relative to the monorepo root. The preset commands above run from `socket-server`; the shared scripts in the [root README](../README.md) run from the monorepo root.

## Server behavior

The 8-byte big-endian packet header carries length, message code, error and request ID. Bodies are limited to 4088 bytes. One logic worker handles connected, packet-received and disconnected events. Connection and disconnection handlers maintain player and session state. Matchmaking requests use Protobuf bodies; other packets are logged and echoed with their code, error, request ID and binary payload unchanged. Transport send/disconnect callbacks are supplied at startup, and send failure disconnects that connection without stopping the worker.

Matchmaking owns a FIFO waiting queue and session membership on the logic worker. With the default configuration, every two queued players receive a MatchFound notification. Cancellation and disconnect remove queued players; leaving or disconnecting notifies remaining session members. Empty sessions are deleted. There are no periodic game ticks or automatic session backfill. The archived room/chat/position documents describe earlier designs, not the current handlers.

Matchmaking uses `../contracts/realtime/proto/matchmaking.proto`. Request/response codes are 16/144, 17/145, 18/146; notifications are 208 (MatchFound) and 209 (SessionPlayerLeft). Matchmaking requests require a nonzero request ID and zero error. Duplicate queue requests or invalid state return error 23. `PlayersPerSession` in `server/include/ServerLimits.hpp` configures 2-16 players; `TryMatchPlayers` in `server/src/PacketHandler.Matchmaking.cpp` controls the FIFO policy. A TCP connection is queued only after MatchRequest; the Unity test client sends it automatically on connection. IDs are temporary, with no authentication or reconnect recovery.

Sends are serialized per connection. Each connection allows up to 256 queued sends of at most 4096 bytes each. A full queue or send failure disconnects that client. Pending I/O completions are drained before connection slots are reused or released. Peer FIN and server shutdown close the connection without guaranteeing delivery of queued responses.

Call `Start`, `Stop`, and `Port` from the owner thread. Callbacks are serialized per connection but may run concurrently across connections. Callbacks must not call `Stop` or allow exceptions to escape. Use `Send` and `Disconnect` from callbacks for the same connection. Client IDs are reusable slot numbers, not persistent session identifiers. Derived servers must call `Stop()` in their destructors while their callback implementations are still alive.

WinSock2 is provided by the Windows SDK. Protobuf is installed through the vcpkg manifest and generated during the build.

For Visual Studio 2022, run `./tools/Build.ps1 -Target Socket -Generator 'Visual Studio 17 2022'` from the monorepo root with CMake 3.25 or later. Use a fresh checkout if the build directory already contains a cache for a different generator. The checked-in presets target Visual Studio 2026.

References: [vcpkg CMake integration](https://learn.microsoft.com/en-us/vcpkg/users/buildsystems/cmake-integration), [VS Code CMake presets](https://github.com/microsoft/vscode-cmake-tools/blob/main/docs/cmake-presets.md), [asynchronous socket closure](https://learn.microsoft.com/en-us/windows/win32/api/winsock/nf-winsock-closesocket).

