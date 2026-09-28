. "$PSScriptRoot/Common.ps1"

# Start an isolated API process without accessing the developer's database.
function Invoke-WithLocalApi {
    param([string]$Configuration = 'Debug', [scriptblock]$Action)
    $apiRoot = Join-Path $RepoRoot 'api-server/src/Zpd.Api'
    $dll = Join-Path $apiRoot "bin/$Configuration/net10.0/Zpd.Api.dll"
    if (!(Test-Path $dll)) { throw 'Build the API first: tools/Build.ps1 -Target Api' }
    $listener = [Net.Sockets.TcpListener]::new([Net.IPAddress]::Loopback, 0)
    $listener.Start()
    $port = $listener.LocalEndpoint.Port
    $listener.Stop()
    $baseUrl = "http://127.0.0.1:$port"
    $startInfo = [Diagnostics.ProcessStartInfo]::new()
    $startInfo.FileName = (Get-Command dotnet).Source
    $startInfo.Arguments = '"{0}" --urls "{1}"' -f $dll, $baseUrl
    $startInfo.WorkingDirectory = $apiRoot
    $startInfo.UseShellExecute = $false
    $startInfo.CreateNoWindow = $true
    $startInfo.RedirectStandardOutput = $true
    $startInfo.RedirectStandardError = $true
    $startInfo.EnvironmentVariables['ASPNETCORE_ENVIRONMENT'] = 'Development'
    $startInfo.EnvironmentVariables['DOTNET_ENVIRONMENT'] = 'Development'
    $startInfo.EnvironmentVariables['ConnectionStrings__DefaultConnection'] = 'Server=127.0.0.1;Port=1;Database=zpd_validation;User ID=validation;Password=unused;Connection Timeout=1;'
    $process = [Diagnostics.Process]::Start($startInfo)
    $stdout = $process.StandardOutput.ReadToEndAsync()
    $stderr = $process.StandardError.ReadToEndAsync()
    try {
        $ready = $false
        $deadline = [DateTime]::UtcNow.AddSeconds(30)
        while ([DateTime]::UtcNow -lt $deadline) {
            if ($process.HasExited) { throw "API exited during startup: $($stderr.GetAwaiter().GetResult())" }
            try {
                $null = Invoke-WebRequest "$baseUrl/openapi/v1.json" -UseBasicParsing -TimeoutSec 2
                $ready = $true
                break
            } catch { Start-Sleep -Milliseconds 200 }
        }
        if (!$ready) { throw 'API startup timed out.' }
        & $Action $baseUrl
    } finally {
        if (!$process.HasExited) { $process.Kill() }
        $process.WaitForExit()
        $logRoot = Join-Path $RepoRoot ('.tools/api/' + [guid]::NewGuid().ToString('N'))
        Write-Utf8File "$logRoot/stdout.log" $stdout.GetAwaiter().GetResult()
        Write-Utf8File "$logRoot/stderr.log" $stderr.GetAwaiter().GetResult()
        $process.Dispose()
    }
}
