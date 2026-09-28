param(
    [ValidateSet('All', 'Socket', 'Api', 'Networking', 'Contracts')][string]$Target = 'All',
    [ValidateSet('Debug', 'Release')][string]$Configuration = 'Debug'
)
. "$PSScriptRoot/Common.ps1"
$socketBuild = Join-Path $RepoRoot 'socket-server/out/build/windows-x64'
if ($Target -in @('All', 'Socket')) {
    Invoke-Checked ctest @('--test-dir', $socketBuild, '-C', $Configuration, '--output-on-failure')
}
if ($Target -in @('All', 'Networking')) {
    Invoke-Checked dotnet @('run', '--project', (Join-Path $RepoRoot 'client/Tools/Verification'),
        '--configuration', $Configuration, '--', (Join-Path $socketBuild "$Configuration/zpd-server.exe"))
}
if ($Target -in @('All', 'Contracts')) { & "$PSScriptRoot/Generate-Protocol.ps1" -Check }
if ($Target -in @('All', 'Api')) {
    & "$RepoRoot/api-server/tests/Smoke.ps1" -Configuration $Configuration
    & "$PSScriptRoot/Export-OpenApi.ps1" -Configuration $Configuration -Check
}
