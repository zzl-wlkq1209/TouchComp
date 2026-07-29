$ErrorActionPreference = "Stop"

$solution = Join-Path $PSScriptRoot "..\TouchComp.sln"

$msbuildPath = $null
$msbuildCommand = Get-Command "MSBuild.exe" -ErrorAction SilentlyContinue
if ($msbuildCommand) {
    $msbuildPath = $msbuildCommand.Source
}

if (-not $msbuildPath) {
    $vswhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
    if (Test-Path -LiteralPath $vswhere) {
        $installPath = & $vswhere -latest -products * -requires Microsoft.Component.MSBuild -property installationPath
        if ($installPath) {
            $candidate = Join-Path $installPath "MSBuild\Current\Bin\MSBuild.exe"
            if (Test-Path -LiteralPath $candidate) {
                $msbuildPath = $candidate
            }
        }
    }
}

if (-not $msbuildPath) {
    throw "MSBuild was not found. Open a Developer PowerShell for Visual Studio or install Visual Studio Build Tools."
}

& $msbuildPath $solution /m /p:Configuration=Release /p:Platform=x64
exit $LASTEXITCODE
