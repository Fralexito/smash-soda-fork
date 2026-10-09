/* enc2 <orig_cifrado> <datos_planos_nuevos|-> <salida> [nombre_128B] [info_texto]
   - Usa la cabecera/logo/serial del original. Si datos == "-" conserva los datos del original.
   - nombre_128B: reemplaza los primeros 128 bytes de la descripcion. OJO: el menu Cargar de Liga Master NO lo muestra.
   - info_texto: reemplaza los bytes 128.. de la descripcion; ES LO QUE MUESTRA el menu Cargar (3 lineas separadas por \n:
     equipo/liga, fecha, competicion). Para distinguir ranuras de prueba, poner la etiqueta en la primera linea. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "crypt.h"
extern const uint8_t MasterKeyPes21[];
int main(int c,char**v){
 if(c<4){fprintf(stderr,"uso: enc2 orig datos|- salida [nombre]\n");return 2;}
 uint32_t n,m;uint8_t*in=readFile(v[1],&n);
 struct FileDescriptorNew*d=createFileDescriptorNew();
 decryptWithKeyNew(d,in,(const char*)MasterKeyPes21);
 if(strcmp(v[2],"-")){
   uint8_t*nd=readFile(v[2],&m);
   if(m!=d->fileHeader->dataSize){fprintf(stderr,"tamano distinto: %u vs %u\n",m,d->fileHeader->dataSize);return 1;}
   memcpy(d->data,nd,m);
 }
 if(c>=5){
   size_t L=strlen(v[4]); if(L>120){fprintf(stderr,"nombre muy largo\n");return 1;}
   memset(d->description,0,128); memcpy(d->description,v[4],L);
 }
 if(c>=6){
   size_t L=strlen(v[5]); uint32_t ds=d->fileHeader->descSize;
   if(ds<=128||L>=ds-128){fprintf(stderr,"info_texto no cabe (max %u)\n",ds-129);return 1;}
   memset(d->description+128,0,ds-128); memcpy(d->description+128,v[5],L);
 }
 int sz;uint8_t*out=encryptWithKeyNew(d,&sz,(const char*)MasterKeyPes21);
 FILE*f=fopen(v[3],"wb");fwrite(out,1,sz,f);fclose(f);
 printf("escrito %d bytes (original %u)\n",sz,n);return 0;}
