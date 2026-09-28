# 개발 규칙

작업 종류별 방법은 [작업 가이드](docs/guides/README.md)를 참고합니다.
새 기능은 [공통 작업 순서](docs/guides/FEATURE_WORKFLOW.md)로 정리하고, API 추가는 [라우팅 가이드](docs/guides/API_ROUTING.md)를 따릅니다.

## 브랜치와 변경

- `main`: 빌드·기본 검증을 통과한 통합 기준.
- `feat/match-ticket`, `fix/result-upload`처럼 기능별 짧은 브랜치를 사용합니다.
- 클라·서버별 영구 개발 브랜치를 만들지 않습니다. 관련 계약과 양쪽 구현은 같은 PR에 포함합니다.
- 커밋 영역은 `client`, `socket`, `api`, `contracts`, `tools`, `docs`를 사용합니다.
- 기능 수정과 대규모 파일 이동은 가능하면 별도 커밋으로 분리합니다.

## 검증

코드 변경은 루트에서 `./tools/Build.ps1`, `./tools/Test.ps1` 순으로 검증합니다.
두 명령에는 Unity 빌드·플레이 검증이 포함되지 않습니다. 각 명령의 `-Target`으로 변경 영역을 좁힐 수 있습니다.
`Test.ps1 -Target Contracts`는 TCP 생성 결과만 확인하고, HTTP 명세는 `-Target Api`에 포함됩니다.
계약 변경은 송수신 양쪽을 검증합니다. 문서만 바꾼 경우에는 코드·명령과 대조하고 링크와 diff를 확인합니다.
Unity 씬·스크립트 변경에는 `./tools/Test-Unity.ps1`을 실행하고 필요한 Play 검증을 기록합니다.
이 검사는 컴파일과 누락 스크립트를 확인하며 게임 플레이 검증 전체를 대신하지 않습니다.
기존 솔로 게임 검증은 `client/Tools/DefenseValidation/Run.ps1`을 사용할 수 있습니다.
이 스크립트는 Unity 프로세스를 시작한 뒤 바로 반환합니다. 출력된 PID나 스크립트 종료만으로 통과를 판단하지 않고,
`client/Temp/SoloDefenseValidation/validation-result.txt`의 `PASS` 또는 `FAIL`과 로그를 확인합니다.
패키지 재사용을 위해 먼저 원본 Unity 프로젝트를 열어 `Library/PackageCache`를 준비합니다.

GitHub 워크플로는 두 서버와 C# 네트워킹·계약 검증을 매 PR과 `main` 푸시에서 실행하도록 설정돼 있습니다.
원격 CI 실행은 아직 확인하지 않았습니다. 브랜치 보호를 설정할 때 필수 체크를 최종 `CI`로 지정합니다.
Unity 라이선스·에디터가 있는 CI 실행 환경은 별도로 구성해야 합니다.

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
