# Zpd API

ASP.NET Core 10 + EF Core 10 + MySQL 기반의 초기 API 프로젝트입니다.
DB 연결 확인 라우트만 구현돼 있습니다. 도메인 모델, CRUD, 테이블, 초기 마이그레이션과 인증은 아직 없습니다.

새 API는 [API 라우팅 추가 방법](../docs/guides/API_ROUTING.md)을 참고해서 구현합니다.

## 준비

- .NET 10 SDK
- DB 연결 성공을 확인하려면 실행 중인 MySQL 서버와 접속 가능한 DB/계정이 필요합니다. 아래 예시는 `zpd`를 사용합니다.

API 실행과 기존 스모크 테스트에는 정상 연결되는 DB가 필수는 아닙니다. DB 연결에 실패하면 연결 확인 API가 HTTP 503을 반환합니다.

아래 명령은 모노레포의 `api-server` 폴더에서 실행합니다. 전체 빌드·검증은 [루트 README](../README.md)를 참고하세요.

```powershell
dotnet user-secrets set "ConnectionStrings:DefaultConnection" "Server=localhost;Port=3306;Database=zpd;User ID=zpd;Password=YOUR_PASSWORD;" --project src/Zpd.Api
dotnet run --project src/Zpd.Api --launch-profile http
```

`dotnet run`에서 패키지 복원과 빌드를 함께 수행합니다.
`appsettings.json`의 연결 문자열은 비밀번호가 없는 예시입니다.
실제 접속 정보는 개발 환경의 User Secrets 또는 환경 변수
`ConnectionStrings__DefaultConnection`으로 설정합니다. `.env` 파일은 자동 로드하지 않습니다.
운영 환경에서도 이 환경 변수로 접속 정보를 전달할 수 있습니다.

## VS Code 개발

`api-server`에서 `code .`를 실행하거나 루트의 `zpd.code-workspace`를 엽니다.
권장 확장인 **C# Dev Kit**과 **REST Client**를 설치합니다.
DB 연결 성공을 확인할 경우 위의 User Secrets를 먼저 설정합니다. `api-server`를 단독으로 열었을 때의 단축키는 다음과 같습니다.

- `F5`: `Zpd.Api (HTTP)` 구성으로 빌드 및 디버깅 시작
- `Shift+F5`: 디버깅 종료
- `Ctrl+Shift+B`: 솔루션 빌드
- `src/Zpd.Api/Zpd.Api.http`의 **Send Request**: 연결 테스트 요청 전송

루트 워크스페이스에서는 API 폴더의 디버깅 구성을 선택합니다. 루트 기본 빌드 작업인 `ZPD: Build servers`는 두 서버를 빌드하므로 API 단독 빌드와 구분합니다.
디버깅은 `launchSettings.json`의 `http` 프로필을 사용합니다.
API 주소는 `http://localhost:5089`입니다.
컨트롤러에 중단점을 설정하고 요청하면 해당 위치에서 실행이 멈춥니다.

## 연결 확인

```powershell
Invoke-RestMethod http://localhost:5089/api/connection
```

- 성공: HTTP 200, `{"status":"ok","database":"connected"}`
- DB 연결 실패: HTTP 503, Problem Details 응답
- API 서버와 MySQL의 연결 여부만 확인하며, 테이블을 생성하거나 데이터를 변경하지 않습니다.

개발 환경의 OpenAPI JSON: <http://localhost:5089/openapi/v1.json>
요청 예시는 `src/Zpd.Api/Zpd.Api.http`에 있습니다.
현재 OpenAPI의 HTTP 200 응답에는 성공 본문의 스키마가 누락돼 있습니다. DTO와 응답 타입을 명시하는 보완이 필요합니다.
스모크 테스트는 연결 확인 라우트의 명세 노출과 HTTP 503만 확인하며, 정상 DB 연결이나 응답 본문까지 검사하지는 않습니다.

## 구조

```text
src/Zpd.Api/
  Controllers/ConnectionController.cs  # 유일한 연결 테스트 API
  Data/AppDbContext.cs                 # 빈 EF Core DbContext
  Properties/launchSettings.json       # 로컬 실행 설정
  Program.cs                          # DI, MySQL, OpenAPI, 오류 응답
  appsettings.json                    # 기본 설정
```

## 모델 추가 이후

엔티티와 DbSet을 추가한 뒤 마이그레이션을 생성하고 적용합니다.
현재 엔티티가 없는 상태에서는 실행할 필요가 없습니다.
다음 명령은 `api-server` 폴더에서 첫 마이그레이션을 추가하는 예시입니다. 이후에는 변경 내용을 나타내는 이름을 사용합니다.
적용 전에 대상 연결 문자열과 생성된 마이그레이션을 확인합니다.

```powershell
dotnet tool restore
dotnet ef migrations add InitialCreate --project src/Zpd.Api --output-dir Data/Migrations
dotnet ef database update --project src/Zpd.Api
```

서버 시작 시 마이그레이션을 자동 적용하지 않습니다.
공급자는 MySQL 공식 `MySql.EntityFrameworkCore`를 사용합니다.
HTTPS 개발 프로필은 `dotnet dev-certs https --trust` 후
`dotnet run --project src/Zpd.Api --launch-profile https`로 실행할 수 있습니다.
