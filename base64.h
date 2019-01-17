#ifndef __base64_H
#define __base64_H

#include<stddef.h>

typedef unsigned char BYTE;

size_t Base64_Decode(char *pDest, const char *pSrc, size_t srclen);
size_t Base64_Encode(char *pDest, const char *pSrc, size_t srclen);

#endif
