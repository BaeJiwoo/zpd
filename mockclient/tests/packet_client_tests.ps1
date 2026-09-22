param([Parameter(Mandatory = $true)][string]$BinDirectory)

$ErrorActionPreference = 'Stop'
$processes = [System.Collections.Generic.List[System.Diagnostics.Process]]::new()

function Start-TestProcess([string]$Name, [int]$Port) {
    $start = [System.Diagnostics.ProcessStartInfo]::new((Join-Path $BinDirectory $Name), "$Port")
    $start.UseShellExecute = $false
    $start.CreateNoWindow = $true
    $start.RedirectStandardInput = $true
    $start.RedirectStandardOutput = $true
    $start.RedirectStandardError = $true
    $process = [System.Diagnostics.Process]::Start($start)
    $processes.Add($process)
    return $process
}

function Expect-Line($Process, [string]$Text) {
    $deadline = [DateTime]::UtcNow.AddSeconds(5)
    while ([DateTime]::UtcNow -lt $deadline) {
        $read = $Process.StandardOutput.ReadLineAsync()
        if (!$read.Wait(5000) -or $null -eq $read.Result) { break }
        if ($read.Result.Contains($Text)) { return }
    }
    throw "Missing output: $Text"
}

function Send-Command($Process, [string]$Text) {
    $Process.StandardInput.WriteLine($Text)
    $Process.StandardInput.Flush()
}

function Expect-Exit($Process, [int]$Code) {
    $output = $Process.StandardOutput.ReadToEndAsync()
    $errors = $Process.StandardError.ReadToEndAsync()
    if (!$Process.WaitForExit(5000)) { throw "Process failed to exit: $($Process.StartInfo.FileName) ($scenario)" }
    if ($Process.ExitCode -ne $Code) { throw "Unexpected exit code: $($Process.ExitCode)" }
}

function Read-Exact($Stream, [int]$Count) {
    $bytes = [byte[]]::new($Count)
    $offset = 0
    while ($offset -lt $Count) {
        $read = $Stream.Read($bytes, $offset, $Count - $offset)
        if ($read -eq 0) { throw 'Unexpected connection close' }
        $offset += $read
    }
    return ,$bytes
}

try {
    $reservation = [System.Net.Sockets.TcpListener]::new([System.Net.IPAddress]::Loopback, 0)
    $reservation.Start()
    $port = $reservation.LocalEndpoint.Port
    $reservation.Stop()
    $server = Start-TestProcess 'zpd-server.exe' $port
    Expect-Line $server 'server ready'
    $client = Start-TestProcess 'zpd-client.exe' $port
    Expect-Line $client 'Notifications arrive'
    Send-Command $client '/ping'
    Expect-Line $client 'Pong'
    Send-Command $client '/echo hello'
    Expect-Line $client 'Echo: hello'
    Send-Command $client '/echo'
    Expect-Line $client 'Echo: '
    $large = 'x' * 9000
    Send-Command $client "/echo $large"
    Expect-Line $client "Echo: $large"
    Send-Command $client '/ping'
    Expect-Line $client 'Pong'
    foreach ($unsupported in '/enter', '/create 2', '/join 1', '/members', '/move 1 2 3', '/positions', '/game 1', '/say hello', 'hello') {
        Send-Command $client $unsupported
        Expect-Line $client 'Unsupported command'
    }
    Send-Command $client '/status'
    Expect-Line $client 'state: idle'
    Send-Command $client '/cancel'
    Expect-Line $client 'server error 23'
    Send-Command $client '/match'
    Expect-Line $client 'Queued for matchmaking'
    Send-Command $client '/match'
    Expect-Line $client 'server error 23'
    Send-Command $client '/status'
    Expect-Line $client 'state: queued'
    Send-Command $client '/cancel'
    Expect-Line $client 'Matchmaking cancelled'
    Send-Command $client '/status'
    Expect-Line $client 'state: idle'
    Send-Command $client '/match'
    Expect-Line $client 'Queued for matchmaking'
    $second = Start-TestProcess 'zpd-client.exe' $port
    Expect-Line $second 'Notifications arrive'
    Send-Command $second '/match'
    Expect-Line $second 'Queued for matchmaking'
    Expect-Line $client 'Match found. Session: 1'
    Expect-Line $second 'Match found. Session: 1'
    Send-Command $client '/leave'
    Expect-Line $client 'Left session'
    Expect-Line $second 'Player left session'
    Send-Command $client '/status'
    Expect-Line $client 'state: idle'
    Send-Command $second '/leave'
    Expect-Line $second 'Left session'
    Send-Command $client '/match'
    Expect-Line $client 'Queued for matchmaking'
    Send-Command $second '/match'
    Expect-Line $second 'Queued for matchmaking'
    Expect-Line $client 'Match found. Session: 2'
    Expect-Line $second 'Match found. Session: 2'
    Send-Command $client '/quit'
    Expect-Exit $client 0
    Expect-Line $second 'Player left session'
    Send-Command $second '/ping'
    Expect-Line $second 'Pong'
    Send-Command $second '/quit'
    Expect-Exit $second 0
    Send-Command $server ''
    Expect-Exit $server 0

    # Only current packet-server replies are accepted.
    foreach ($scenario in 'legacy-ping', 'legacy-echo', 'bad-body', 'wrong-id', 'bad-echo', 'wrong-code') {
        $listener = [System.Net.Sockets.TcpListener]::new([System.Net.IPAddress]::Loopback, 0)
        $listener.Start()
        $peer = $null
        try {
            $client = Start-TestProcess 'zpd-client.exe' $listener.LocalEndpoint.Port
            $accept = $listener.AcceptTcpClientAsync()
            if (!$accept.Wait(5000)) { throw 'Client did not connect' }
            $peer = $accept.Result
            $stream = $peer.GetStream()
            $stream.ReadTimeout = 5000
            Expect-Line $client 'Notifications arrive'
            $echo = $scenario -in 'legacy-echo', 'bad-echo'
            if ($echo) { Send-Command $client '/echo hello' }
            else { Send-Command $client '/ping' }
            $header = Read-Exact $stream 8
            $body = Read-Exact $stream (($header[0] * 256 + $header[1]) - 8)
            switch ($scenario) {
                'legacy-ping' { $header[2] = 130 }
                'legacy-echo' { $header[2] = 129 }
                'bad-body' { $header[1] = 9; $body = [byte[]]@(120) }
                'wrong-id' { $header[7] = 0 }
                'bad-echo' { $body[$body.Length - 1] = 120 }
                'wrong-code' { $header[2] = 1 }
            }
            $stream.Write($header, 0, $header.Length)
            $stream.Write($body, 0, $body.Length)
            Expect-Exit $client 1
        } finally {
            if ($null -ne $peer) { $peer.Dispose() }
            $listener.Stop()
        }
    }
    Write-Output 'Packet client tests passed: ping, echo, matchmaking, cancel, leave, disconnect notifications, unsupported commands and malformed replies.'
} finally {
    foreach ($process in $processes) {
        if (!$process.HasExited) { $process.Kill(); $process.WaitForExit() }
        $process.Dispose()
    }
}
