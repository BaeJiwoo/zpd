# 실시간 TCP 계약

`proto/*.proto`는 본문 스키마, `codes.json`은 메시지·오류 번호와 응답 매핑의 원본입니다.
`codegen.json`은 protoc 버전, 체크인된 C# 런타임의 버전·SHA-256, Unity 생성 대상 스키마를 고정합니다.
C++ 의존성은 `socket-server/vcpkg.json`의 baseline으로 고정합니다.

## 현재 지원 범위

| 요청 | 응답 | 동작 |
| --- | --- | --- |
| 16 MatchRequest | 144 MatchResponse | FIFO 매칭 대기 |
| 17 CancelMatchRequest | 145 CancelMatchResponse | 대기 취소 |
| 18 LeaveSessionRequest | 146 LeaveSessionResponse | 세션 퇴장 |
| 서버 알림 | 208 MatchFound | 세션과 전체 참가자 목록 |
| 서버 알림 | 209 SessionPlayerLeft | 퇴장한 참가자 |

현재 서버는 위 매칭 요청 외 패킷을 원래 헤더·본문 그대로 에코합니다.
다른 `.proto`와 메시지 번호는 이전 설계의 예약 정의이며 방·채팅·전투 구현을 의미하지 않습니다.
인증, 재접속 복구, 영속 사용자 ID와 전투 틱은 아직 없습니다.

## 프레임

| 오프셋 | 크기 | 의미 |
| --- | --- | --- |
| 0 | 2 | 전체 길이, big-endian, 8~4096 |
| 2 | 1 | 메시지 번호 |
| 3 | 1 | 오류 번호 |
| 4 | 4 | requestId, big-endian |
| 8 | 최대 4088 | 본문 |

매칭 요청은 오류 0과 0이 아닌 requestId를 사용합니다. 응답은 requestId를 유지하며 알림은 0입니다.
매칭 오류 2는 본문, 3은 요청 헤더, 23은 현재 상태 문제입니다.
프레이밍 구현은 언어별로 유지하고 CTest와 C# 통합 검증으로 같은 바이트를 확인합니다.

## 생성

루트에서 `./tools/Build.ps1 -Target Socket` 실행 후 `./tools/Generate-Protocol.ps1`을 실행합니다.
C++ 본문 코드는 `socket-server/common/CMakeLists.txt`에 등록된 스키마에서 빌드 폴더로 생성되며 Git에서 제외됩니다.
Unity C# 본문은 `codegen.json`의 `csharpSchemas`에 지정한 스키마만 생성합니다. 현재는 `matchmaking.proto` 하나입니다.
스키마 파일을 추가하면 CMake의 대상 목록과 Unity에서 사용할 생성 대상도 함께 확인합니다.
Unity C# 본문과 양쪽 언어의 메시지·오류 코드 상수는 체크인하며 `-Check`가 재생성 결과와 비교합니다.
헤더 크기·프레임 한도와 접속 기본값은 아직 언어별 코드에 있습니다. 생성 검사가 이 값들의 일치까지 보장하지는 않습니다.
체크인된 Google.Protobuf.dll은 원래 런타임을 유지하고 해시로 검증합니다.
업그레이드할 때 vcpkg baseline, codegen 설정, 런타임과 NOTICE를 함께 검토합니다.
