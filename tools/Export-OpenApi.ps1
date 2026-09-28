param([ValidateSet('Debug', 'Release')][string]$Configuration = 'Debug', [switch]$Check)
. "$PSScriptRoot/LocalApi.ps1"
Invoke-WithLocalApi -Configuration $Configuration -Action {
    param($baseUrl)
    $spec = Invoke-RestMethod "$baseUrl/openapi/v1.json"
    # Host/port vary per execution; the contract describes routes independently of deployment.
    $spec.PSObject.Properties.Remove('servers')
    $expected = ($spec | ConvertTo-Json -Depth 100).Replace("`r`n", "`n").TrimEnd() + "`n"
    $path = Join-Path $RepoRoot 'contracts/http/openapi.json'
    if ($Check) {
        if (!(Test-Path $path)) { throw 'Missing OpenAPI snapshot. Run tools/Export-OpenApi.ps1.' }
        $actual = ((Get-Content $path -Raw | ConvertFrom-Json) | ConvertTo-Json -Depth 100).Replace("`r`n", "`n").TrimEnd() + "`n"
        if ($actual -cne $expected) { throw 'OpenAPI snapshot is stale. Run tools/Export-OpenApi.ps1.' }
    } else { Write-Utf8File $path $expected }
}
Write-Host $(if ($Check) { 'OpenAPI snapshot is current.' } else { 'OpenAPI snapshot exported.' })
