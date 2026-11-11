# Lanza el juego en modo benchmark (-MengerBench): mide cada pose fija, guarda capturas y CSV en
# Saved/Bench, falla si alguna pose no llega a los fps mínimos y compara las capturas de la misma
# pose a distintos niveles (deben ser iguales: la profundidad no puede degradar la imagen).
param(
    [int]$ResX = 1920,
    [int]$ResY = 1080,
    [double]$MinFps = 60,
    # Diferencia media máxima por canal (0-255) entre capturas de la misma pose a distinto nivel.
    [double]$MaxLevelDiff = 1.0
)
. "$PSScriptRoot\common.ps1"

$benchDir = Join-Path $ProjectRoot 'Saved\Bench'
if (Test-Path $benchDir) { Remove-Item $benchDir -Recurse -Force }

$process = Start-Process -FilePath $Editor -PassThru -Wait -ArgumentList @(
    "`"$UProject`"", '-game', '-windowed', "-ResX=$ResX", "-ResY=$ResY",
    '-MengerBench', '-NoSplash', '-NoSound', '-log=MengerBench.log')

$csv = Join-Path $benchDir 'bench.csv'
if (-not (Test-Path $csv)) {
    $log = Join-Path $ProjectRoot 'Saved\Logs\MengerBench.log'
    Get-Content $log -ErrorAction SilentlyContinue | Select-String -Pattern '(Error|MengerBench|Shader)' | Select-Object -Last 30
    throw "No se generó $csv (código de salida $($process.ExitCode))"
}

$rows = Import-Csv $csv
$rows | Format-Table -AutoSize | Out-String | Write-Output
$failed = $false
$slow = @($rows | Where-Object { [double]$_.fps -lt $MinFps })
if ($slow.Count -gt 0) {
    Write-Output "Por debajo de $MinFps fps: $($slow.pose -join ', ')"
    $failed = $true
}

# Wall10 ya ve la esponja raíz a lo lejos; a partir del nivel ~30 la imagen ya no puede cambiar con el nivel.
$reference = Join-Path $benchDir 'Wall50.png'
foreach ($deep in @('Wall1000.png')) {
    $output = & uv run --quiet --with pillow --with numpy python (Join-Path $PSScriptRoot 'imgdiff.py') $reference (Join-Path $benchDir $deep)
    $diff = [double]$output
    Write-Output ("Wall50 frente a {0}: diferencia media {1:N3}" -f $deep, $diff)
    if ($diff -gt $MaxLevelDiff) { $failed = $true }
}

if ($failed) { exit 1 }
Write-Output "Todo en orden. Capturas en $benchDir"
