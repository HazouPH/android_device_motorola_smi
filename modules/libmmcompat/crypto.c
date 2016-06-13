#include <malloc.h>

void (*CRYPTO_get_locking_callback(void))(int mode,int type,const char *file,
                int line)
{
    return 0;
}

void *CRYPTO_malloc(int num, const char *file, int line)
{
    if (num <= 0) return NULL;
    return malloc(num);
}

void CRYPTO_free(void *str)
{
    free(str);
}
