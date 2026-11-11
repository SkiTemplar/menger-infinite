# Compila el editor (por defecto) o el juego y muestra solo errores, avisos y el resultado.
param(
    [ValidateSet('Editor', 'Game')] [string]$Target = 'Editor',
    [ValidateSet('Development', 'DebugGame', 'Shipping')] [string]$Configuration = 'Development'
)
. "$PSScriptRoot\common.ps1"

$targetName = if ($Target -eq 'Editor') { 'MengerInfiniteEditor' } else { 'MengerInfinite' }
$cmdArgs = @($targetName, 'Win64', $Configuration, "-Project=$UProject", '-WaitMutex', '-NoHotReloadFromIDE')
# Los avisos de deprecación que salen de cabeceras del motor son ruido: solo se muestran los del proyecto.
$code = Invoke-Native $BuildBat $cmdArgs '(error|Result:|^(?!.*Epic Games).*warning)'
if ($code -ne 0) { throw "Build $targetName falló con código $code" }
