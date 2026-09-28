param([ValidateSet('Debug', 'Release')][string]$Configuration = 'Debug')
. "$PSScriptRoot/../../tools/LocalApi.ps1"
Invoke-WithLocalApi -Configuration $Configuration -Action {
    param($baseUrl)
    $spec = Invoke-RestMethod "$baseUrl/openapi/v1.json"
    if (!$spec.paths.'/api/connection'.get) { throw 'Connection endpoint is missing from OpenAPI.' }
    $status = 0
    try { $status = [int](Invoke-WebRequest "$baseUrl/api/connection" -UseBasicParsing -TimeoutSec 10).StatusCode }
    catch {
        if (!$_.Exception.Response) { throw }
        $status = [int]$_.Exception.Response.StatusCode
    }
    if ($status -ne 503) { throw "Expected HTTP 503 when the database is unavailable; got $status." }
    Write-Host 'API smoke checks passed: OpenAPI routing and database failure response.'
}
