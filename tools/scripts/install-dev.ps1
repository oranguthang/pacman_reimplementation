$ErrorActionPreference = "Stop"
Set-StrictMode -Version Latest

$repoRoot = Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
$createNesGameDir = Join-Path $repoRoot "tools\create-nes-game"
$createNesGameEntry = Join-Path $createNesGameDir "index.js"
$npmCacheDir = Join-Path $createNesGameDir ".npm-cache"
$downloadCacheDir = if ($env:CACHE_DIRECTORY) {
    $env:CACHE_DIRECTORY
} else {
    Join-Path $repoRoot "tools\create-nes-game-cache"
}
$createNesGameRepo = if ($env:CREATE_NES_GAME_REPO_URL) {
    $env:CREATE_NES_GAME_REPO_URL
} else {
    "https://github.com/igwgames/create-nes-game.git"
}

function Assert-Command {
    param([string]$Name)

    if (-not (Get-Command $Name -ErrorAction SilentlyContinue)) {
        throw "Required command not found: $Name"
    }
}

function Assert-LastExitCode {
    param([string]$Message)

    if ($LASTEXITCODE -ne 0) {
        throw $Message
    }
}

function Remove-IfExists {
    param(
        [string]$Path,
        [switch]$Recurse
    )

    if (Test-Path $Path) {
        if ($Recurse) {
            Remove-Item -LiteralPath $Path -Recurse -Force
        } else {
            Remove-Item -LiteralPath $Path -Force
        }
    }
}

Assert-Command "git"
Assert-Command "node"
Assert-Command "npm"

$env:CACHE_DIRECTORY = $downloadCacheDir
New-Item -ItemType Directory -Force -Path $downloadCacheDir | Out-Null

if (-not (Test-Path $createNesGameDir)) {
    if (Test-Path $createNesGameRepo) {
        Write-Host "Copying local create-nes-game checkout into tools/create-nes-game..."
        New-Item -ItemType Directory -Force -Path $createNesGameDir | Out-Null
        Get-ChildItem -Force $createNesGameRepo |
            Where-Object { $_.Name -notin @('.git', 'node_modules', 'dist', 'scratchpad') } |
            Copy-Item -Destination $createNesGameDir -Recurse -Force
    } else {
        Write-Host "Cloning create-nes-game into tools/create-nes-game..."
        git clone $createNesGameRepo $createNesGameDir
        Assert-LastExitCode "Failed cloning create-nes-game from $createNesGameRepo"
    }
} else {
    Write-Host "tools/create-nes-game already exists; skipping clone."
}

Push-Location $createNesGameDir
try {
    New-Item -ItemType Directory -Force -Path $npmCacheDir | Out-Null
    if (Test-Path "package-lock.json") {
        Write-Host "Installing create-nes-game npm dependencies with npm ci..."
        npm ci --cache $npmCacheDir
        Assert-LastExitCode "npm ci failed in $createNesGameDir"
    } else {
        Write-Host "Installing create-nes-game npm dependencies with npm install..."
        npm install --cache $npmCacheDir
        Assert-LastExitCode "npm install failed in $createNesGameDir"
    }
} finally {
    Pop-Location
}

Write-Host "Downloading project dependencies into tools/..."
node $createNesGameEntry download-dependencies
if ($LASTEXITCODE -ne 0) {
    $cc65Archive = Join-Path $downloadCacheDir "cc65-2.19-win.zip"
    $cc65ExtractDir = Join-Path $repoRoot "tools\cc65"

    if (Test-Path $cc65Archive) {
        Write-Warning "create-nes-game download-dependencies failed; removing cached cc65 archive and retrying once..."
        Remove-IfExists $cc65Archive
        Remove-IfExists $cc65ExtractDir -Recurse

        node $createNesGameEntry download-dependencies
    }
}
Assert-LastExitCode "create-nes-game download-dependencies failed"

Write-Host "Developer environment is ready."
