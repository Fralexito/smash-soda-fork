# Verificar si CMake está instalado
$cmakePath = Get-Command cmake -ErrorAction SilentlyContinue
if (-not $cmakePath) {
    Write-Host "CMake no encontrado. Instalando CMake..."
    # Descargar e instalar CMake
    $cmakeUrl = "https://github.com/Kitware/CMake/releases/download/v3.27.0/cmake-3.27.0-windows-x86_64.msi"
    $cmakeMsi = "$env:TEMP\cmake-installer.msi"
    
    Write-Host "Descargando CMake..."
    Invoke-WebRequest -Uri $cmakeUrl -OutFile $cmakeMsi
    
    Write-Host "Instalando CMake..."
    Start-Process -FilePath "msiexec.exe" -ArgumentList "/i `"$cmakeMsi`" /passive" -Wait
    
    # Limpiar
    Remove-Item $cmakeMsi -Force
    
    Write-Host "CMake instalado correctamente"
}

# Navegar a la carpeta del proyecto
$projectDir = "C:\Users\WinterOS\smash-soda-fork"
$buildDir = "$projectDir\build"

cd $projectDir

Write-Host "Limpiando build previo..."
Remove-Item $buildDir -Recurse -Force -ErrorAction SilentlyContinue
New-Item -ItemType Directory -Path $buildDir -Force | Out-Null

Write-Host "Configurando CMake..."
cd $buildDir
cmake -G "Visual Studio 18 2026" -A x64 ..

if ($LASTEXITCODE -ne 0) {
    Write-Host "Error en CMake configuration"
    Read-Host "Presiona Enter para continuar"
    exit 1
}

Write-Host "Compilando proyecto..."
cmake --build . --config Release --parallel

if ($LASTEXITCODE -ne 0) {
    Write-Host "Error en compilacion"
    Read-Host "Presiona Enter para continuar"
    exit 1
}

Write-Host "EXITO: Compilacion completada!"
Write-Host ""
Write-Host "Ejecutando PhoenixLink.exe..."

$exePath = "$projectDir\build\SmashSoda\Release\PhoenixLink.exe"
if (Test-Path $exePath) {
    Start-Process -FilePath $exePath
    Write-Host "Aplicacion iniciada"
} else {
    Write-Host "ERROR: PhoenixLink.exe no encontrado en $exePath"
}

Read-Host "Presiona Enter para terminar"
