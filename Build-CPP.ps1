#usage -----> powershell -ExecutionPolicy Bypass -Command "& {. .\Build-CPP.ps1; Dist -major 0 -minor 0 -patch 3}"
$msbuild = "C:\Program Files\Microsoft Visual Studio\2022\Community\Msbuild\Current\Bin\MSBuild.exe"

function Add-RMSkinFooter {
    param(
        [Parameter(Mandatory = $true)][string]$ZipPath
    )

    if (!(Test-Path $ZipPath)) {
        throw "ZIP file not found: $ZipPath"
    }

    $zipSize = [long](Get-Item $ZipPath).Length

    try {
        # Use the GitHub Actions method that works
        $sizeBytes = [System.BitConverter]::GetBytes($zipSize)
        Add-Content -Path $ZipPath -Value $sizeBytes -Encoding Byte

        $flags = [byte]0
        Add-Content -Path $ZipPath -Value $flags -Encoding Byte

        $rmskin = [string]"RMSKIN`0"
        Add-Content -Path $ZipPath -Value $rmskin -NoNewLine -Encoding ASCII

        Write-Host "Added RMSKIN footer using GitHub Actions method" -ForegroundColor Green
        return
    }
    catch {
        Write-Host "GitHub Actions method failed, trying FileStream..." -ForegroundColor Yellow
    }

    try {
        $fileStream = [System.IO.File]::OpenWrite($ZipPath)
        $fileStream.Seek(0, [System.IO.SeekOrigin]::End)

        $sizeBytes = [System.BitConverter]::GetBytes($zipSize)
        $fileStream.Write($sizeBytes, 0, $sizeBytes.Length)

        $flagByte = [byte]0
        $fileStream.WriteByte($flagByte)

        $rmskinBytes = [System.Text.Encoding]::ASCII.GetBytes("RMSKIN`0")
        $fileStream.Write($rmskinBytes, 0, $rmskinBytes.Length)

        $fileStream.Close()
        $fileStream.Dispose()

        Write-Host "Added RMSKIN footer using FileStream method" -ForegroundColor Green
    }
    catch {
        if ($fileStream) {
            $fileStream.Close()
            $fileStream.Dispose()
        }
        throw "Failed to add RMSKIN footer: $_"
    }
}

function BumpVersion {
    param(
        [Parameter(Mandatory = $true)][string]$ver
    )

    $skinDefPath = Join-Path (Get-Location).Path "Resources\skin_definition.json"
    
    if (!(Test-Path $skinDefPath)) {
        throw "skin_definition.json not found at: $skinDefPath"
    }

    $skinDef = Get-Content $skinDefPath -Raw | ConvertFrom-Json

    $skinDef.version = $ver

    ($skinDef | ConvertTo-Json -Depth 10) | Set-Content $skinDefPath -Encoding UTF8

    Write-Host "Bumped skin_definition.json to version $ver" -ForegroundColor Green
}

