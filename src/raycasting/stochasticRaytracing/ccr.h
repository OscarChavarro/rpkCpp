/* ccr.h: Constant Control Radiosity */

#ifndef _RPK_CCR_H_
#define _RPK_CCR_H_

/* determines and returns optimal constant control radiosity value for
 * the given radiance distribution: this is, the value of beta that 
 * minimises F(beta) = sum over all patches P of P->area times 
 * absolute value of (globalGetRadiance(P) - globalGetScaling(P) * beta).
 *
 * - getRadiance() returns the radiance to be propagated from a
 * given ELEMENT. 
 * - getScaling() returns a scale factor (per color component) to be
 * multiplied with the radiance of the element. If getScaling is a nullptr
 * pointer, no scaling is applied. Scaling is used in the context of 
 * random walk radiosity (see randwalk.c). */
extern COLOR determineControlRadiosity(COLOR *(*getRadiance)(ELEMENT *),
                                       COLOR (*getScaling)(ELEMENT *));

#endif
