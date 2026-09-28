# 모노레포 이전 검증

검증일: 2026-09-28. 코드 기준 커밋: `73ee3bf`.

## 실행 결과

| 검증 | 결과 |
| --- | --- |
| Windows CMake Debug 빌드 | 서버·mockclient·테스트 실행 파일 생성 성공 |
| ASP.NET Core Debug 빌드 | 성공, 경고 0 / 오류 0 |
| CTest | packet-client-integration, server-integration 모두 통과 |
| C# 네트워크 통합 | 프레이밍·실제 서버 에코·FIFO 매칭·취소 경합·세션 분리·퇴장·재접속·잘못된 요청 검증 통과 |
| TCP 생성 일치 | 공통 스키마·코드 정의에서 재생성한 결과와 체크인 파일 일치 |
| API 스모크 | OpenAPI 라우트 노출과 DB 연결 실패 시 HTTP 503 확인 |
| HTTP 생성 일치 | 실제 API에서 추출한 OpenAPI와 체크인 스냅샷 일치 |
| Unity 6000.3.11f1 | 새 client 경로에서 컴파일 성공, 게임 씬 3개의 누락 스크립트 검사 통과 |
| 기존 Unity 에셋 | 기존 `.meta` 2,175개의 GUID 보존 확인 |
| Git 이력 | 세 원본 기준 커밋이 main의 조상임을 확인, 객체 연결 검사 통과 |
| 백업 | 세 원본 bundle 검증 및 SHA-256 일치 확인 |
| 문서·스크립트 | 현재 문서의 상대 링크, PowerShell 구문, Git diff 공백 검사 통과 |

## 새 복제본 검증

통합 저장소를 `.tools/verify`에 `git clone --no-hardlinks`로 복제했습니다.
이 복제본에는 원본 체크아웃 백업, Unity Library, 이전 CMake 빌드 폴더가 없습니다.
복제본의 `tools/Build.ps1`과 `tools/Test.ps1`을 실행해 두 서버 빌드와 모든 기본 검증이 통과했습니다.
vcpkg와 NuGet의 머신 공용 의존성 캐시는 사용했습니다. 새 머신에 도구를 설치하는 절차 자체를 검증한 것은 아닙니다.

## 검증 범위

Unity 검증은 컴파일과 씬의 누락 스크립트 확인입니다. 실제 화면 조작이나 전체 게임 플레이 검증은 포함하지 않습니다.
API 검증은 개발 DB를 사용하지 않으므로 MySQL 정상 연결·도메인 CRUD를 검증하지 않습니다.
로비·보상·인증·멀티플레이 전투 로직은 기존과 같이 미구현입니다.
GitHub Actions 파일은 추가했으며 원격 CI 실행·배포·브랜치 보호 설정은 수행하지 않았습니다.
