# 클라이언트 갱신 검증

- 검증일: 2026-09-28
- 원본: `C:/Users/10sae/Projects/zpd-client`
- 원본 커밋: `0664488ab5c1bd9d3da34c3e2ab45f5417c1be07`
- 반영 위치: 모노레포의 `client`
- 원본 폴더 보존, 중첩 Git 저장소·Unity 캐시 제외

## 반영 내용

- 로그인 화면·계정 세션·공통 HTTP 클라이언트
- 로비·솔로 디펜스의 MVC 구조와 씬 이동
- 프로필·인벤토리·아이템 사용·결과·보상 요청
- TCP 코드의 `Networking/Tcp` 이동
- 원본 에셋·씬·프로젝트 설정·문서·검증 코드
- 모노레포 공통 프로토콜 생성과 Unity 검증 도구 유지
  - TCP 메시지 번호는 `contracts/realtime/codes.json`에서 생성
  - API 설계 초안은 `docs/design/API_IMPLEMENTATION.md`에서 관리
- 입력 검증 수정
  - 에디터 갱신에서 건너뛰던 게임 입력 액션을 플레이 입력 갱신에서 검사
  - 합성 마우스 상태 → Input System 액션 → UI 레이캐스트·클릭 경로 확인
  - UI 입력 검사는 Direct3D 11 사용

## 검증 결과

| 검증 | 결과 |
| --- | --- |
| Unity 6000.3.11f1 컴파일 | 통과 |
| 씬 검사 | Login·Lobby·LegacyLobby·SoloDefense·ConnectionTest, 누락 스크립트 없음 |
| HTTP·인증 공통 계층 | 23개 통과 |
| 로그인·계정 API 클라이언트 | 45개 통과 |
| 로비 MVC·서비스 상태 | 22개 통과 |
| 로비 HTTP 어댑터 | 22개 통과 |
| 로비 마우스 입력 | 프로필·인벤토리 열기와 닫기 통과 |
| 솔로 디펜스 | 290개 통과, 전투·강화·중단·종료·로비 왕복 포함 |
| 실제 소켓 서버 통신 | 프레이밍·에코·매칭·취소·세션 분리·퇴장·재접속 통과 |
| TCP 생성 파일 | 공통 계약의 생성 결과와 일치 |
| 원본 에셋 GUID | 원본 `.meta` 2,248개 보존, 반영 후 중복 GUID 없음 |
| 문서·도구 | 현재 문서 상대 링크·PowerShell 구문·Git 공백 검사 통과 |

## 재실행

Unity 에디터를 종료한 뒤 저장소 루트에서 실행한다.

```powershell
./tools/Build.ps1 -Target Socket
./tools/Test.ps1 -Target Networking
./tools/Test.ps1 -Target Contracts
./tools/Test-Unity.ps1 -Gameplay
```

- 플레이 검증은 `.tools/unity-gameplay`의 별도 프로젝트에서 실행
- 로그·결과는 `artifacts/unity`에 저장, Git에서 제외
- 입력 검사 수정 후 실패했던 검사를 다시 실행해 통과 확인

## 검증 범위와 남은 항목

- HTTP 검증은 테스트용 서버 사용
  - 실제 API 서버에는 인증·로비·결과·보상 라우트가 아직 없음
  - 실제 계정 로그인·데이터 저장·보상 지급은 서버 구현 후 별도 통합 검증 필요
- TCP 검증은 임시 포트 사용
  - 서버 기본 포트 `30000`, 클라 초기값 `20000`은 실행 시 일치시켜야 함
- Unity 배치 검증 중 `UnityEditor.Search.SearchDatabase`의 검색 인덱스 초기화 예외 발생
  - 각 기능 검사는 종료 코드 0과 `PASS` 결과 확인
  - 에디터 검색 인덱스 문제는 별도 확인 필요
- Standalone 빌드와 사람의 수동 플레이는 이번 검증에 포함하지 않음