function New-Package {
    param(
        [Parameter(Mandatory = $true)][string]$RootConfig,
        [string]$OutPath,
        [string]$OutDirectory,
        [string]$OutFile,
        [switch]$Quiet
    )

    $skinDefPath = Join-Path (Get-Location).Path "Resources\skin_definition.json"
    
    if (!(Test-Path $skinDefPath)) {
        throw "skin_definition.json not found at: $skinDefPath"
    }

    $skinDef = Get-Content $skinDefPath -Raw | ConvertFrom-Json

    $pwdRoot = (Get-Location).Path
    $skinDir = if ($skinDef.skinDir) { Join-Path $pwdRoot $skinDef.skinDir } else { Join-Path $pwdRoot "." }
    if (!(Test-Path $skinDir)) {
        throw "skinDir '$skinDir' does not exist."
    }

    $temp = Join-Path ([System.IO.Path]::GetTempPath()) ([System.Guid]::NewGuid().ToString())
    New-Item -ItemType Directory -Path $temp -Force | Out-Null

    if (-not $Quiet) { Write-Host "Using temp directory: $temp" -ForegroundColor Gray }

    $iniLines = @()
    $iniLines += "[rmskin]"
    if ($skinDef.name) { $iniLines += "Name=$($skinDef.name)" }
    if ($skinDef.version) { $iniLines += "Version=$($skinDef.version)" }
    if ($skinDef.author) { $iniLines += "Author=$($skinDef.author)" }
    if ($skinDef.minimumVersion) { $iniLines += "MinimumVersion=$($skinDef.minimumVersion)" }
    if ($skinDef.minimumWindows) { $iniLines += "MinimumWindows=$($skinDef.minimumWindows)" }
    if ($skinDef.load) { $iniLines += "Load=$($skinDef.load)" }
    if ($skinDef.loadType) { $iniLines += "LoadType=$($skinDef.loadType)" }

    foreach ($prop in $skinDef.PSObject.Properties.Name) {
        if ($prop -in @("name","version","author","minimumVersion","minimumWindows","load","loadType","plugins","skinDir","output","headerImage","variableFiles","configPrefix")) { continue }
        $val = $skinDef.$prop
        if ($val) { $iniLines += "$prop=$val" }
    }
    $iniContent = $iniLines -join "`r`n"
    $iniPath = Join-Path $temp "RMSKIN.ini"
    # Use ASCII encoding without trailing newline like GitHub Actions
    $iniContent | Set-Content -Path $iniPath -Encoding ASCII -NoNewline

    if (-not $Quiet) { Write-Host "Created RMSKIN.ini" -ForegroundColor Cyan }

    $skinstemp = Join-Path $temp "Skins"
    New-Item -ItemType Directory -Path $skinstemp -Force | Out-Null
    $destSkinRoot = Join-Path $skinstemp $RootConfig
    New-Item -ItemType Directory -Path $destSkinRoot -Force | Out-Null

    $rootConfigPath = Join-Path $skinDir $RootConfig
    try {
        $rootConfigPath = [System.IO.Path]::GetFullPath($rootConfigPath)
    } catch {

        $cleanRoot = ($RootConfig -replace '^[.][\\/]+','')
        $rootConfigPath = Join-Path $skinDir $cleanRoot
    }

    if (!(Test-Path $rootConfigPath)) {
        throw "RootConfig path '$rootConfigPath' not found."
    }

    $excluded = @(".git", ".gitignore")

    $itemsToCopy = Get-ChildItem -Path $rootConfigPath -Recurse | Where-Object { 
        $relativePath = $_.FullName.Substring($rootConfigPath.Length + 1)
        $shouldExclude = $false
        foreach ($exclude in $excluded) {
            if ($relativePath -like "*$exclude*" -or $_.Name -eq $exclude) {
                $shouldExclude = $true
                break
            }
        }
        -not $shouldExclude
    }

    foreach ($item in $itemsToCopy) {
        $relativePath = $item.FullName.Substring($rootConfigPath.Length + 1)
        $destinationPath = Join-Path $destSkinRoot $relativePath
        $destinationDir = Split-Path $destinationPath -Parent

        if (!(Test-Path $destinationDir)) {
            New-Item -ItemType Directory -Path $destinationDir -Force | Out-Null
        }

        if (-not $item.PSIsContainer) {
            Copy-Item -Path $item.FullName -Destination $destinationPath -Force
        }
    }

    if (-not $Quiet) { Write-Host "Copied skin files from $rootConfigPath" -ForegroundColor Cyan }

    $pluginsTemp = Join-Path $temp "Plugins"
    $pluginsTemp32 = Join-Path $pluginsTemp "32bit"
    $pluginsTemp64 = Join-Path $pluginsTemp "64bit"
    New-Item -ItemType Directory -Path $pluginsTemp -Force | Out-Null
    New-Item -ItemType Directory -Path $pluginsTemp32 -Force | Out-Null
    New-Item -ItemType Directory -Path $pluginsTemp64 -Force | Out-Null

    if ($skinDef.plugins) {
        if (-not $Quiet) { Write-Host "Collecting plugins..." -ForegroundColor Cyan }
        foreach ($plugin in $skinDef.plugins) {

            $x32path = $plugin.x32
            $x64path = $plugin.x64
            if ($x32path) {

                if ($x32path -notmatch '^[A-Za-z]:\\') {
                    $x32path = Join-Path $pwdRoot $x32path
                }
                if (Test-Path $x32path) {

                    if ((Get-Item $x32path).PSIsContainer) {
                        Get-ChildItem -Path $x32path -Filter *.dll -Recurse | ForEach-Object {
                            Copy-Item -Path $_.FullName -Destination $pluginsTemp32 -Force
                        }
                    } else {
                        Copy-Item -Path $x32path -Destination $pluginsTemp32 -Force
                    }
                    if (-not $Quiet) { Write-Host "Copied x32 plugin from $x32path" }
                } else {
                    if (-not $Quiet) { Write-Host "x32 plugin path not found: $x32path" -ForegroundColor Yellow }
                }
            }
            if ($x64path) {
                if ($x64path -notmatch '^[A-Za-z]:\\') {
                    $x64path = Join-Path $pwdRoot $x64path
                }
                if (Test-Path $x64path) {
                    if ((Get-Item $x64path).PSIsContainer) {
                        Get-ChildItem -Path $x64path -Filter *.dll -Recurse | ForEach-Object {
                            Copy-Item -Path $_.FullName -Destination $pluginsTemp64 -Force
                        }
                    } else {
                        Copy-Item -Path $x64path -Destination $pluginsTemp64 -Force
                    }
                    if (-not $Quiet) { Write-Host "Copied x64 plugin from $x64path" }
                } else {
                    if (-not $Quiet) { Write-Host "x64 plugin path not found: $x64path" -ForegroundColor Yellow }
                }
            }
        }
    }

    if ($skinDef.headerImage) {
        $header = $skinDef.headerImage

        if ($header -notmatch '^[A-Za-z]:\\') {
            $possible = Join-Path $pwdRoot $header
            if (Test-Path $possible) { $header = $possible }
        }
        if (Test-Path $header) {
            Copy-Item -Path $header -Destination (Join-Path $temp "RMSKIN.bmp") -Force
            if (-not $Quiet) { Write-Host "Copied header image" -ForegroundColor Cyan }
        } else {
            if (-not $Quiet) { Write-Host "Header image not found: $header" -ForegroundColor Yellow }
        }
    }

    if (-not $Quiet) {
        Write-Host "`nTemp directory contents:" -ForegroundColor Gray
        Get-ChildItem -Path $temp -Recurse | ForEach-Object {
            $relativePath = $_.FullName.Substring($temp.Length + 1)
            Write-Host "  $relativePath" -ForegroundColor Gray
        }
    }

    $zipPath = Join-Path $temp "skin.zip"
    if (Test-Path $zipPath) { Remove-Item $zipPath -Force }

    $tempContents = Get-ChildItem -Path $temp -Exclude "*.zip", "*.rmskin"
    if ($tempContents.Count -eq 0) {
        throw "No content found in temp directory to package"
    }

    if (-not $Quiet) { Write-Host "Creating ZIP archive..." -ForegroundColor Cyan }
    Compress-Archive -Path $tempContents.FullName -DestinationPath $zipPath -Force -CompressionLevel Optimal

    if (!(Test-Path $zipPath)) {
        throw "Failed to create ZIP archive"
    }

    $zipInfo = Get-Item $zipPath
    if ($zipInfo.Length -eq 0) {
        throw "ZIP archive is empty (0 bytes)"
    }

    if (-not $Quiet) { Write-Host "ZIP created successfully ($($zipInfo.Length) bytes)" -ForegroundColor Cyan }

    Add-RMSkinFooter -ZipPath $zipPath
    if (-not $Quiet) { Write-Host "Added RMSKIN footer to ZIP" -ForegroundColor Cyan }

    $filename = $skinDef.name
    if ($skinDef.version) { $filename += " $($skinDef.version)" }
    $filename += ".rmskin"

    if ($skinDef.output) {
        $outCandidate = $skinDef.output

        if ($outCandidate -notmatch '^[A-Za-z]:\\') { $outCandidate = Join-Path $pwdRoot $outCandidate }
        $OutputPath = $outCandidate
    } elseif ($OutPath) {
        $OutputPath = $OutPath
    } else {
        $dir = $env:USERPROFILE + "\Desktop"
        if ($OutDirectory) { $dir = $OutDirectory }
        $OutputPath = Join-Path $dir $filename
    }

    $interimRmskin = Join-Path $temp "skin.rmskin"
    Move-Item -Path $zipPath -Destination $interimRmskin -Force

    if (!(Test-Path $interimRmskin)) {
        throw "Failed to create RMSKIN file"
    }

    $rmskinInfo = Get-Item $interimRmskin
    if ($rmskinInfo.Length -eq 0) {
        throw "RMSKIN file is empty (0 bytes)"
    }

    $outDirPath = Split-Path $OutputPath -Parent
    if (!(Test-Path $outDirPath)) { New-Item -ItemType Directory -Path $outDirPath -Force | Out-Null }

    if (Test-Path $OutputPath) { Remove-Item $OutputPath -Force }

    Move-Item -Path $interimRmskin -Destination $OutputPath -Force

    try {
        Remove-Item -Path $temp -Recurse -Force
    } catch {
        if (-not $Quiet) { Write-Host "Warning: Could not clean up temp directory: $temp" -ForegroundColor Yellow }
    }

    if (!(Test-Path $OutputPath)) {
        throw "Final RMSKIN file was not created at: $OutputPath"
    }

    $finalInfo = Get-Item $OutputPath
    if ($finalInfo.Length -eq 0) {
        throw "Final RMSKIN file is empty (0 bytes): $OutputPath"
    }

    if (-not $Quiet) {
        Write-Host "Created RMSKIN at: $OutputPath ($($finalInfo.Length) bytes)" -ForegroundColor Green
    }

    return $OutputPath
}

