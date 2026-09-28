param([string]$Protoc = '', [switch]$Check)
# Convenient entry point when working inside the Unity project.
& "$PSScriptRoot/../../tools/Generate-Protocol.ps1" -Protoc $Protoc -Check:$Check
