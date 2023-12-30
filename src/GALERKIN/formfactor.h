/* formfactor.h: all kind of formfactor computations */

#ifndef _FORMFACTOR_H_
#define _FORMFACTOR_H_

#include "scene/scene.h"
#include "GALERKIN/interaction.h"

/* Returns true if the two patches can "see" each other: P and Q see each 
 * other if at least a part of P is in front of Q and vice versa. */
extern int Facing(PATCH *P, PATCH *Q);

/*
 * Area (or volume) to area (or volume) form factor:
 *
 * IN: 	link->rcv, link->src, link->nrcv, link->nsrc: receiver and source element
 *	and the number of basis functions to consider on them.
 *	shadowlist: a list of possible occluders.
 * OUT: link->K, link->deltaK: generalized form factor(s) and error estimation
 *	coefficients (to be used in the refinement oracle EvaluateInteraction()
 *	in hierefine.c.
 *	link->crcv: number of error estimation coefficients (only 1 for the moment)
 *	link->vis: visibility factor: 255 for total visibility, 0 for total 
 *	occludedness
 *
 * The caller provides enough storage for storing the coefficients.
 *
 * Assumptions: 
 *
 * - The first basis function on the elements is constant and equal to 1.
 * - The basis functions are orthonormal on their reference domain (unit square or
 *   standard triangle).
 * - The parameter mapping on the elements is uniform.
 *
 * With these assumptions, the jacobians are all constant and equal to the area
 * of the elements and the basis overlap matrices are the area of the element times
 * the unit matrix. 
 *
 * Reference:
 *
 * - Ph. Bekaert, Y. D. Willems, "Error Control for Radiosity", Eurographics 
 * 	Rendering Workshop, Porto, Portugal, June 1996, pp 153--164.
 *
 * We always use a constant approximation on clusters. For the form factor
 * computations, a cluster area of one fourth of it's total surface area
 * is used. 
 *
 * Reference:
 *
 * - F. Sillion, "A Unified Hierarchical Algorithm for Global Illumination
 *	with Scattering Volumes and Object Clusters", IEEE TVCG Vol 1 Nr 3,
 *	sept 1995.
 */
extern unsigned AreaToAreaFormFactor(INTERACTION *link, GEOMLIST *shadowlist);

/* Adjusts the form factors for the patch so that their sum won't be
 * larger than one */

#endif