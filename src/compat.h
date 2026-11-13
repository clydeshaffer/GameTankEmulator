// The point of this file is to allow for GLIBC 2.39 support.
// Linux Mint and Ubuntu LTS 24.04 still use GLIBC 2.39, but this project
// forces the use of GLIBC 2.43 or 2.44. The reason is because of a few functions

// This file rebinds those functions to the ones used in GLIBC 2.39, if needed.
// This shouldn't have any effect on the emulation. The only tradeoff is that the last digit in a 32-bit float
// will be rounded up, which shouldn't be a problem for most people on the GameTank.

#ifndef __GLIBC_COMPAT_SYMBOL_H__
#define __GLIBC_COMPAT_SYMBOL_H__ 1

#if defined(__x86_64__) && defined(__GNU_LIBRARY__)

#include <features.h>

#if __GLIBC__ == 2 && __GLIBC_MINOR__ >= 43
__asm__(.symver sqrtf,sqrtf@GLIBC_2.2.5);
__asm__(.symver atan2f,atan2f@GLIBC_2.2.5);
__asm__(.symver acosf,acosf@GLIBC_2.2.5);
#endif

#if __GLIBC__ == 2 && __GLIBC_MINOR__ >= 44
__asm__(.symver sinh,sinh@GLIBC_2.2.5);
__asm__(.symver cosh,cosh@GLIBC_2.2.5);
#endif

#endif
#endif
