/*
    Phoenix Link: comprobacion previa para libpesXcrypter (dominio publico). Archivo NUEVO; el resto de esta carpeta es el original.
    decryptWithKeyNew() se fia de los tamanos que trae la cabecera del archivo: con un archivo a medio escribir, cortado o que no es
    de PES, lee fuera del buffer y cierra el programa. Esta funcion descifra SOLO la cabecera y comprueba que los tamanos cuadran con
    el tamano real del archivo ANTES de llamar a decryptWithKeyNew().
*/
#include <stdlib.h>
#include <string.h>

#include "mt19937ar.h"
#include "crypt.h"
#include "masterkey.h"

/* definidas (sin declarar) en crypt.c */
void cryptHeader(uint8_t *output, const uint8_t *input, const uint8_t *key);
void xorRepeatingBlocks(uint8_t *output, const uint8_t *input, int length);
void xorWithLongParam(const uint8_t *input, uint8_t *output, uint64_t param);
void cryptStream(uint8_t *output, const uint8_t *key, const uint8_t *input, int length);

/* 1 = la cabecera es coherente con `fileSize` (se puede llamar a decryptWithKeyNew). 0 = no. */
int pesxCabeceraNuevaValida(const uint8_t *input, size_t fileSize, const char *masterKey)
{
    uint8_t enc[320];
    uint8_t rollingKey[64], intermediateKey[64];
    struct FileHeaderNew h;
    unsigned long long total;

    if (!input || fileSize < 320 + sizeof(struct FileHeaderNew) || fileSize > ((size_t)512 << 20))
        return 0;
    cryptHeader(enc, input, (const uint8_t *)masterKey);
    memcpy(rollingKey, enc, 64);
    xorRepeatingBlocks(rollingKey, &enc[64], 256);
    xorWithLongParam(rollingKey, intermediateKey, sizeof(struct FileHeaderNew));
    cryptStream((uint8_t *)&h, intermediateKey, input + 320, sizeof(struct FileHeaderNew));

    if (h.descSize == 0 || h.descSize > (1u << 20) || h.logoSize > (16u << 20) || h.serialLength > 4096 || h.dataSize == 0 || h.dataSize > ((unsigned)512 << 20))
        return 0;
    total = 320ull + sizeof(struct FileHeaderNew) + h.descSize + h.logoSize + h.dataSize + (unsigned long long)h.serialLength * 2ull;
    return total <= (unsigned long long)fileSize ? 1 : 0;
}
