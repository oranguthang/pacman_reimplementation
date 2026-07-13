$ErrorActionPreference = "Stop"
Set-StrictMode -Version Latest

# Idempotent local dev-environment setup.
#
# Why this doesn't just call `create-nes-game download-dependencies`:
# create-nes-game uses an old axios (0.25) that cannot tunnel HTTPS through an
# HTTP proxy (it sends an absolute-form GET instead of CONNECT), so every tool
# download fails with HTTP 405 behind a local proxy (e.g. HTTP_PROXY=127.0.0.1:1081).
# On top of that its retry logic DELETES tools/cc65 on failure, so a re-run wiped a
# working install. Here we download each tool ourselves with curl.exe (which tunnels
# HTTPS through a proxy correctly) and skip anything already installed. Re-running is
# safe and never deletes an existing tool.

$repoRoot          = Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
$toolsDir          = Join-Path $repoRoot "tools"
$createNesGameDir  = Join-Path $toolsDir "create-nes-game"
$npmCacheDir       = Join-Path $createNesGameDir ".npm-cache"
$cacheDir          = if ($env:CACHE_DIRECTORY) { $env:CACHE_DIRECTORY } else { Join-Path $toolsDir "create-nes-game-cache" }
$createNesGameRepo = if ($env:CREATE_NES_GAME_REPO_URL) { $env:CREATE_NES_GAME_REPO_URL } else { "https://github.com/igwgames/create-nes-game.git" }

function Assert-Command {
    param([string]$Name)
    if (-not (Get-Command $Name -ErrorAction SilentlyContinue)) {
        throw "Required command not found: $Name"
    }
}

# True only if the file is a complete, readable zip. Opening it reads the central
# directory, so a truncated/partial download (which can still start with "PK") fails.
function Test-ZipFile {
    param([string]$Path)
    try {
        Add-Type -AssemblyName System.IO.Compression.FileSystem -ErrorAction SilentlyContinue
        $zip = [System.IO.Compression.ZipFile]::OpenRead($Path)
        try { return ($zip.Entries.Count -gt 0) } finally { $zip.Dispose() }
    } catch { return $false }
}

# Ensure $cacheDir\$FileName exists, downloading it with curl.exe if needed.
# curl.exe honours HTTP(S)_PROXY and tunnels HTTPS via CONNECT, so it works where
# create-nes-game's axios does not. A cached file that is a corrupt/error-page zip
# is re-downloaded automatically. Returns the full path to the cached file.
function Get-CachedFile {
    param([string]$Url, [string]$FileName)

    $dest  = Join-Path $cacheDir $FileName
    $isZip = $FileName.ToLower().EndsWith(".zip")

    if (Test-Path $dest) {
        if ($isZip -and -not (Test-ZipFile $dest)) {
            Write-Host "    cached $FileName is not a valid zip; re-downloading"
            Remove-Item $dest -Force
        } else {
            Write-Host "    cache hit: $FileName"
            return $dest
        }
    }

    New-Item -ItemType Directory -Force -Path $cacheDir | Out-Null
    $tmp = "$dest.partial"
    if (Test-Path $tmp) { Remove-Item $tmp -Force }

    Write-Host "    downloading $FileName (via curl, proxy-safe)..."
    & curl.exe -fSL --retry 3 -o $tmp $Url
    if ($LASTEXITCODE -ne 0) {
        Remove-Item $tmp -Force -ErrorAction SilentlyContinue
        throw "Download failed: $Url  (if you are behind a proxy, make sure it allows this host)"
    }
    if ($isZip -and -not (Test-ZipFile $tmp)) {
        Remove-Item $tmp -Force
        throw "Downloaded $FileName is not a valid zip (got an error page instead of the file?)"
    }

    Move-Item -Force $tmp $dest
    return $dest
}

# Install a zip-packaged tool: skip if $Marker exists, else download+extract.
function Install-ZipTool {
    param([string]$Name, [string]$Marker, [string]$Url, [string]$ZipName, [string]$DestDir)
    if (Test-Path $Marker) { Write-Host "  [skip] $Name already installed"; return }
    Write-Host "  [install] $Name"
    $zip = Get-CachedFile -Url $Url -FileName $ZipName
    New-Item -ItemType Directory -Force -Path $DestDir | Out-Null
    Write-Host "    extracting to $DestDir"
    Expand-Archive -LiteralPath $zip -DestinationPath $DestDir -Force
    if (-not (Test-Path $Marker)) { throw "$Name extraction did not produce $Marker" }
}

