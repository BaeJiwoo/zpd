# 모노레포 이전 기록

2026-09-28에 독립 저장소 3개를 루트 `main`에 통합했습니다.
원본 커밋은 다시 작성하거나 squash하지 않고 merge 부모로 연결했습니다.

| 기존 저장소 | 기준 브랜치 | 보존한 커밋 | 새 경로 |
| --- | --- | --- | --- |
| zpd-client | main | 4682039ddf0fbc1ae47624919c4f06ef61a279ec | client |
| zpd-server | session | 53d09781e6cd5a20599caa4275974ce75a215fab | socket-server |
| zpd-apiserver | main | 14e29f63d7a1f3251a5679479d4c6b55b8884f18 | api-server |

소켓 서버의 로컬 master는 session의 조상이며 16커밋 뒤에 있었습니다.
각 원본 브랜치는 `refs/archive/<새 경로>/heads/<브랜치>`로도 보존했습니다.
이전 태그가 있다면 `legacy/<새 경로>/` 접두사로 가져오도록 이전했습니다.

## 로컬 백업

`.migration-backup/`은 Git에서 제외됩니다.

- `client.bundle`, `socket-server.bundle`, `api-server.bundle`: 원본 refs와 그 refs에서 도달 가능한 커밋·Git 객체 백업. 미커밋 파일이나 로컬 설정은 포함하지 않습니다.
- `sources.json`: 원본 커밋·원격 주소·bundle SHA-256.
- `repositories/`: 원래 `.git`과 캐시·로컬 설정까지 포함한 세 원본 폴더.
- `removed-files/`: Git에 들어 있던 Word 임시 잠금 파일.

백업은 같은 디스크에 있으므로 별도의 외부 백업을 대신하지 않습니다.
필요하면 다음처럼 새 폴더에 원본을 복원할 수 있습니다. 현재 작업 폴더를 덮어쓰지 않습니다.

```powershell
git clone -b main .migration-backup/client.bundle ../zpd-client-restored
git clone -b session .migration-backup/socket-server.bundle ../zpd-server-restored
git clone -b main .migration-backup/api-server.bundle ../zpd-api-restored
```

## 새 작업 위치

Unity Hub는 `client`를 새 프로젝트 경로로 등록합니다. 캐시는 새 위치에서 다시 생성됩니다.
VS Code는 루트 `zpd.code-workspace`를 엽니다. 이전 CMake 캐시는 새 위치로 복사하지 않습니다.
API User Secrets ID는 동일하여 기존 개발 계정의 설정을 사용할 수 있습니다.
Protobuf DLL과 Unity 기존 에셋 GUID는 보존했습니다.

이전 당시에는 원격 origin을 설정하지 않았으며, 원래 GitHub 저장소 세 개는 변경하지 않았습니다.
현재 루트 origin은 [BaeJiwoo/zpd](https://github.com/BaeJiwoo/zpd)로 연결했습니다. 모노레포의 `main`은 이 저장소에 푸시합니다.
원격 CI 결과 확인과 브랜치 보호의 필수 CI 설정은 별도 절차입니다.
기존 이력은 main의 조상으로 포함되어 일반 main 푸시에도 함께 전달됩니다.
archive refs 자체와 로컬 백업 디렉터리는 일반 main 푸시에 포함되지 않습니다.
