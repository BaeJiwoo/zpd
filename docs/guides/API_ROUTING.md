# API 라우팅 추가 방법

현재 API는 ASP.NET Core 컨트롤러 방식입니다. 구현된 컨트롤러 라우트는 `GET /api/connection` 하나입니다.
개발 환경에서는 명세를 조회하는 `GET /openapi/v1.json`도 노출합니다.
아래 아이템 경로와 파일명은 새 기능을 추가할 때의 예시이며, 아이템 API는 아직 구현되지 않았습니다.
명령은 별도 안내가 없으면 저장소 루트에서 실행합니다.

## 1. 요청부터 정하기

[공통 작업 순서](FEATURE_WORKFLOW.md)에 맞춰 다음 내용을 먼저 정합니다.

| 항목 | 정할 내용 |
| --- | --- |
| 호출 주체 | 게임 클라, 기획 툴, 서버 내부 호출 중 누구인지 |
| 라우트 | URL, HTTP 메서드, 경로·쿼리·본문에 들어갈 값 |
| 권한 | 누가 조회하거나 수정할 수 있는지 |
| 데이터 | 읽거나 쓸 테이블과 해당 데이터의 쓰기 주체 |
| 응답 | 성공 상태 코드와 DTO, 실패 상태 코드와 오류 내용 |
| 재시도 | 같은 요청을 다시 보내도 되는지, 중복 처리를 어떻게 막는지 |
| 반영 시점 | 저장한 값을 클라·소켓 서버가 언제 읽어 적용하는지 |

게임용 API의 `/api/v1` 경로는 [설계 초안](../design/API_IMPLEMENTATION.md)의 제안입니다.
새 기능을 구현할 때 경로와 버전 정책을 확정합니다. 기존 연결 확인 경로는 그대로 둡니다.
기획 툴의 관리용 경로는 게임 유저용 경로와 권한을 구분해서 정합니다.

## 2. 컨트롤러에 라우트 추가하기

기준 파일은 [ConnectionController.cs](../../api-server/src/Zpd.Api/Controllers/ConnectionController.cs)입니다.
`Controllers/<기능명>Controller.cs`를 만들고 `ControllerBase`를 상속합니다.
`[ApiController]`, `[Route(...)]`, HTTP 메서드 특성으로 요청을 연결합니다.