# Install a single-file tool (bare executable): skip if $Marker exists.
function Install-FileTool {
    param([string]$Name, [string]$Marker, [string]$Url, [string]$CacheName)
    if (Test-Path $Marker) { Write-Host "  [skip] $Name already installed"; return }
    Write-Host "  [install] $Name"
    $src = Get-CachedFile -Url $Url -FileName $CacheName
    New-Item -ItemType Directory -Force -Path (Split-Path -Parent $Marker) | Out-Null
    Copy-Item -Force $src $Marker
}

# famitone2 ships inside create-nes-game itself; no network needed.
function Install-Famitone2 {
    $marker = Join-Path $toolsDir "famitone2\nsf2data.exe"
    if (Test-Path $marker) { Write-Host "  [skip] famitone2 already installed"; return }
    Write-Host "  [install] famitone2 (from bundled create-nes-game binaries)"
    $srcDir = Join-Path $createNesGameDir "tools\neslib"
    New-Item -ItemType Directory -Force -Path (Join-Path $toolsDir "famitone2") | Out-Null
    Copy-Item -Force (Join-Path $srcDir "nsf2data\nsf2data.exe") (Join-Path $toolsDir "famitone2\nsf2data.exe")
    Copy-Item -Force (Join-Path $srcDir "text2data\text2data.exe") (Join-Path $toolsDir "famitone2\text2data.exe")
}

# --- Prerequisites ---------------------------------------------------------
Assert-Command "git"
Assert-Command "node"
Assert-Command "npm"
Assert-Command "curl"   # curl.exe ships with Windows 10/11 and tunnels HTTPS through the proxy

# --- create-nes-game (clone + npm deps) ------------------------------------
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
        if ($LASTEXITCODE -ne 0) { throw "Failed cloning create-nes-game from $createNesGameRepo" }
    }
} else {
    Write-Host "tools/create-nes-game already exists; skipping clone."
}

if (-not (Test-Path (Join-Path $createNesGameDir "node_modules"))) {
    Push-Location $createNesGameDir
    try {
        New-Item -ItemType Directory -Force -Path $npmCacheDir | Out-Null
        if (Test-Path "package-lock.json") {
            Write-Host "Installing create-nes-game npm dependencies (npm ci)..."
            npm ci --cache $npmCacheDir
            if ($LASTEXITCODE -ne 0) { throw "npm ci failed in $createNesGameDir" }
        } else {
            Write-Host "Installing create-nes-game npm dependencies (npm install)..."
            npm install --cache $npmCacheDir
            if ($LASTEXITCODE -ne 0) { throw "npm install failed in $createNesGameDir" }
        }
    } finally {
        Pop-Location
    }
} else {
    Write-Host "create-nes-game npm dependencies already installed; skipping."
}

# --- Build/test tools (idempotent, proxy-safe) -----------------------------
Write-Host "Provisioning build tools into tools/ ..."

$config      = Get-Content (Join-Path $repoRoot ".create-nes-game.config.json") -Raw | ConvertFrom-Json
$nskToolsUrl = $config.extraDependencies[0].'win32-x64'

Install-ZipTool  -Name "cc65" `
    -Marker  (Join-Path $toolsDir "cc65\bin\ca65.exe") `
    -Url     "https://gde-files.nes.science/cc65-2.19-win.zip" `
    -ZipName "cc65-2.19-win.zip" `
    -DestDir (Join-Path $toolsDir "cc65")

Install-ZipTool  -Name "nes-starter-kit-tools" `
    -Marker  (Join-Path $toolsDir "nes-starter-kit-tools\tmx2c.exe") `
    -Url     $nskToolsUrl `
    -ZipName "nes-starter-kit-tools-win.zip" `
    -DestDir (Join-Path $toolsDir "nes-starter-kit-tools")

Install-Famitone2

Install-ZipTool  -Name "fceux" `
    -Marker  (Join-Path $toolsDir "emulators\fceux\fceux.exe") `
    -Url     "https://gde-files.nes.science/fceux-2.6.4-win32.zip" `
    -ZipName "fceux-2.6.4-win32.zip" `
    -DestDir (Join-Path $toolsDir "emulators\fceux")

Install-FileTool -Name "nes-test" `
    -Marker    (Join-Path $toolsDir "nes-test\nes-test.exe") `
    -Url       "https://gh.nes.science/nes-test/releases/download/v0.3.2/nes-test-win.exe" `
    -CacheName "nes-test-win.exe"

Write-Host "Developer environment is ready."
