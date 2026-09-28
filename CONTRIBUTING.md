# 개발 규칙

## 브랜치와 변경

- `main`: 빌드·기본 검증을 통과한 통합 기준.
- `feat/match-ticket`, `fix/result-upload`처럼 기능별 짧은 브랜치를 사용합니다.
- 클라·서버별 영구 개발 브랜치를 만들지 않습니다. 관련 계약과 양쪽 구현은 같은 PR에 포함합니다.
- 커밋 영역은 `client`, `socket`, `api`, `contracts`, `tools`, `docs`를 사용합니다.
- 기능 수정과 대규모 파일 이동은 가능하면 별도 커밋으로 분리합니다.

## 검증

루트에서 `./tools/Build.ps1`, `./tools/Test.ps1`을 실행합니다.
각 명령의 `-Target`으로 프로젝트별 검증도 가능합니다. 계약 변경은 송수신 양쪽을 검증합니다.
Unity 씬·스크립트 변경에는 `./tools/Test-Unity.ps1`을 실행하고 필요한 Play 검증을 기록합니다.
이 검사는 컴파일과 누락 스크립트를 확인하며 게임 플레이 검증 전체를 대신하지 않습니다.
기존 솔로 게임 검증은 `client/Tools/DefenseValidation/Run.ps1`을 사용할 수 있습니다.

현재 GitHub CI는 두 서버와 C# 네트워킹·계약 검증을 매 PR마다 실행합니다.
필수 체크는 최종 `CI`로 지정합니다. Unity 라이선스·에디터가 있는 CI 실행 환경은 별도로 구성해야 합니다.

## Unity와 바이너리

`Assets`, `Packages`, `ProjectSettings`와 `.meta`를 함께 추적합니다.
Library / Temp / Logs / UserSettings / 생성된 IDE 프로젝트 / 빌드 산출물은 제외합니다.
파일 이동 시 `.meta`를 함께 옮기고 GUID를 유지합니다. 씬·프리팹은 텍스트 직렬화를 유지합니다.
현재 리소스는 기존 Git 이력 그대로 보존합니다. Git LFS는 아직 도입하지 않았습니다.
향후 크고 자주 바뀌는 원본 리소스에 선택적으로 도입하고, 기존 이력 재작성은 별도 이전 작업으로 다룹니다.
외부 리소스의 라이선스·크레딧은 `client/Assets/ThirdParty`에 유지합니다.

## 설정과 비밀

도구 경로는 환경 변수와 루트 상대 경로를 사용합니다. 개인 절대 경로를 체크인하지 않습니다.
DB 접속 비밀번호는 User Secrets 또는 `ConnectionStrings__DefaultConnection`으로 설정합니다.
로컬 HTTP는 Unity Editor/Development Build에 한정합니다. 배포 주소는 HTTPS를 사용합니다.

## 생성 파일

TCP는 contracts/realtime을 수정한 뒤 `tools/Generate-Protocol.ps1`을 실행합니다.
HTTP는 API 구현을 수정·빌드한 뒤 `tools/Export-OpenApi.ps1`을 실행합니다.
생성 파일, 생성기 버전 설정과 구현을 같은 변경에 포함하고 `-Check`로 확인합니다.
