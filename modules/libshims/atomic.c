#include <stdatomic.h>

int32_t android_atomic_inc(volatile int32_t* addr)
{
    volatile atomic_int_least32_t* a = (volatile atomic_int_least32_t*)addr;
        /* Int32_t, if it exists, is the same as int_least32_t. */
    return atomic_fetch_add_explicit(a, 1, memory_order_release);
}

int32_t android_atomic_dec(volatile int32_t* addr)
{
    volatile atomic_int_least32_t* a = (volatile atomic_int_least32_t*)addr;
    return atomic_fetch_sub_explicit(a, 1, memory_order_release);
}

int32_t android_atomic_and(int32_t value, volatile int32_t* addr)
{
    volatile atomic_int_least32_t* a = (volatile atomic_int_least32_t*)addr;
    return atomic_fetch_and_explicit(a, value, memory_order_release);
}

int32_t android_atomic_or(int32_t value, volatile int32_t* addr)
{
    volatile atomic_int_least32_t* a = (volatile atomic_int_least32_t*)addr;
    return atomic_fetch_or_explicit(a, value, memory_order_release);
}
