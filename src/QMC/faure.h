/* faure.h: Faure's QMC sequences + generalized Faure sequences */

#ifndef _FAURE_H_
#define _FAURE_H_

/* return sample with given index */
extern
double *Faure(int seed);

/* if NO_GRAY is defined, you can't mix NextFaure() and Faure() calls,
 * but Faure() will be faster because it doesn't need to convert to
 * seed to it's Gray code. */
extern
double *NextFaure();

/* initialize for Original Faure sequence */
extern
void
InitFaure(int dim);

/* initialize for generalized Faure sequence */
extern
void InitGFaure(int dim);

#endif
