# ZPD

Unity 클라이언트, Windows TCP 소켓 서버, ASP.NET Core API 서버를 함께 관리하는 모노레포입니다.
각 프로젝트는 독립적으로 빌드·실행·배포하며, 통신 계약 변경은 같은 PR에서 관리합니다.

| 경로 | 역할 | 현재 상태 |
| --- | --- | --- |
| [client](client/README.md) | Unity 6000.3.11f1 게임 클라이언트 | 솔로 디펜스, 로비 UI, TCP 매칭 테스트 |
| [socket-server](socket-server/README.md) | C++20 / WinSock2 / IOCP 서버 | FIFO 매칭·세션·에코; 전투 틱·인증 미구현 |
| [api-server](api-server/README.md) | ASP.NET Core 10 / EF Core / MySQL | DB 연결 확인; 로비·보상 API 미구현 |
| [contracts](contracts/README.md) | TCP·HTTP 통신 계약 | 스키마·코드 생성·구현된 HTTP 명세 |
| [docs](docs/README.md) | 전체 구조·개발 규칙·설계 | 현재 상태와 제안·과거 문서를 구분 |
| [tools](tools) | 공통 빌드·실행·검증 | PowerShell 7에서 루트 기준 실행 |

## 준비

- PowerShell 7, Git, .NET 10 SDK (`global.json` 기준).
- 소켓 서버: Visual Studio 2026 C++ 도구와 Windows SDK, CMake 4.2+, vcpkg.
- 클라이언트: `client/ProjectSettings/ProjectVersion.txt`에 고정된 Unity.
- API의 DB 성공 응답 확인 시에만 MySQL과 `zpd` DB/계정이 필요합니다.

```powershell
$env:VCPKG_ROOT = 'C:\vcpkg'  # 자신의 설치 경로
./tools/Build.ps1
./tools/Test.ps1
```

Visual Studio 2022 환경에서는 `./tools/Build.ps1 -Generator 'Visual Studio 17 2022'`를 사용합니다
(CMake 3.25+). 기존 빌드 폴더에 다른 generator가 설정되어 있으면 새 체크아웃에서 빌드하세요.
`Test.ps1`은 소켓 CTest, C# 실제 서버 통신, 프로토콜 생성 일치, API 스모크·OpenAPI 일치를 검증합니다.
API 스모크 테스트는 별도 로컬 프로세스와 실패용 DB 주소를 사용해 개발 DB에 접근하지 않습니다.

## 실행

각 서버는 별도 터미널에서 실행합니다.

```powershell
./tools/Run.ps1 -Target Socket      # TCP 127.0.0.1:20000
./tools/Run.ps1 -Target Api         # HTTP http://localhost:5089
./tools/Run.ps1 -Target MockClient  # 개발용 콘솔; /match, /cancel, /leave
```

API의 실제 DB 연결 정보는 다음처럼 로컬 User Secrets에 설정합니다.

```powershell
dotnet user-secrets set 'ConnectionStrings:DefaultConnection' 'Server=localhost;Port=3306;Database=zpd;User ID=zpd;Password=YOUR_PASSWORD;' --project api-server/src/Zpd.Api
```

Unity Hub에서 **`client` 폴더**를 프로젝트로 엽니다. `SoloDefense`는 솔로 게임,
`ConnectionTest`는 두 클라이언트의 TCP 매칭, `Lobby`는 로비 UI입니다.
게임 결과·보상 API는 미구현이므로 해당 요청의 실패 표시는 정상입니다.
로컬 HTTP는 Editor/Development Build에서만 허용하며 배포 시 HTTPS 주소를 지정합니다.
Unity 컴파일과 씬의 누락 스크립트는 에디터를 닫고 아래 명령으로 확인합니다.

```powershell
./tools/Test-Unity.ps1
```

VS Code에서는 `zpd.code-workspace`를 열면 프로젝트별 디버깅 설정과 루트 작업을 사용할 수 있습니다.

## 계약 변경

```powershell
./tools/Generate-Protocol.ps1       # TCP C# 메시지와 C++/C# 코드 상수 생성
./tools/Generate-Protocol.ps1 -Check
./tools/Build.ps1 -Target Api
./tools/Export-OpenApi.ps1          # 실제 구현에서 HTTP 명세 추출
./tools/Export-OpenApi.ps1 -Check
```

TCP 원본은 `contracts/realtime`, HTTP 원본은 API 구현입니다. 생성 파일을 직접 수정하지 않습니다.
미구현 API 설계는 [설계 초안](docs/design/API_IMPLEMENTATION.md)에 있습니다.

## Git과 CI

`main`을 통합 기준으로 사용하고 기능별 짧은 브랜치·PR로 작업합니다.
[기여 규칙](CONTRIBUTING.md), [구조와 책임](docs/ARCHITECTURE.md),
[이력 이전·복구 안내](docs/MIGRATION.md)를 참고하세요.

GitHub Actions는 소켓·클라이언트 네트워킹과 API를 독립된 작업으로 검증합니다.
처음에는 모든 PR에서 두 작업을 실행하며, 브랜치 보호의 필수 체크 이름은 `CI`입니다.
Unity 에디터 검증은 라이선스와 에디터가 설치된 로컬 환경에서 별도로 실행합니다.
CI 파일은 준비되어 있지만 원격 저장소 연결·푸시·브랜치 보호 설정은 별도 작업입니다.
