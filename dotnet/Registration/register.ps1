#! pwsh

$isOsx = $PSVersionTable.Platform -eq "Unix"
$config = if ($isOsx) { "osx-universal-debug" } else { "win-x64-debug" }
$entryPoint = Join-Path (Resolve-Path "$PSScriptRoot/../../native") "out/build/$config/NativeHost"
$entryPoint = $entryPoint.Replace("\", "/")

foreach($ver in 2021, 2022) {
    $dest =
        if ($isOsx) {
            "$($env:HOME)/Library/Application Support/SketchUp $ver/SketchUp/Plugins"
        } else {
            "$($env:AppData)\SketchUp\SketchUp $ver\SketchUp\Plugins"
        }

    if (Test-Path $dest) {
        $subDir = "$dest/MyPlugin"
        Copy-Item "$PSScriptRoot/MyPlugin.rb" $dest
        $null = New-Item -ItemType Directory -ErrorAction SilentlyContinue $subDir
        "require '$entryPoint'" > "$subDir/loader.rb"
    }
}