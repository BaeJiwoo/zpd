# HTTP 계약

원본은 `api-server/src/Zpd.Api`의 실제 API 구현입니다.
`openapi.json`은 `./tools/Build.ps1 -Target Api` 후 `./tools/Export-OpenApi.ps1`로 생성합니다.
`-Check`는 실제 구현에서 다시 추출한 결과와 스냅샷을 비교합니다.
추출 과정은 임시 loopback 주소의 API를 실행하고 개발 DB를 사용하지 않습니다.
실행마다 달라지는 `servers` 주소만 제거합니다. 생성 파일을 직접 편집하지 않습니다.

현재 구현된 컨트롤러 라우트는 DB 연결 확인용 `GET /api/connection` 하나입니다.
로비·소셜·게임 결과·보상의 서버 처리는 미구현이며 [API 설계 초안](../../docs/design/API_IMPLEMENTATION.md)에 제안돼 있습니다.
솔로 클라에는 게임 결과·보상 전송 코드가 있지만 대응하는 서버 라우트는 없습니다. OpenAPI에 구현된 API처럼 등록하지 않습니다.

생성 명세는 코드의 응답 타입·메타데이터에 의존합니다. 현재 HTTP 200에는 본문 스키마가 없어
실제 성공 응답의 `status`, `database` 필드가 표현되지 않습니다. API 구현에 DTO와 응답 타입을 추가한 뒤 재생성해야 합니다.
`-Check`는 생성 결과와 파일이 같은지만 확인하므로, 응답 스키마의 누락이나 실제 업무 동작까지 검증하지 않습니다.
