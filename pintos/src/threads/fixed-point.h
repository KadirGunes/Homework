#ifndef THREADS_FIXED_POINT_H
#define THREADS_FIXED_POINT_H

#include <stdint.h>

/* 17.14 fixed point formatı
   En soldaki 17 bit = tam sayı kısmı
   En sağdaki 14 bit = kesirli kısım */

#define FP (1 << 14)   /* 2^14 = 16384 */

/* int → fixed point(dönüştürücü)*/
#define FP_FROM_INT(n)       ((n) * FP)

/* fixed point → int (aşağı yuvarla) */
#define FP_TO_INT(x)         ((x) / FP)

/* fixed point → int (en yakına yuvarla) */
#define FP_TO_INT_ROUND(x)   ((x) >= 0 ? ((x) + FP / 2) / FP : ((x) - FP / 2) / FP)

/* fixed point + fixed point */
#define FP_ADD(x, y)         ((x) + (y))

/* fixed point - fixed point */
#define FP_SUB(x, y)         ((x) - (y))

/* fixed point + int */
#define FP_ADD_INT(x, n)     ((x) + (n) * FP)

/* fixed point - int */
#define FP_SUB_INT(x, n)     ((x) - (n) * FP)

/* fixed point * fixed point */
#define FP_MUL(x, y)         ((int64_t)(x) * (y) / FP)

/* fixed point * int */
#define FP_MUL_INT(x, n)     ((x) * (n))

/* fixed point / fixed point */
#define FP_DIV(x, y)         ((int64_t)(x) * FP / (y))

/* fixed point / int */
#define FP_DIV_INT(x, n)     ((x) / (n))

#endif 
