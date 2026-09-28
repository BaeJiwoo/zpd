# 구조와 책임

시스템을 배치하고 연결해서 게임의 동작을 만듭니다.
전체 배치는 [루트 README의 시스템 배치](../README.md#시스템-배치)를 기준으로 합니다.

게임 클라는 API 서버와 소켓 서버에 연결합니다. 추후 추가할 기획자용 아이템 수정 툴은 API 서버에 연결합니다.
API 서버와 소켓 서버는 각각 담당하는 데이터를 MySQL에서 조회하고 저장하는 구조를 목표로 합니다.

현재 API는 DB 연결 확인만, 소켓 서버는 매칭·세션·에코만 구현되어 있습니다.
소켓 서버의 DB 연결, 기획 툴, 서버의 전투 결과 저장·인증·프로필·보상 처리는 아직 구현되지 않았습니다.
클라이언트에는 로그인과 계정 세션, 로비 HTTP 요청, 솔로 결과·보상 요청이 구현되어 있습니다.

아래 표는 목표로 하는 책임 분담입니다. 각 기능의 구현 상태는 위 설명과 프로젝트별 README를 따릅니다.

| 프로젝트 | 맡을 책임 | 다른 영역과의 경계 |
| --- | --- | --- |
| client | 입력·화면·연출·로컬 솔로 플레이 | 영속 재화·멀티플레이 판정은 서버 결과로 반영 |
| socket-server | 접속·매칭·세션, 향후 실시간 전투 판정과 담당 데이터 저장 | 계정·로비 데이터의 변경 규칙은 API 서버가 담당 |
| api-server | 로비·계정·프로필·소셜·아이템 API와 해당 데이터의 영속 상태 | 멀티플레이 입력 처리·상태 동기화는 소켓 서버 책임 |
| 기획 툴, 추가 예정 | 아이템 정보 조회·수정·변경 내용 확인 | API 서버를 통해 변경 내용을 검증하고 저장 |
| contracts | 전송 메시지·코드·호환 규칙 | C++·Unity 런타임 구현을 공통 라이브러리로 강제하지 않음 |

멀티플레이 결과는 소켓 서버가 확정하도록 구현합니다. 결과 저장과 보상 반영의 담당 서버, 테이블, 전달 방식은 구현 전에 정합니다.
두 서버가 같은 DB에 연결하더라도 같은 보상이나 재화를 각자 지급하지 않도록 쓰기 주체를 정합니다.
기존 API 설계 초안의 내부 정산 API는 후보 방식이며, 소켓 서버의 모든 저장이 API를 거쳐야 한다는 뜻은 아닙니다.
클라이언트의 솔로 결과 보고는 별도 검증 정책이 필요합니다. 로컬 수치를 보상 지급의 증명으로 사용하지 않습니다.

## 데이터를 연결하는 기준

- 테이블마다 조회하는 시스템과 수정하는 시스템을 기록합니다.
- DB 구조 변경은 하나의 마이그레이션 이력으로 관리하는 것을 기준으로 합니다. API 프로젝트에는 EF Core와 `dotnet-ef` 설정이 있지만, 엔티티와 마이그레이션은 아직 없습니다. 첫 DB 기능을 추가할 때 이력 위치와 적용 절차를 정합니다.
- 전투 중 메모리 상태와 DB에 남길 데이터를 구분합니다. 실시간 입력 처리가 DB 응답을 기다리도록 만들지 않습니다.
- 기획 툴에서 아이템을 수정할 때 검증, 변경 이력, 게임에 반영할 시점과 이미 진행 중인 세션의 처리 방식을 정합니다.

기능별 작업 방법은 [공통 작업 순서](guides/FEATURE_WORKFLOW.md)에 기록합니다.

## 프로젝트 내부

클라이언트는 Defense / Gameplay / Lobby / Networking 구분을 유지합니다.
Defense·Gameplay·Lobby 내부는 Model / View / Controller와 DTO / Service로 나눕니다.
HTTP·인증은 Networking, 별도 어셈블리의 TCP 통신은 Networking/Tcp에 둡니다.
소켓 서버는 server / common / mockclient를 유지합니다. common에는 C++ 프레이밍과 유틸리티가 남고
언어 독립적 스키마·코드 정의만 contracts로 이동합니다.
API는 아직 작은 초기 프로젝트이므로 도메인이 추가될 때 src 내부를 기능별로 확장합니다.

## 실행·배포

Unity, CMake, .NET의 빌드 체계는 각각 유지합니다. tools는 이를 실행하는 공통 진입점입니다.
루트 `Build.ps1`은 두 서버만 빌드합니다. Unity 빌드는 에디터에서 별도로 진행합니다.
소켓 서버는 루트의 `contracts/realtime`을 참조하므로 빌드할 때 전체 저장소 구조를 유지합니다.
소켓 서버는 Windows 전용이며 현재 구조로 Linux 컨테이너 실행을 가정하지 않습니다.
하나의 커밋으로 세 프로젝트의 소스 조합을 고정할 수 있지만 배포 시점은 별개입니다.
배포 기록에는 대상 프로젝트, 커밋 SHA, 통신 계약 변경과 이전 클라이언트 지원 범위를 남깁니다.
현재 자동화는 빌드·검증까지이며 배포 파이프라인, 운영 주소, TLS 인증서 설정은 아직 없습니다.

### 현재 개발 주소와 포트

| 대상 | 현재 값 | 설정 위치 |
| --- | --- | --- |
| API `http` 개발 프로필 | `http://localhost:5089` | [launchSettings.json](../api-server/src/Zpd.Api/Properties/launchSettings.json) |
| 소켓 서버 | 모든 IPv4 인터페이스의 TCP `30000` | [NetworkSettings.hpp](../socket-server/common/include/NetworkSettings.hpp); 실행 인자로 포트 변경 가능 |
| 콘솔 mockclient | `127.0.0.1:30000` | 같은 C++ 기본 포트; 실행 인자로 포트 변경 가능 |
| Unity 매칭 테스트 | `127.0.0.1:20000` | [ConnectionTest.unity](../client/Assets/Scenes/ConnectionTest.unity)의 직렬화 값, [NetworkSettings.cs](../client/Assets/Scripts/Networking/Tcp/NetworkSettings.cs)의 코드 기본값 |
| Unity 로그인 | `https://127.0.0.1:18080/api/v1` | [Login.unity](../client/Assets/Scenes/Login.unity)의 `api_root`, [ApiClient.cs](../client/Assets/Scripts/Networking/ApiClient.cs)의 `DefaultRoot` |
| 로비·솔로 결과·보상 요청 | 로그인 세션의 API 주소 | [AccountSession.cs](../client/Assets/Scripts/Networking/AccountSession.cs)의 `ApiRoot`와 Bearer 토큰 |

클라이언트의 로그인 기본 주소에는 현재 서비스가 없습니다. 이 저장소의 API 서버에도 인증·로비·결과·보상 라우트가 없어, 주소만 변경해서 해당 기능을 이용할 수는 없습니다.

현재 TCP 기본 포트가 서로 다르므로 실행 시 맞춰야 합니다. 루트 실행 명령을 사용한다면 Unity Port 입력창에 `30000`을 넣습니다.
Unity의 코드 기본값을 바꿔도 이미 저장된 씬의 직렬화 값은 함께 바뀌지 않으므로 둘 다 확인합니다.
다른 PC에서 접속할 때는 `localhost`나 `127.0.0.1` 대신 실제 서버 주소를 사용합니다. 콘솔 mockclient는 현재 loopback 주소로 고정돼 있습니다.
