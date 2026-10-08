#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "crypt.h"
extern const uint8_t MasterKeyPes21[];
int main(int c,char**v){uint32_t n;uint8_t*in=readFile(v[1],&n);
 struct FileDescriptorNew*d=createFileDescriptorNew();
 decryptWithKeyNew(d,in,(const char*)MasterKeyPes21);
 printf("dataSize=%u ver=%.32s type=%.32s\n",d->fileHeader->dataSize,d->fileHeader->gameVersionString,d->fileHeader->fileTypeString);
 FILE*f=fopen(v[2],"wb");fwrite(d->data,1,d->fileHeader->dataSize,f);fclose(f);
 int sz;uint8_t*out=encryptWithKeyNew(d,&sz,(const char*)MasterKeyPes21);
 printf("reencrypted=%d original=%u identical=%d\n",sz,n,sz==(int)n&&!memcmp(out,in,n));
 FILE*g=fopen(v[3],"wb");fwrite(out,1,sz,g);fclose(g);
 struct FileDescriptorNew*d2=createFileDescriptorNew();decryptWithKeyNew(d2,out,(const char*)MasterKeyPes21);
 printf("roundtrip_data_equal=%d\n",!memcmp(d2->data,d->data,d->fileHeader->dataSize));return 0;}
