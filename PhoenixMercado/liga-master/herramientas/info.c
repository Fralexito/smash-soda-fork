#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "crypt.h"
extern const uint8_t MasterKeyPes21[];
int main(int c,char**v){uint32_t n;uint8_t*in=readFile(v[1],&n);
 struct FileDescriptorNew*d=createFileDescriptorNew();
 decryptWithKeyNew(d,in,(const char*)MasterKeyPes21);
 struct FileHeaderNew*h=d->fileHeader;
 printf("dataSize=%u logoSize=%u descSize=%u serialLength=%u\n",h->dataSize,h->logoSize,h->descSize,h->serialLength);
 printf("type=%.32s ver=%.32s\n",h->fileTypeString,h->gameVersionString);
 printf("desc hex:");for(uint32_t i=0;i<h->descSize&&i<400;i++)printf("%02x",d->description[i]);printf("\n");
 printf("desc utf16: ");for(uint32_t i=0;i+1<h->descSize;i+=2){unsigned ch=d->description[i]|(d->description[i+1]<<8);if(ch==0)printf("|");else if(ch<128)putchar(ch);else printf("<%x>",ch);}printf("\n");
 printf("desc ascii: ");for(uint32_t i=0;i<h->descSize;i++){unsigned ch=d->description[i];putchar(ch>=32&&ch<127?ch:'.');}printf("\n");
 printf("serial hex:");for(uint32_t i=0;i<h->serialLength*2&&i<64;i++)printf("%02x",d->serial[i]);printf("\n");
 printf("mystery:");for(int i=0;i<64;i++)printf("%02x",h->mysteryData[i]);printf("\nhash:");for(int i=0;i<64;i++)printf("%02x",h->hash[i]);printf("\n");
 return 0;}
