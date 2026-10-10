# =============================================================================
#  Phoenix Sync · build independiente
#  Vive en la raíz del repo (NO dentro de SmashSoda/) a propósito: la build de
#  Phoenix Soda junta todos los .cpp de SmashSoda/ y no debe verse afectada.
#
#  Compilar (Windows, desde la raíz del repo):
#    cmake -S PhoenixSync -B build-sync -A x64
#    cmake --build build-sync --config Release
#    build-sync\Release\PhoenixSyncPruebas.exe
# =============================================================================
cmake_minimum_required(VERSION 3.18)
project(PhoenixSync LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
if(MSVC)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreadedDLL")   # igual que Phoenix Soda
    add_compile_options(/utf-8 $<$<COMPILE_LANGUAGE:CXX>:/W3> $<$<COMPILE_LANGUAGE:CXX>:/permissive->)
endif()

# --- Terceros en C (ver terceros/README.md) -----------------------------------
add_library(PhoenixSyncTerceros STATIC
    terceros/pesxcrypter/crypt.c
    terceros/pesxcrypter/masterkey.c
    terceros/pesxcrypter/mt19937ar.c
    terceros/miniz/miniz.c
    terceros/ed25519/fe.c
    terceros/ed25519/ge.c
    terceros/ed25519/sc.c
    terceros/ed25519/sha512.c
    terceros/ed25519/verify.c
    terceros/ed25519/sign.c
    terceros/ed25519/keypair.c
)
if(MSVC)
    target_compile_options(PhoenixSyncTerceros PRIVATE /W0)
    target_compile_definitions(PhoenixSyncTerceros PRIVATE _CRT_SECURE_NO_WARNINGS)
else()
    target_compile_options(PhoenixSyncTerceros PRIVATE -w)
endif()

# --- Core: toda la lógica, sin Windows ni interfaz ---------------------------
add_library(PhoenixSyncCore STATIC
    core/Sha256.cpp
    core/Copias.cpp
    core/ClienteMercado.cpp
    core/OptionFile.cpp
    core/BaseDatosParche.cpp
    core/Catalogo.cpp
    core/Emparejamiento.cpp
    core/Integridad.cpp
    core/SobrePes.cpp
    core/LigaMaster.cpp
    core/Firma.cpp
    core/BlobLM.cpp
    core/Sincronizacion.cpp
    core/RutasJuego.cpp
    core/Respaldos.cpp
    core/Entrega.cpp
    core/Grupo.cpp
    core/ClienteSync.cpp
    core/SyncCompartido.cpp
)
target_link_libraries(PhoenixSyncCore PUBLIC PhoenixSyncTerceros)
target_include_directories(PhoenixSyncCore PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}"
    "${CMAKE_CURRENT_SOURCE_DIR}/../external"   # nlohmann/json (ya está en el repo)
)

# --- Pruebas: corren en cualquier sistema ------------------------------------
add_executable(PhoenixSyncPruebas pruebas/main.cpp)
target_link_libraries(PhoenixSyncPruebas PRIVATE PhoenixSyncCore)
add_executable(PhoenixSyncPruebasCompartido pruebas/compartido.cpp)
target_link_libraries(PhoenixSyncPruebasCompartido PRIVATE PhoenixSyncCore)

# --- App de prueba: solo Windows ---------------------------------------------
if(WIN32)
    add_executable(PhoenixSync app/main.cpp app/ComandosSync.cpp windows/Plataforma.cpp)
    target_compile_definitions(PhoenixSync PRIVATE UNICODE _UNICODE NOMINMAX)
    target_link_libraries(PhoenixSync PRIVATE PhoenixSyncCore winhttp crypt32 shell32 ole32 uuid)
endif()
