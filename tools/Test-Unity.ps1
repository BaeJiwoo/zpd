param([string]$Unity = '', [int]$TimeoutSeconds = 900)
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
