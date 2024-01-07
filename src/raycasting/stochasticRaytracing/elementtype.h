/* elementtype.h: Monte Carlo radiosity element type */

#ifndef _ELEMENT_TYPE_H_
#define _ELEMENT_TYPE_H_

#include "common/linealAlgebra/Matrix2x2.h"
#include "QMC/niederreiter.h"
#include "raycasting/stochasticRaytracing/elementlistmcrad.h"
#include "raycasting/stochasticRaytracing/basismcrad.h"

typedef union PatchOrGeomPtr {
    PATCH *patch;
    Geometry *geom;
} PatchOrGeomPtr;

class ELEMENT {
  public:
    PatchOrGeomPtr pog;    /* pointer to patch for surface elements or
			 * to geometry for cluster elements */
    long id;        /* unique ID number */
    COLOR Ed, Rd;        /* average diffuse emittance and reflectance of element */

    niedindex ray_index;    /* incremented each time a ray is shot from the elem */

    float quality;    /* for merging the result of multiple iterations */
    float prob;        /* sampling probability */
    float ng;        /* nr of samples gathered on the patch */
    float area;        /* area of all surfaces contained in the element */

    GalerkinBasis *basis;        /* radiosity approximation data, see basis.h */
    /* higher order approximations need an array of color values for representing
     * radiance. */
    COLOR *rad, *unshot_rad, *received_rad;
    COLOR source_rad;    /* always constant source radiosity */

    float imp, unshot_imp, received_imp, source_imp; /* for view-importance driven sampling */
    niedindex imp_ray_index;    /* ray index for importance propagation */

    Vector3D midpoint;            /* element midpoint */
    Vertex *vertex[4];            /* up to 4 vertex pointers for
					 * surface elements */
    ELEMENT *parent;        /* parent element in hierarchy */
    ELEMENT **regular_subelements;    /* for surface elements with regular
					 * quadtree subdivision */
    ELEMENTLIST *irregular_subelements; /* clusters */
    Matrix2x2 *uptrans;            /* relates surface element (u,v)
					 * coordinates to patch (u,v)
					 * coordinates */
    signed char child_nr;            /* -1 for clusters or toplevel
					 * surface elements, 0..3 for
					 * regular surface subelements */
    char nrvertices;            /* nr of surf. element vertices */
    char iscluster;            /* whether it is a cluster or not */
    char flags;                /* unused so far */
};

#endif
