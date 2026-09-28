# GitHub 업로드 점검

- **점검일**: 2026-09-28
- **기준**: `main`의 `80ef9a2`와 현재 작업 파일
- **결론**: 점검 범위에서 업로드를 막는 파일·용량·비밀값 문제 없음
- **점검 당시 남은 절차**: 변경 사항 커밋, 새 원격 저장소 연결, `main` 푸시

## 확인 결과

| 항목 | 결과 |
| --- | --- |
| 현재 파일 | 추적 파일 4,409개와 점검 시작 시 신규 문서 4개 확인 |
| Git 이력 | `main`에 포함된 33개 커밋, 파일 객체 4,554개 확인 |
| 제외 대상 | 이전 백업, 로컬 도구, Unity 캐시, CMake 출력, .NET `bin`·`obj` 제외 |
| 이미 추적된 제외 파일 | 없음. `git ls-files -ci --exclude-standard` 결과 0개 |
| 일반적인 비밀값 패턴 | 현재 텍스트·문서 2,401개, 과거 텍스트·문서 객체 2,604개에서 후보 없음 |
| API 접속 정보 | 커밋된 예시 연결 문자열의 비밀번호는 빈 값 |
| 파일 크기 | 현재 파일과 전체 이력 모두 50 MiB 초과 없음. 최대 파일은 Noto Sans KR 폰트, 약 4.43 MiB |
| 저장소 크기 | 로컬 Git pack 약 75.35 MiB. `main` 파일 객체의 압축 전 합계 약 219.9 MiB |
| Git 구조 | 중첩 서브모듈 없음, 파일 객체 누락 없음, 필수 파일에 LFS 다운로드 의존성 없음 |
| 외부 리소스 | 폰트·캐릭터·Protobuf의 크레딧·라이선스 파일 포함 확인 |
| CI | 워크플로의 실행 조건·권한·명령과 사용 액션 버전 확인. 원격 실행은 아직 미확인 |

GitHub는 일반 Git 파일이 50 MiB를 넘으면 경고하고, 100 MiB를 넘으면 업로드를 차단한다.
현재 파일 크기는 해당 기준 이내다. [GitHub 파일 크기 제한](https://docs.github.com/en/repositories/working-with-files/managing-large-files/about-large-files-on-github)

## 보완한 제외 규칙

루트 [.gitignore](../.gitignore)에 다음 항목 추가:

- 로컬 API 설정: `appsettings.Local.json`, `appsettings.*.Local.json`과 소문자 `local` 변형
- 로컬 비밀값: `secrets.json`, `.ssh/`
- 인증서·서명 키: `*.pem`, `*.key`, `*.pfx`, `*.p12`, `*.keystore`, `*.jks`
- 테스트 결과·커버리지, OS 메타데이터

제외 대상 33개와 포함 대상 27개 경로를 `git check-ignore --no-index`로 확인했다.
`.env.example`, 기본 API 설정, Unity 에셋·`.meta`, Protobuf DLL·생성 코드, 프로젝트·의존성 설정은 계속 포함된다.
Git diff 공백 검사도 통과했다.

## 범위와 남은 확인

- 비밀값 검사는 일반적인 토큰 형식, 비밀번호·키 할당, 개인 키, 인증 URL과 파일명을 기준으로 수행했다. 텍스트와 Office 문서 내부 XML도 확인했다.
- 과거 Word 임시 잠금 파일은 보존된 커밋에 남아 있다. 현재 추적 대상에서는 제거돼 있으며, 이번 점검에서 기존 이력을 다시 쓰지는 않았다.
- `.gitignore`는 이미 커밋된 내용을 지우지 않는다. 과거에 비밀값이 들어간 경우에는 별도 이력 처리가 필요하지만, 이번 검사에서는 발견되지 않았다. [GitHub 제외 규칙 안내](https://docs.github.com/en/get-started/git-basics/ignoring-files)
- 이번 점검은 업로드 대상과 Git 설정에 대한 검사다. 빌드·게임 플레이·DB 정상 연결은 다시 실행하지 않았다. 기존 결과는 [이전 검증 기록](VALIDATION.md) 참고.
- 실제 푸시와 GitHub CI 실행은 하지 않았다. 업로드 후 Actions 결과 확인 필요.
