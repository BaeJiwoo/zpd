# HTTP 계약

원본은 `api-server/src/Zpd.Api`의 실제 API 구현입니다.
`openapi.json`은 `./tools/Build.ps1 -Target Api` 후 `./tools/Export-OpenApi.ps1`로 생성합니다.
`-Check`는 실제 구현에서 다시 추출한 결과와 스냅샷을 비교합니다.
추출 과정은 임시 loopback 주소의 API를 실행하고 개발 DB를 사용하지 않습니다.
실행마다 달라지는 `servers` 주소만 제거합니다. 생성 파일을 직접 편집하지 않습니다.

현재 구현된 도메인 라우트는 `GET /api/connection` 하나입니다.
로비·소셜·게임 결과·보상은 [API 설계 초안](../../docs/design/API_IMPLEMENTATION.md)에만 있으며
OpenAPI에 확정 API처럼 등록하지 않습니다.
