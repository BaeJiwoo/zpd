param(
    [Parameter(Mandatory)][ValidateSet('Socket', 'Api', 'MockClient')][string]$Target,
    [ValidateSet('Debug', 'Release')][string]$Configuration = 'Debug'
)
. "$PSScriptRoot/Common.ps1"
if ($Target -eq 'Api') {
    Invoke-Checked dotnet @('run', '--project', (Join-Path $RepoRoot 'api-server/src/Zpd.Api'),
        '--configuration', $Configuration, '--no-build', '--launch-profile', 'http')
} else {
    $name = if ($Target -eq 'Socket') { 'zpd-server.exe' } else { 'zpd-client.exe' }
    $executable = Join-Path $RepoRoot "socket-server/out/build/windows-x64/$Configuration/$name"
    if (!(Test-Path $executable)) { throw 'Build the socket server first: tools/Build.ps1 -Target Socket' }
    Invoke-Checked $executable @()
}
