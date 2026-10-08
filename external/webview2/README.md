# WebView2 (Microsoft Edge) — archivos del SDK usados por Phoenix Link

Phoenix Link muestra su interfaz nueva (HTML) con **WebView2**, el navegador de Windows
(el mismo motor de Edge) incrustado en la ventana. Viene instalado en Windows 11 y en
casi todos los Windows 10; si falta, la app usa la interfaz ImGui de siempre.

| Archivo | Versión | Origen | SHA-256 |
|---|---|---|---|
| `include/WebView2.h` | SDK 1.0.664.37 | paquete NuGet `Microsoft.Web.WebView2` (copia en el historial de github.com/webview/webview, `script/microsoft.web.webview2.1.0.664.37/`) | `aecfebdbc54cfccea7c5f53d5bd7b2667c7a355bd5360eeb82d049b81bf3c472` |
| `x64/WebView2Loader.dll` | 1.0.3296.44 | paquete NuGet `Microsoft.Web.WebView2` (copia en el crate `webview2-com-sys` 0.38.0) — firmado por Microsoft Corporation | `1f278c2c6796ce1a08f8cc5fc90efbcc87c6136179287c9edf19946991ea429b` |
| `x86/WebView2Loader.dll` | 1.0.3296.44 | ídem | `662332890b7c4a5c7f344622b976cd2d811f623e81c09677f5967729207d75e0` |
| `LICENSE.txt` | — | licencia BSD del SDK de WebView2 (permite redistribuir) | — |

Notas:
- Se usa la cabecera 1.0.664.37 a propósito: solo trae interfaces estables que cualquier
  WebView2 Runtime actual soporta. El código no usa nada más nuevo.
- `WebView2Loader.dll` se carga en tiempo de ejecución (`LoadLibrary`) desde la carpeta del
  exe; no hace falta ninguna `.lib`. CMake la copia junto a `PhoenixLink.exe`.
- `EventToken.h` (lo incluye `WebView2.h`) viene con el Windows SDK de Visual Studio.
