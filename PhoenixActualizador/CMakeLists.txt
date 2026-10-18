# PhoenixActualizador.exe: instala la versión nueva de Phoenix Link (ver Instalador.h). Sin dependencias, runtime estático.
add_executable(PhoenixActualizador WIN32 main.cpp Instalador.cpp Instalador.h)
set_target_properties(PhoenixActualizador PROPERTIES MSVC_RUNTIME_LIBRARY "MultiThreaded")
target_compile_definitions(PhoenixActualizador PRIVATE UNICODE _UNICODE NOMINMAX)
target_compile_options(PhoenixActualizador PRIVATE /utf-8)
