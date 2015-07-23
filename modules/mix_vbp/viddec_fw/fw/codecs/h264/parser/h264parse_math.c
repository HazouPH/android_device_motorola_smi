//#include "math.h"
// Arithmatic functions using add & subtract

#include "h264parse.h"

uint32_t mult_u(register uint32_t var1, register uint32_t var2)
{

    register unsigned long var_out = 0;

    while (var2 > 0)
    {

        if (var2 & 0x01)
        {
            var_out += var1;
        }
        var2 >>= 1;
        var1 <<= 1;
    }
    return var_out;

}// mult_u

uint32_t ldiv_mod_u(uint32_t a, uint32_t b, uint32_t * mod)
{
    register unsigned long div = b;
    register unsigned long res = 0;
    register unsigned long bit = 0x1;

    if (!div)
    {
        *mod = 0;
        return 0xffffffff ; // Div by 0
    }

    if (a < b)
    {
        *mod = a;
        return 0; // It won't even go once
    }

    while (!(div & 0x80000000))
    {
        div <<= 1;
        bit <<= 1;
    }

    while (bit)
    {
        if (div <= a)
        {
            res |= bit;
            a -= div;
        }
        div >>= 1;
        bit >>= 1;
    }
    *mod = a;
    return res;
}// ldiv_mod_u


unsigned ldiv_u(register unsigned a, register unsigned  b)
{
    register unsigned div = b << 16;
    register unsigned res = 0;
    register unsigned bit = 0x10000;

    while (bit)
    {
        div >>= 1;
        bit >>= 1;
        if (div < a)
        {
            res |= bit;
            a -= div;
        }
    }

    return res;
}