function New-PluginZip {
    param(
        [Parameter(Mandatory = $true)][string]$Name,
        [Parameter(Mandatory = $true)][string]$Version,
        [string]$OutputDirectory = ".\dist"
    )
    
    $zipName = "$($Name)_v$($Version)_x64_x86_dll.zip"
    $zipPath = Join-Path $OutputDirectory $zipName
    
    # Remove existing zip if it exists
    if (Test-Path $zipPath) {
        Remove-Item $zipPath -Force
    }
    
    # Check if plugin directories exist and have DLLs
    $x64Path = Join-Path $OutputDirectory "x64"
    $x32Path = Join-Path $OutputDirectory "x32"
    
    $foldersToZip = @()
    
    if (Test-Path $x64Path) {
        $x64Dlls = Get-ChildItem -Path $x64Path -Filter "*.dll" -File
        if ($x64Dlls.Count -gt 0) {
            $foldersToZip += $x64Path
            Write-Host "Found $($x64Dlls.Count) x64 plugin(s)" -ForegroundColor Cyan
        }
    }
    
    if (Test-Path $x32Path) {
        $x32Dlls = Get-ChildItem -Path $x32Path -Filter "*.dll" -File
        if ($x32Dlls.Count -gt 0) {
            $foldersToZip += $x32Path
            Write-Host "Found $($x32Dlls.Count) x32 plugin(s)" -ForegroundColor Cyan
        }
    }
    
    if ($foldersToZip.Count -eq 0) {
        Write-Host "No plugin DLLs found to zip" -ForegroundColor Yellow
        return $null
    }
    
    # Create the zip file with folder structure
    try {
        Compress-Archive -Path $foldersToZip -DestinationPath $zipPath -Force -CompressionLevel Optimal
        
        if (Test-Path $zipPath) {
            $zipInfo = Get-Item $zipPath
            Write-Host "Created plugin ZIP: $zipName ($($zipInfo.Length) bytes)" -ForegroundColor Green
            return $zipPath
        } else {
            Write-Host "Failed to create plugin ZIP file" -ForegroundColor Red
            return $null
        }
    }
    catch {
        Write-Host "Error creating plugin ZIP: $_" -ForegroundColor Red
        return $null
    }
}

