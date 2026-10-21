# Beta privada de Phoenix Link: cómo repartirla sin que se la queden (0.2.83+)

Qué protege y qué no: un programa que corre en el PC de otra persona no se puede blindar al 100 %. Lo que sí se consigue es **disuadir, rastrear de quién es cada copia y poder apagarla**. La parte de más valor (liga, radar, traspasos) vive en la web y esa sí la controlas.

## 1. Antes de repartir (una sola vez)
1. **Pon los repos en privado** (GitHub › cada repo › Settings › Danger Zone › Change visibility): `Fralexito/smash-soda-fork` y `Fralexito/phoenix-overlay`. Hoy son públicos. Mientras sigan así, **no se sube nada nuevo** (los cambios se guardan solo en tu PC con `git commit`).
2. Decide la **fecha de caducidad** de la beta: constante `kCaduca` en `SmashSoda/phoenix/PhoenixBuild.h` (formato `AAAA-MM-DD`). Pasada esa fecha Link no abre y pide la versión nueva; 7 días antes avisa al arrancar. Para renovar: cambia la fecha, compila y reparte los instaladores nuevos.
3. Tu PC está **exento** de la caducidad mientras exista el archivo marca que define `esDueno()` en `PhoenixBeta.h` (en la carpeta de configuración de Link). No lo nombres en ningún sitio público. Los testers no lo tienen.

## 2. Un instalador distinto por persona
```
powershell -ExecutionPolicy Bypass -File tools\CREAR-INSTALADOR-EXE.ps1 -Tester Mirko
```
- Crea `PhoenixLink-Setup-<versión>-Mirko.exe` en `C:\dev\phoenix-beta2-dist`.
- Cada instalador tiene un **SHA256 distinto**. Se anota en `testers.csv` (tester, fecha, versión, caducidad, sha256, archivo). Ese archivo es **privado**: no lo compartas.
- Lo que instala: `tester.id` (el nombre del tester) junto a `PhoenixLink.exe` y la `LICENCIA-BETA.txt`. Link manda el nombre en la cabecera `X-Phoenix-Tester` a la web.
- El asistente muestra la **licencia de la versión de prueba** y obliga a aceptarla antes de instalar.
- Si una copia aparece donde no debe: calcula su SHA256 (si es el instalador) o mira `tester.id` (si es la carpeta instalada) y búscalo en `testers.csv`.

## 3. Cómo mandarlo
- Un enlace **privado por persona**: Drive restringido a su correo, no «cualquiera con el enlace».
- Solo gente de confianza y pocas personas al principio.
- Un mensaje claro por escrito: «es una prueba privada, no lo compartas; caduca el AAAA-MM-DD».

## 4. Apagar a una persona (revocar)
En la web (Supabase), cada PC vinculada es una fila de `dispositivos_host`:
- **Suspender** (reversible): `select public.staff_suspender_dispositivo('<id del dispositivo>', true, 'motivo');` (con `false` lo reactivas).
- **Revocar** (definitivo): `select public.revocar_dispositivo('<id del dispositivo>');`
- Desde ese momento ese Link deja de funcionar en la liga, los traspasos y las salas («La web no reconoce este Link»). Lo local (abrir Link, mandos) sigue hasta que caduque la build.
- Para saber el id: `select id, usuario, nombre, creado from public.dispositivos_host order by creado desc;`

## 5. Límites honestos
- El reloj del PC se puede cambiar para saltarse la caducidad.
- Quien borre `tester.id` pierde el rastro por carpeta, pero el SHA256 del instalador sigue señalándolo.
- Los archivos de la interfaz (`ui`) y los módulos de PES (`.lua`) van legibles dentro de la instalación.
- La base del proyecto (Smash Soda) es de licencia MIT: esa parte la puede copiar cualquiera. Lo propio de Phoenix es lo que va cubierto por la licencia de la beta.
- `parsec.dll` (SDK de Parsec) va dentro del instalador. La licencia de la API de Soda Arcade dice que las versiones compiladas que se redistribuyen no deberían llevar el SDK de Parsec. Con pocas personas conocidas el riesgo es bajo; antes de repartir a más gente, decide si cada tester lo descarga de Parsec o si se pide permiso.
- Esta licencia está escrita en lenguaje sencillo y **no ha sido revisada por un abogado**.