예를 들어 컨트롤러에 `[Route("api/v1/items")]`, 액션에 `[HttpGet("{itemId}")]`를 붙이면
`GET /api/v1/items/{itemId}`가 됩니다. 같은 URL과 HTTP 메서드가 중복되지 않게 합니다.
경로 값은 `[FromRoute]`, 조회 조건은 `[FromQuery]`, JSON 요청은 `[FromBody]`로 구분합니다.
[ASP.NET Core 라우팅 문서](https://learn.microsoft.com/en-us/aspnet/core/mvc/controllers/routing?view=aspnetcore-10.0)

[Program.cs](../../api-server/src/Zpd.Api/Program.cs)에 `AddControllers()`와 `MapControllers()`가 이미 있습니다.
같은 API 프로젝트에 컨트롤러를 추가할 때 라우트마다 `Program.cs`에 별도 매핑을 넣을 필요는 없습니다.

## 3. 요청과 응답 DTO 정하기

DB 엔티티와 외부로 주고받는 DTO를 구분합니다.
필요해지면 `api-server/src/Zpd.Api/Contracts/<기능명>/`에 요청·응답 DTO를 추가합니다.
이 C# 폴더는 API 내부 타입을 두는 위치이고, 루트 `contracts/http`는 생성된 외부 명세를 두는 위치입니다.

필수 값, 길이, 숫자 범위 같은 입력 조건을 정의합니다.
`[ApiController]`의 모델 검증 오류는 기본적으로 HTTP 400을 반환합니다.
소유권, 아이템 존재 여부, 변경 가능한 상태 같은 업무 조건은 별도로 검사합니다.
현재 오류 응답은 `ProblemDetails`를 사용합니다. 초안에 있는 별도 오류 형식을 도입하려면 공통 계약부터 맞춥니다.
[ASP.NET Core Web API 문서](https://learn.microsoft.com/en-us/aspnet/core/web-api/?view=aspnetcore-10.0)

반환 타입과 `[ProducesResponseType]`에 실제 성공·실패 응답을 표현해 OpenAPI에 반영합니다.
성공 DTO가 있으면 `ActionResult<ItemResponse>` 또는 `[ProducesResponseType<ItemResponse>(StatusCodes.Status200OK)]`처럼 타입을 명시합니다.
`ItemResponse`는 예시 이름이며 실제 필드와 DTO를 먼저 정의합니다.
현재 연결 확인 API는 익명 객체와 `IActionResult`를 반환하고 성공 상태 코드만 선언하므로,
생성 명세에 HTTP 200의 `status`, `database` 필드가 빠져 있습니다. 라우트 연결 방식만 참고하고 이 누락을 그대로 따르지 않습니다.
이 특성은 명세용 정보이며 실제 응답 본문이나 권한 검사를 대신하지 않습니다.
[컨트롤러 반환 타입 문서](https://learn.microsoft.com/en-us/aspnet/core/web-api/action-return-types?view=aspnetcore-10.0)

빈 값이나 임시 데이터로 미구현 기능을 성공 처리하지 않습니다.

## 4. 처리 로직과 서비스 연결하기

컨트롤러는 요청을 받고 결과를 HTTP 응답으로 바꾸는 일을 맡습니다.
데이터 변경 규칙이나 여러 단계의 처리가 생기면 `Services/<기능명>Service.cs`로 분리합니다.
서비스를 추가한 경우에만 `Program.cs`에 필요한 DI 등록을 추가하고 컨트롤러에서 주입받습니다.
예를 들어 `ItemService`를 실제로 만든 뒤 `builder.Services.AddScoped<ItemService>();`로 등록할 수 있습니다.

현재 인증 서비스와 권한 정책은 없습니다. 권한이 필요한 라우트는 인증 설정·미들웨어·정책을 함께 구현해야 합니다.
URL을 관리용으로 나누는 것만으로 권한이 제한되지는 않습니다. 인증 주체, 접근 정책과 거절 응답을 함께 검증합니다.
기획 툴용 아이템 수정 권한을 일반 게임 유저 권한과 같게 취급하지 않습니다.

## 5. DB 변경 연결하기

기존 [AppDbContext.cs](../../api-server/src/Zpd.Api/Data/AppDbContext.cs)를 사용합니다.
지금은 엔티티와 `DbSet`이 없으므로, 저장이 필요한 기능부터 모델·제약 조건·마이그레이션을 추가합니다.
구체적인 생성 명령은 [API README](../../api-server/README.md#모델-추가-이후)를 참고합니다.

소켓 서버도 같은 DB를 사용할 예정이므로, 테이블을 바꾸기 전에 어느 서버가 읽고 쓰는지 확인합니다.
스키마 변경은 하나의 마이그레이션 이력으로 관리하고, 적용할 DB와 기존 데이터 영향을 확인합니다.
서로 함께 성공해야 하는 변경, 같은 요청의 중복 저장, 동시 수정 충돌도 이 단계에서 다룹니다.
DB 작업에는 요청의 `CancellationToken`을 전달합니다.
[EF Core 마이그레이션 문서](https://learn.microsoft.com/en-us/ef/core/managing-schemas/migrations/)

## 6. 호출 예시와 테스트 추가하기

[Zpd.Api.http](../../api-server/src/Zpd.Api/Zpd.Api.http)에 실제 요청 예시를 추가합니다.
정상 요청뿐 아니라 잘못된 입력, 없는 대상, 권한 거절, 필요한 경우 중복·동시 요청을 검증합니다.

현재 [Smoke.ps1](../../api-server/tests/Smoke.ps1)은 OpenAPI에 연결 확인 라우트가 있는지와 DB 연결 실패 시 HTTP 503인지만 확인합니다.
DB 연결 성공 응답, 실패 응답 본문의 필드, 도메인 데이터 변경은 검사하지 않습니다.
새 기능의 동작을 자동으로 검증해 주지는 않습니다.
테스트를 추가하면 [tools/Test.ps1](../../tools/Test.ps1)의 API 대상에서 실제로 실행되도록 연결합니다.
DB가 필요한 테스트는 별도 테스트 DB와 준비·정리 절차를 둡니다.
기존 스모크 테스트의 실패용 DB 주소를 개발 DB 주소로 바꾸지 않습니다.

## 7. 빌드하고 명세 갱신하기

```powershell
./tools/Build.ps1 -Target Api
./tools/Export-OpenApi.ps1
./tools/Test.ps1 -Target Api
```

API 구현이 명세의 원본입니다. 생성된 [openapi.json](../../contracts/http/openapi.json)을 직접 수정하지 않습니다.
`Test.ps1 -Target Api`는 API 스모크와 OpenAPI 일치 검사를 실행합니다.
OpenAPI 경로·필드·응답이 의도대로 바뀌었는지 diff를 보고, 호출하는 클라나 툴도 같은 계약으로 맞춥니다.
생성 결과가 같다는 것만으로 명세가 완전하다고 판단하지 않습니다. 실제 응답과 필드·타입·필수 여부를 대조합니다.
명세 추출도 API 프로세스를 실행하므로 서버 시작 시 DB 마이그레이션이나 데이터 조회를 강제하는 변경은
DB 없이 명세를 추출하는 현재 절차에 영향을 줍니다. 그런 변경이 필요하면 추출·검증 절차도 함께 수정합니다.

## 완료 기준

- 합의한 URL과 HTTP 메서드로 요청이 들어옵니다.
- 입력·권한·상태 검증과 성공·실패 응답이 동작합니다.
- 필요한 데이터 변경과 중복·충돌 처리를 검증했습니다.
- 호출하는 쪽에서 결과와 실패를 구분해 반영합니다.
- 새 기능 테스트가 실제 검증 명령에 포함돼 있습니다.
- API 구현, 요청 예시, 생성된 명세, 관련 가이드를 함께 갱신했습니다.
