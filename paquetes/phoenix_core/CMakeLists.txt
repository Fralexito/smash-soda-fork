# Phoenix Core · fase 0. Compilar en Windows con MSVC (x64):
#   cmake -S . -B build -A x64
#   cmake --build build --config Release
# Sale build\Release\phoenix_core.dll → copiarla a SiderAddons\phoenix_core\ (INSTALAR.ps1 la busca ahí).
cmake_minimum_required(VERSION 3.16)
project(phoenix_core CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
# /MT: sin depender de las DLL de Visual C++ instaladas en la PC del jugador
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
add_library(phoenix_core SHARED src/phoenix_core.cpp src/redireccion_online.cpp src/redireccion_online.h src/phoenix_api.h)
target_compile_definitions(phoenix_core PRIVATE PHOENIX_CORE_EXPORTS WIN32_LEAN_AND_MEAN NOMINMAX _CRT_SECURE_NO_WARNINGS)
target_compile_options(phoenix_core PRIVATE /W3 /EHa /utf-8)