function Build-Architecture {
    param(
        [Parameter(Mandatory = $true)][string]$Platform,
        [Parameter(Mandatory = $true)][string]$SolutionFile
    )
    
    Write-Host "Building $Platform architecture..." -ForegroundColor Yellow
    
    $buildArgs = @(
        $SolutionFile
        "/p:Configuration=Release"
        "/p:Platform=$Platform"
        "/m"  # Multi-processor build
        "/v:minimal"  # Minimal verbosity
    )
    
    & $msbuild $buildArgs
    
    if ($LASTEXITCODE -ne 0) {
        Write-Host "MSBuild failed for $Platform with exit code $LASTEXITCODE" -ForegroundColor Red
        return $false
    }
    
    Write-Host "$Platform build completed successfully" -ForegroundColor Green
    return $true
}

function Find-BuiltDll {
    param(
        [Parameter(Mandatory = $true)][string]$Architecture,
        [Parameter(Mandatory = $true)][string]$ProjectName
    )
    
    # Map MSBuild platform names to folder names that are actually used
    $platformMapping = @{
        "x64" = @("x64")
        "x86" = @("x86", "x32", "Win32")  # x86 can output to multiple folder names
        "Win32" = @("x86", "x32", "Win32")  # Win32 also maps to these folders
    }
    
    $searchFolders = $platformMapping[$Architecture]
    if (-not $searchFolders) {
        Write-Host "Unknown architecture: $Architecture" -ForegroundColor Red
        return $null
    }
    
    # Build search paths for all possible folder names
    $searchPaths = @()
    foreach ($folder in $searchFolders) {
        $searchPaths += @(
            ".\$ProjectName\$folder\Release\$ProjectName.dll",
            ".\$ProjectName\bin\$folder\Release\$ProjectName.dll",
            ".\$ProjectName\bin\Release\$ProjectName.dll",
            ".\bin\$folder\Release\$ProjectName.dll",
            ".\bin\Release\$ProjectName.dll",
            ".\$folder\Release\$ProjectName.dll",
            ".\Release\$ProjectName.dll"
        )
    }
    
    # Also check for .NET builds
    $netSearchPaths = Get-ChildItem ".\$ProjectName\bin\Release\net*\$ProjectName.dll" -ErrorAction SilentlyContinue | 
                      Select-Object -First 1 -ExpandProperty FullName
    
    if ($netSearchPaths) {
        $searchPaths += $netSearchPaths
    }
    
    foreach ($path in $searchPaths) {
        if (Test-Path $path) {
            Write-Host "Found $Architecture DLL at: $path" -ForegroundColor Cyan
            return $path
        }
    }
    
    Write-Host "No $Architecture DLL found in expected locations" -ForegroundColor Yellow
    Write-Host "Searched paths:" -ForegroundColor Gray
    foreach ($path in $searchPaths) {
        Write-Host "  $path" -ForegroundColor Gray
    }
    return $null
}

