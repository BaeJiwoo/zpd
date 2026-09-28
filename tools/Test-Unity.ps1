param([string]$Unity = '', [int]$TimeoutSeconds = 900, [switch]$Gameplay)
. "$PSScriptRoot/Common.ps1"
if (!$Unity) {
    $versionLine = Get-Content "$RepoRoot/client/ProjectSettings/ProjectVersion.txt" | Select-Object -First 1
    $version = $versionLine.Split(':')[1].Trim()
    $Unity = "C:/Program Files/Unity/Hub/Editor/$version/Editor/Unity.exe"
}
if (!(Test-Path -LiteralPath $Unity)) { throw 'Install the pinned Unity editor, or supply -Unity.' }
$log = Join-Path $RepoRoot 'artifacts/unity/validation.log'
[IO.Directory]::CreateDirectory((Split-Path $log -Parent)) | Out-Null
$arguments = '-batchmode -nographics -quit -projectPath "{0}" -executeMethod Zpd.Editor.ProjectValidation.Run -logFile "{1}"' -f (Join-Path $RepoRoot 'client'), $log
$process = Start-Process -FilePath $Unity -ArgumentList $arguments -WindowStyle Hidden -PassThru
if (!$process.WaitForExit($TimeoutSeconds * 1000)) {
    $process.Kill()
    throw "Unity validation timed out. See $log"
}
if ($process.ExitCode -ne 0 -or !(Select-String -LiteralPath $log -Pattern 'ZPD_VALIDATION_PASSED' -Quiet)) {
    throw "Unity validation failed. See $log"
}
Write-Host "Unity compilation and scene validation passed. Log: $log"

if (!$Gameplay) { return }

# Run the imported play-mode checks in an isolated copy of the authored assets.
$clientRoot = Join-Path $RepoRoot 'client'
$validationRoot = Join-Path $RepoRoot ('.tools/unity-gameplay/' + [guid]::NewGuid().ToString('N'))
[IO.Directory]::CreateDirectory($validationRoot) | Out-Null
Copy-Item -LiteralPath "$clientRoot/Assets" -Destination "$validationRoot/Assets" -Recurse
Copy-Item -LiteralPath "$clientRoot/ProjectSettings" -Destination "$validationRoot/ProjectSettings" -Recurse
[IO.Directory]::CreateDirectory("$validationRoot/Packages") | Out-Null
$manifest = Get-Content "$clientRoot/Packages/manifest.json" -Raw | ConvertFrom-Json
foreach ($package in Get-ChildItem "$clientRoot/Library/PackageCache" -Directory) {
    $packageJson = Join-Path $package.FullName 'package.json'
    if (Test-Path -LiteralPath $packageJson) {
        $info = Get-Content $packageJson -Raw | ConvertFrom-Json
        $manifest.dependencies | Add-Member -MemberType NoteProperty -Name $info.name -Value ('file:' + $package.FullName.Replace('\', '/')) -Force
    }
}
Write-Utf8File "$validationRoot/Packages/manifest.json" ($manifest | ConvertTo-Json -Depth 10)
Copy-Item "$clientRoot/Tools/LobbyValidation/*.cs" "$validationRoot/Assets/Editor/"
Copy-Item -LiteralPath "$clientRoot/Tools/DefenseValidation/DefenseGameplayChecks.cs" -Destination "$validationRoot/Assets/Editor/DefenseGameplayChecks.cs"

$checks = @(
    @{ Name = 'networking'; Method = 'NetworkingChecks.Run'; Result = 'networking-result.txt' }
    @{ Name = 'login'; Method = 'LoginChecks.Run'; Result = 'login-result.txt' }
    @{ Name = 'lobby'; Method = 'LobbyChecks.Run'; Result = 'validation-result.txt' }
    @{ Name = 'lobby-api'; Method = 'LobbyApiChecks.Run'; Result = 'api-result.txt' }
    @{ Name = 'lobby-pointer'; Method = 'LobbyPointerChecks.Run'; Result = 'pointer-result.txt' }
    @{ Name = 'defense'; Method = 'Zpd.Defense.Editor.DefenseGameplayChecks.Run'; Result = 'validation-result.txt' }
)
Write-Host "Gameplay validation project: $validationRoot"
foreach ($check in $checks) {
    $checkLog = Join-Path $RepoRoot "artifacts/unity/$($check.Name).log"
    $resultPath = Join-Path $validationRoot $check.Result
    Write-Utf8File $resultPath 'RUNNING'
    $graphics = if ($check.Name -eq 'lobby-pointer') { '-force-d3d11' } else { '-nographics' }
    $arguments = '-batchmode {0} -projectPath "{1}" -executeMethod {2} -logFile "{3}"' -f $graphics, $validationRoot, $check.Method, $checkLog
    $process = Start-Process -FilePath $Unity -ArgumentList $arguments -WorkingDirectory $validationRoot -WindowStyle Hidden -PassThru
    if (!$process.WaitForExit($TimeoutSeconds * 1000)) {
        $process.Kill()
        throw "Unity $($check.Name) validation timed out. See $checkLog"
    }
    $result = Get-Content -LiteralPath $resultPath -Raw
    Write-Utf8File (Join-Path $RepoRoot "artifacts/unity/$($check.Name)-result.txt") $result
    if ($process.ExitCode -ne 0 -or !$result.StartsWith('PASS:')) {
        throw "Unity $($check.Name) validation failed: $result See $checkLog"
    }
    Write-Host $result.Trim()
}
