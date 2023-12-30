#ifndef _SAMPLING_H_
#define _SAMPLING_H_

/* Sample from a discrete pdf. total is the sum of the
   (non scaled) probabilities, x_1 is re-adjusted to [0,1] after
   sampling, pdf for choosing index is filled in */
int SampleDiscrete(const float probabilities[], float total, double *x_1, double *pdf);
int DSampleDiscrete(const double probabilities[], double total, double *x_1, double *pdf);

#endif
