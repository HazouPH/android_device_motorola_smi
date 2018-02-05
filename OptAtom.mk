# Atom optimizations specified to improve memory speed.
ARCH_X86_HAVE_MMX   := true
ARCH_X86_HAVE_SSE   := true
ARCH_X86_HAVE_SSE2  := true
ARCH_X86_HAVE_SSE3  := true
ARCH_X86_HAVE_SSSE3 := true
ARCH_X86_HAVE_MOVBE := true
ARCH_X86_HAVE_POPCNT := false
TARGET_GLOBAL_CFLAGS += \
                        -O2 \
                        -flto \
                        -march=atom \
                        -mmmx \
                        -m8bit-idiv \
                        -msse \
                        -msse2 \
                        -msse3 \
                        -mssse3 \
                        -mpclmul \
                        -mcx16 \
                        -msahf \
                        -mmovbe \
                        -ftree-vectorize \
                        -fomit-frame-pointer \
                        -finline-functions \
                        -fpredictive-commoning \
                        -fgcse-after-reload \
                        -fforce-addr \
                        -ffast-math \
                        -fsingle-precision-constant \
                        -floop-block \
                        -floop-interchange \
                        -floop-strip-mine \
                        -floop-parallelize-all \
                        -ftree-parallelize-loops=2 \
                        -ftree-loop-if-convert \
                        -ftree-loop-if-convert-stores \
                        -ftree-loop-distribution \
                        -foptimize-register-move \
                        -fgraphite-identity \

# The following are very specific to our Atom
TARGET_GLOBAL_CFLAGS += \
                        --param l1-cache-line-size=64 \
                        --param l1-cache-size=24 \
                        --param l2-cache-size=512 \

TARGET_GLOBAL_CFLAGS += -DUSE_SSSE3 -DUSE_SSE2
TARGET_GLOBAL_CPPFLAGS += -march=atom -fno-exceptions
TARGET_GLOBAL_LDFLAGS += -Wl,-O1

# We only have 1GB of RAM
MALLOC_SVELTE := true

# Intel ART optimized build flags (needs art-extension)
#VENDOR_ART_PATH := art-extension/art-extension
#BOARD_USES_DLMALLOC_META_CHUNK_FILTER := true
#WITH_DEXPREOPT_COMP := true