function Copy-DllToStandardizedFolder {
    param(
        [Parameter(Mandatory = $true)][string]$SourcePath,
        [Parameter(Mandatory = $true)][string]$Architecture,
        [Parameter(Mandatory = $true)][string]$DistPath
    )
    
    # Map MSBuild platforms to standardized folder names
    $folderMapping = @{
        "x64" = "x64"
        "x86" = "x32"
        "Win32" = "x32"
    }
    
    $targetFolder = $folderMapping[$Architecture]
    if (-not $targetFolder) {
        Write-Host "Unknown architecture: $Architecture" -ForegroundColor Red
        return $false
    }
    
    $targetPath = Join-Path $DistPath $targetFolder
    
    # Ensure target directory exists
    if (!(Test-Path $targetPath)) {
        New-Item -ItemType Directory -Path $targetPath -Force | Out-Null
    }
    
    try {
        Copy-Item -Path $SourcePath -Destination $targetPath -Force
        Write-Host "Copied $Architecture build to $targetFolder folder: $SourcePath" -ForegroundColor Green
        return $true
    }
    catch {
        Write-Host "Failed to copy $Architecture build: $_" -ForegroundColor Red
        return $false
    }
}

function Dist {
    param (
        [Parameter(Mandatory = $true)][int16]$major,
        [Parameter(Mandatory = $true)][int16]$minor,
        [Parameter(Mandatory = $true)][int16]$patch
    )

    $solutionFile = ".\WebView2-Plugin.sln"
    $projectName = "WebView2"
    
    if (!(Test-Path $solutionFile)) {
        throw "Solution file not found: $solutionFile"
    }

    if (Test-Path $msbuild) {
        Write-Host "Starting build process..." -ForegroundColor Yellow
        
        # Clean previous builds
        Write-Host "Cleaning previous builds..." -ForegroundColor Gray
        & $msbuild $solutionFile /t:Clean /p:Configuration=Release /v:minimal
        
        # Build both architectures - using x64 and Win32
        $x64Success = Build-Architecture -Platform "x64" -SolutionFile $solutionFile
        $x86Success = Build-Architecture -Platform "Win32" -SolutionFile $solutionFile
        
        # If neither build succeeded, throw an error
        if (-not $x64Success -and -not $x86Success) {
            throw "Both x64 and x86 builds failed"
        }
        
        if (-not $x64Success) {
            Write-Host "Warning: x64 build failed, continuing with x86 only" -ForegroundColor Yellow
        }
        
        if (-not $x86Success) {
            Write-Host "Warning: x86 build failed, continuing with x64 only" -ForegroundColor Yellow
        }
        
    } else {
        Write-Host "MSBuild not found at $msbuild - skipping build step" -ForegroundColor Yellow
    }

    # Clean and prepare dist directory with standardized folder names
    Remove-Item -Recurse -Force -ErrorAction SilentlyContinue .\dist
    New-Item -ItemType Directory -Path ".\dist\x64" -Force | Out-Null
    New-Item -ItemType Directory -Path ".\dist\x32" -Force | Out-Null

    $ver = "$($major).$($minor).$($patch)"
    BumpVersion $ver

    # Find and copy built DLLs to standardized folders
    $builtX64 = Find-BuiltDll -Architecture "x64" -ProjectName $projectName
    $builtX86 = Find-BuiltDll -Architecture "x86" -ProjectName $projectName

    $x64Copied = $false
    $x86Copied = $false

    if ($builtX64) {
        $x64Copied = Copy-DllToStandardizedFolder -SourcePath $builtX64 -Architecture "x64" -DistPath ".\dist"
    } else {
        Write-Host "x64 build not found at expected paths." -ForegroundColor Yellow
    }

    if ($builtX86) {
        $x86Copied = Copy-DllToStandardizedFolder -SourcePath $builtX86 -Architecture "x86" -DistPath ".\dist"
    } else {
        Write-Host "x86 build not found at expected paths." -ForegroundColor Yellow
    }

    # Verify at least one architecture was copied successfully
    $x64Files = Get-ChildItem ".\dist\x64\*.dll" -ErrorAction SilentlyContinue
    $x32Files = Get-ChildItem ".\dist\x32\*.dll" -ErrorAction SilentlyContinue
    
    if (-not $x64Files -and -not $x32Files) {
        throw "No DLL files found in either x64 or x32 dist directories. Build may have failed."
    }

    # Update skin definition with plugin paths (always use standardized folder names)
    $skinDefPath = Join-Path (Get-Location).Path "Resources\skin_definition.json"
    
    if (Test-Path $skinDefPath) {
        $skinDef = Get-Content $skinDefPath -Raw | ConvertFrom-Json
        if ($skinDef.plugins) {
            foreach ($p in $skinDef.plugins) {
                if ($p.name -and $p.name -match "$projectName|WebView|WebView.dll|$projectName.dll") {
                    # Only set paths for architectures that were successfully copied
                    if ($x32Files) { $p.x32 = ".\dist\x32" }
                    if ($x64Files) { $p.x64 = ".\dist\x64" }
                }
            }
            ($skinDef | ConvertTo-Json -Depth 10) | Set-Content $skinDefPath -Encoding UTF8
        }
    }

    # Determine root config
    $skinDef = Get-Content $skinDefPath -Raw | ConvertFrom-Json
    $root = $null
    if ($skinDef.load) {
        $root = ($skinDef.load -split "[\\/]+")[0]
    }
    if (-not $root) { throw "Could not determine root config from skin_definition.json load field." }

    # Create plugin ZIP file
    $pluginZip = New-PluginZip -Name $skinDef.name -Version $ver
    if ($pluginZip) {
        try {
            $pluginZipFullPath = [System.IO.Path]::GetFullPath($pluginZip)
        } catch {
            $pluginZipFullPath = (Resolve-Path $pluginZip -ErrorAction SilentlyContinue).Path
            if (-not $pluginZipFullPath) {
                $pluginZipFullPath = $pluginZip
            }
        }
        Write-Host "Plugin ZIP created at: $pluginZipFullPath" -ForegroundColor Green
    }

    # Create RMSKIN package
    $output = New-Package -RootConfig $root
    try {
        $outputFullPath = [System.IO.Path]::GetFullPath($output)
    } catch {
        $outputFullPath = (Resolve-Path $output -ErrorAction SilentlyContinue).Path
        if (-not $outputFullPath) {
            $outputFullPath = $output
        }
    }
    Write-Host "RMSKIN packaged: $outputFullPath" -ForegroundColor Green
    
    Write-Host "`nBuild completed successfully!" -ForegroundColor Green
    Write-Host "Output files:" -ForegroundColor White
    if ($pluginZip) { 
        Write-Host "  - Plugin DLLs: $pluginZipFullPath" -ForegroundColor Gray 
    }
    Write-Host "  - RMSKIN package: $outputFullPath" -ForegroundColor Gray
    
    # Summary of what was built
    Write-Host "`nBuild Summary:" -ForegroundColor White
    if ($x64Files) {
        Write-Host "  - x64 plugins: $($x64Files.Count) file(s)" -ForegroundColor Gray
    }
    if ($x32Files) {
        Write-Host "  - x32 plugins: $($x32Files.Count) file(s)" -ForegroundColor Gray
    }
}