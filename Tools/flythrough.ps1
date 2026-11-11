# Graba el descenso sin costura (-MengerFlythrough) fuera de pantalla: lanza el juego con -RenderOffscreen y
# prioridad baja, guarda un PNG por frame en Saved/Flythrough, compara el último frame con el primero (deben
# ser iguales: el ciclo baja un número entero de niveles) y, si se pide, codifica el vídeo con ffmpeg.
#
# Mientras el juego está abierto se toma el cerrojo de GPU compartido (carpeta $LockDir, creación atómica)
# y se vigila que ninguna ventana de Unreal pase a primer plano; si pasa, se mata el proceso por PID.
param(
    [int]$Frames = 720,
    [int]$Levels = 3,
    [double]$Start = 20,
    [double]$Distance = 3,
    [double]$Yaw = 6,
    [double]$Pitch = 2,
    [int]$Every = 1,
    [int]$Warmup = 60,
    [int]$ResX = 1920,
    [int]$ResY = 1080,
    [int]$TimeoutMinutes = 40,
    [string]$Mp4 = '',
    [string]$LockDir = (Join-Path $env:USERPROFILE '.ultron\.tmp\portfolio-2026-10\locks\gpu'),
    [string]$ForegroundScript = (Join-Path $env:USERPROFILE '.ultron\.tmp\portfolio-2026-10\fractales-tools\fg.ps1'),
    # Diferencia media máxima por canal (0-255) entre el primer y el último frame.
    [double]$MaxLoopDiff = 0.5
)
. "$PSScriptRoot\common.ps1"

$flyDir = Join-Path $ProjectRoot 'Saved\Flythrough'
$inv = [Globalization.CultureInfo]::InvariantCulture

function Get-Foreground {
    if (-not (Test-Path $ForegroundScript)) { return '' }
    return (& powershell -NoProfile -File $ForegroundScript | Out-String).Trim()
}

function Test-UnrealForeground([string]$Name) { return $Name -match 'Unreal|MengerInfinite' }

function Enter-GpuLock {
    $deadline = (Get-Date).AddMinutes(20)
    New-Item -ItemType Directory -Force -Path (Split-Path -Parent $LockDir) | Out-Null
    while ($true) {
        try {
            New-Item -ItemType Directory -Path $LockDir -ErrorAction Stop | Out-Null
            return
        }
        catch {
            if ((Get-Date) -gt $deadline) { throw "Cerrojo de GPU ocupado 20 min: $(Get-Content (Join-Path $LockDir 'owner.txt') -ErrorAction SilentlyContinue)" }
            Start-Sleep -Seconds 30
        }
    }
}

$before = Get-Foreground
if (Test-UnrealForeground $before) { throw "Ya hay una ventana de Unreal en primer plano ($before): no se lanza nada" }

Enter-GpuLock
$process = $null
try {
    Set-Content -Path (Join-Path $LockDir 'owner.txt') -Value "fractales-ue $PID $(Get-Date -Format o)"
    $gameArgs = @(
        "`"$UProject`"", '-game', '-RenderOffscreen', "-ResX=$ResX", "-ResY=$ResY", '-nosound', '-unattended',
        '-NoSplash', '-benchmark', '-fps=60', '-MengerFlythrough', '-log=MengerFlythrough.log',
        "-FlyFrames=$Frames", "-FlyLevels=$Levels", ('-FlyStart=' + $Start.ToString($inv)),
        ('-FlyDistance=' + $Distance.ToString($inv)), ('-FlyYaw=' + $Yaw.ToString($inv)),
        ('-FlyPitch=' + $Pitch.ToString($inv)), "-FlyEvery=$Every", "-FlyWarmup=$Warmup")
    $process = Start-Process -FilePath $Editor -ArgumentList $gameArgs -WindowStyle Hidden -PassThru
    $process.PriorityClass = 'BelowNormal'
    Set-Content -Path (Join-Path $LockDir 'owner.txt') -Value "fractales-ue $($process.Id) $(Get-Date -Format o)"
    Write-Output "Juego lanzado (PID $($process.Id)); primer plano antes: $before"

    $started = Get-Date
    while (-not $process.HasExited) {
        $elapsed = ((Get-Date) - $started).TotalSeconds
        $foreground = Get-Foreground
        if (Test-UnrealForeground $foreground) {
            Stop-Process -Id $process.Id -Force
            throw "El juego ha pasado a primer plano ($foreground): proceso $($process.Id) terminado"
        }
        if ($elapsed -gt $TimeoutMinutes * 60) {
            Stop-Process -Id $process.Id -Force
            throw "Tiempo agotado ($TimeoutMinutes min): proceso $($process.Id) terminado"
        }
        Start-Sleep -Seconds $(if ($elapsed -lt 30) { 1 } else { 5 })
    }
    Write-Output ("Juego cerrado (código {0}) en {1:N0} s; primer plano después: {2}" -f $process.ExitCode, ((Get-Date) - $started).TotalSeconds, (Get-Foreground))
}
finally {
    if ($process -and -not $process.HasExited) { Stop-Process -Id $process.Id -Force }
    Remove-Item -Path $LockDir -Recurse -Force -ErrorAction SilentlyContinue
}

$pngs = @(Get-ChildItem -Path $flyDir -Filter 'frame_*.png' -ErrorAction SilentlyContinue)
$expected = [math]::Floor($Frames / $Every) + 1
Write-Output "Capturas: $($pngs.Count) de $expected"
if ($pngs.Count -ne $expected) {
    Get-Content (Join-Path $ProjectRoot 'Saved\Logs\MengerFlythrough.log') -ErrorAction SilentlyContinue |
        Select-String -Pattern '(Error|MengerFlythrough)' | Select-Object -Last 20
    throw "Faltan capturas en $flyDir"
}

$first = Join-Path $flyDir 'frame_00000.png'
$last = Join-Path $flyDir ('frame_{0:D5}.png' -f $Frames)
$failed = $false
if (($Frames % $Every) -eq 0) {
    $diff = [double](& uv run --quiet --with pillow --with numpy python (Join-Path $PSScriptRoot 'imgdiff.py') $first $last)
    Write-Output ("Primer frame frente al frame {0}: diferencia media {1:N4}" -f $Frames, $diff)
    if ($diff -gt $MaxLoopDiff) { $failed = $true }
}

if ($Mp4 -and $Every -eq 1) {
    # El frame $Frames repite el 0: se codifican 0..Frames-1 para que el bucle no muestre un frame doble.
    $ffArgs = @('-y', '-hide_banner', '-loglevel', 'error', '-framerate', '60', '-start_number', '0',
        '-i', (Join-Path $flyDir 'frame_%05d.png'), '-frames:v', "$Frames", '-c:v', 'libx264', '-preset', 'slow',
        '-crf', '16', '-pix_fmt', 'yuv420p', '-an', '-movflags', '+faststart', $Mp4)
    $ffmpeg = Start-Process -FilePath 'ffmpeg' -ArgumentList $ffArgs -NoNewWindow -PassThru
    # Sin leer Handle antes de que termine, Process.ExitCode queda vacío en PowerShell 5.1.
    $null = $ffmpeg.Handle
    $ffmpeg.PriorityClass = 'BelowNormal'
    $ffmpeg.WaitForExit()
    if ($ffmpeg.ExitCode -ne 0) { throw "ffmpeg falló con código $($ffmpeg.ExitCode)" }
    Write-Output "Vídeo: $Mp4"
}

if ($failed) { exit 1 }
