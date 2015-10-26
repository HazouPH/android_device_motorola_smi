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
                        -march=bonnel \
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

# Use Intel libm
USE_PRIVATE_LIBM := true

# Dalvik with houdini
INTEL_HOUDINI := true
WITH_JIT := true
LOCAL_CFLAGS += -DARCH_IA32
WITH_SELF_VERIFICATION := true
TARGET_ARCH_LOWMEM := true

# x86 Dalvik opts, only for intel BSP dalvik
WITH_PCG := true
WITH_CONDMARK := true
WITH_TLA := true
WITH_REGION_GC := true
WITH_JIT_TUNING := tue
USE_INTEL_IPP := true

# customize the malloced address to be 16-byte aligned
BOARD_MALLOC_ALIGNMENT := 16

ifdef INTEL_HOUDINI
TARGET_CPU_ABI2 := armeabi-v7a
ADDITIONAL_BUILD_PROPERTIES += ro.product.cpu.upgradeabi=armeabi
ADDITIONAL_BUILD_PROPERTIES += dalvik.vm.houdini=on
endif
