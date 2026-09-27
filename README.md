# Zpd API

ASP.NET Core 10 + EF Core 10 + MySQL 기반의 빈 API 프로젝트입니다.
도메인 모델, CRUD, 테이블, 초기 마이그레이션은 포함하지 않습니다.

## 준비

- .NET 10 SDK
- 실행 중인 MySQL 서버와 미리 생성된 `zpd` 데이터베이스 및 접근 계정

저장소 루트에서 실행합니다.

```powershell
dotnet restore
dotnet tool restore
dotnet user-secrets set "ConnectionStrings:DefaultConnection" "Server=localhost;Port=3306;Database=zpd;User ID=zpd;Password=YOUR_PASSWORD;" --project src/Zpd.Api
dotnet run --project src/Zpd.Api --launch-profile http
```

`appsettings.json`의 연결 문자열은 비밀번호가 없는 예시입니다.
실제 접속 정보는 개발 환경의 User Secrets 또는 환경 변수
`ConnectionStrings__DefaultConnection`으로 설정합니다. `.env` 파일은 자동 로드하지 않습니다.
운영 환경에서도 이 환경 변수로 접속 정보를 전달할 수 있습니다.

## 연결 확인

```powershell
Invoke-RestMethod http://localhost:5089/api/connection
```

- 성공: HTTP 200, `{"status":"ok","database":"connected"}`
- DB 연결 실패: HTTP 503, Problem Details 응답
- API 서버와 MySQL의 연결 여부만 확인하며, 테이블을 생성하거나 데이터를 변경하지 않습니다.

개발 환경의 OpenAPI JSON: <http://localhost:5089/openapi/v1.json>
요청 예시는 `src/Zpd.Api/Zpd.Api.http`에 있습니다.

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
현재 빈 프로젝트에서는 실행할 필요가 없습니다.

```powershell
dotnet ef migrations add InitialCreate --project src/Zpd.Api --output-dir Data/Migrations
dotnet ef database update --project src/Zpd.Api
```

서버 시작 시 마이그레이션을 자동 적용하지 않습니다.
공급자는 MySQL 공식 `MySql.EntityFrameworkCore`를 사용합니다.
HTTPS 개발 프로필은 `dotnet dev-certs https --trust` 후
`dotnet run --project src/Zpd.Api --launch-profile https`로 실행할 수 있습니다.
