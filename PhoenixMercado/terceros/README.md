# Código de terceros (sin modificar salvo lo indicado)

| Carpeta | Origen | Licencia |
|---|---|---|
| `pesxcrypter/` | github.com/the4chancup/pesXdecrypter (rama libpesXcrypter) | Dominio público; `mt19937ar.*` con licencia BSD de sus autores |
| `miniz/` | github.com/richgel999/miniz v3.0.2 | MIT |
| `ed25519/` | github.com/orlp/ed25519 (solo verificar/firmar/claves: sin `seed.c`, `add_scalar.c` ni `key_exchange.c`) | zlib |

Único cambio: en `pesxcrypter/masterkey.h`, `MasterKeyZero` pasa a `extern` (evita una doble definición al enlazar).
