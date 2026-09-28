param(
    [ValidateSet('All', 'Socket', 'Api')][string]$Target = 'All',
    [ValidateSet('Debug', 'Release')][string]$Configuration = 'Debug',
    [string]$Generator = 'Visual Studio 18 2026'
)
. "$PSScriptRoot/Common.ps1"

if ($Target -in @('All', 'Socket')) {
    if (!$env:VCPKG_ROOT -or !(Test-Path "$env:VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake")) {
        throw 'Set VCPKG_ROOT to your vcpkg installation directory.'
    }
    $socketRoot = Join-Path $RepoRoot 'socket-server'
    $buildRoot = Join-Path $socketRoot 'out/build/windows-x64'
    Invoke-Checked cmake @('-S', $socketRoot, '-B', $buildRoot, '-G', $Generator, '-A', 'x64',
        "-DCMAKE_TOOLCHAIN_FILE=$env:VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake", '-DVCPKG_TARGET_TRIPLET=x64-windows')
    Invoke-Checked cmake @('--build', $buildRoot, '--config', $Configuration, '--', '/p:VcpkgEnabled=false')
}
if ($Target -in @('All', 'Api')) {
    Invoke-Checked dotnet @('build', (Join-Path $RepoRoot 'api-server/Zpd.slnx'), '--configuration', $Configuration)
}
