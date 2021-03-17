#ifndef _LIBICONV_H
#define _LIBICONV_H
#define LIBICONV_INTERNAL
#include <stddef.h>
#ifndef WINICONV_CONST
# ifdef ICONV_CONST
#  define WINICONV_CONST ICONV_CONST
# else
#  ifndef __GNUC__
#    define WINICONV_CONST const
#  else
#    define WINICONV_CONST
#  endif
# endif
#endif
#ifdef __cplusplus
extern "C" {
#endif
typedef void* ppelib_iconv_t;
ppelib_iconv_t ppelib_iconv_open(const char *tocode, const char *fromcode);
int ppelib_iconv_close(ppelib_iconv_t cd);
size_t ppelib_iconv(ppelib_iconv_t cd, WINICONV_CONST char **inbuf, size_t *inbytesleft, char **outbuf, size_t *outbytesleft);
#ifdef __cplusplus
}
#endif
#endif
