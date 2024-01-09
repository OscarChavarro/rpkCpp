#include <cstdlib>

#include "common/error.h"
#include "skin/Geometry.h"
#include "shared/render.h"
#include "shared/camera.h"
#include "IMAGE/tonemap/tonemapping.h"
#include "GALERKIN/elementgalerkin.h"
#include "GALERKIN/galerkinP.h"
#include "GALERKIN/basisgalerkin.h"
#include "GALERKIN/clustergalerkincpp.h"

// Element render modes, additive
#define OUTLINE    1
#define FLAT    2
#define GOURAUD 4
#define STRONG  8

static int nr_elements = 0;
static int nr_clusters = 0;

/**
Orientation and position of regular sub-elements is fully determined by the
following transformations. A uniform mapping of parameter domain to the
elements is supposed (i.o.w. use patchUniformPoint() to map (u,v) coordinates
on the toplevel element to a 3D point on the patch). The sub-elements
have equal area. No explicit Jacobian stuff needed to compute integrals etc..
etc
*/

/* up-transforms for regular quadrilateral sub-elements:
 *
 *   (v)
 *
 *    1 +---------+---------+
 *      |         |         |
 *      |         |         |
 *      | 3       | 4       |
 *  0.5 +---------+---------+
 *      |         |         |
 *      |         |         |
 *      | 1       | 2       |
 *    0 +---------+---------+
 *      0        0.5        1   (u)
 */
Matrix2x2 quadupxfm[4] = {
        /* south-west [0,0.5] x [0,0.5] */
        {{{0.5, 0.0}, {0.0, 0.5}}, {0.0, 0.0}},

        /* south-east: [0.5,1] x [0,0.5] */
        {{{0.5, 0.0}, {0.0, 0.5}}, {0.5, 0.0}},

        /* north-west: [0,0.5] x [0.5,1] */
        {{{0.5, 0.0}, {0.0, 0.5}}, {0.0, 0.5}},

        /* north-east: [0.5,1] x [0.5,1] */
        {{{0.5, 0.0}, {0.0, 0.5}}, {0.5, 0.5}},
};

/* up-transforms for regular triangular subelements:
 *
 *  (v)
 *
 *   1 +
 *     | \
 *     |   \
 *     |     \
 *     | 3     \
 * 0.5 +---------+
 *     | \     4 | \
 *     |   \     |   \
 *     |     \   |     \
 *     | 1     \ | 2     \
 *   0 +---------+---------+
 *     0        0.5        1  (u)
 */
Matrix2x2 triupxfm[4] = {
        /* left: (0,0),(0.5,0),(0,0.5) */
        {{{0.5,  0.0}, {0.0, 0.5}},  {0.0, 0.0}},

        /* right: (0.5,0),(1,0),(0.5,0.5) */
        {{{0.5,  0.0}, {0.0, 0.5}},  {0.5, 0.0}},

        /* top: (0,0.5),(0.5,0.5),(0,1) */
        {{{0.5,  0.0}, {0.0, 0.5}},  {0.0, 0.5}},

        /* middle: (0.5,0.5),(0,0.5),(0.5,0) */
        {{{-0.5, 0.0}, {0.0, -0.5}}, {0.5, 0.5}},
};

/* returns the total number of elements in use */
int
GetNumberOfElements() {
    return nr_elements;
}

int
GetNumberOfClusters() {
    return nr_clusters;
}

int
GetNumberOfSurfaceElements() {
    return nr_elements - nr_clusters;
}

/* (re)allocates storage for the coefficients to represent radiance, received radiance
 * and unshot radiance on the element. */
void
ElementReallocCoefficients(ELEMENT *elem) {
    COLOR *radiance, *received_radiance, *unshot_radiance;
    int basis_size = 0;

    if ( isCluster(elem)) {
        /* we always use a constant basis on cluster elements. */
        basis_size = 1;
    } else {
        switch ( GLOBAL_galerkin_state.basis_type ) {
            case CONSTANT:
                basis_size = 1;
                break;
            case LINEAR:
                basis_size = 3;
                break;
            case QUADRATIC:
                basis_size = 6;
                break;
            case CUBIC:
                basis_size = 10;
                break;
            default:
                logFatal(-1, "ElementReallocCoefficients", "Invalid basis type %d", GLOBAL_galerkin_state.basis_type);
        }
    }

    radiance = (COLOR *)malloc(basis_size * sizeof(COLOR));
    clusterGalerkinClearCoefficients(radiance, basis_size);
    if ( elem->radiance ) {
        clusterGalerkinCopyCoefficients(radiance, elem->radiance, MIN(elem->basis_size, basis_size));
        free(elem->radiance);
    }
    elem->radiance = radiance;

    received_radiance = (COLOR *)malloc(basis_size * sizeof(COLOR));
    clusterGalerkinClearCoefficients(received_radiance, basis_size);
    if ( elem->received_radiance ) {
        clusterGalerkinCopyCoefficients(received_radiance, elem->received_radiance, MIN(elem->basis_size, basis_size));
        free(elem->received_radiance);
    }
    elem->received_radiance = received_radiance;

    if ( GLOBAL_galerkin_state.iteration_method == SOUTHWELL ) {
        unshot_radiance = (COLOR *)malloc(basis_size * sizeof(COLOR));
        clusterGalerkinClearCoefficients(unshot_radiance, basis_size);
        if ( !isCluster(elem)) {
            if ( elem->unshot_radiance ) {
                clusterGalerkinCopyCoefficients(unshot_radiance, elem->unshot_radiance,
                                                MIN(elem->basis_size, basis_size));
                free(elem->unshot_radiance);
            } else if ( elem->pog.patch->surface ) {
                unshot_radiance[0] = SELFEMITTED_RADIANCE(elem->pog.patch);
            }
        }
        elem->unshot_radiance = unshot_radiance;
    } else {
        if ( elem->unshot_radiance ) {
            free(elem->unshot_radiance);
        }
        elem->unshot_radiance = (COLOR *) nullptr;
    }

    elem->basis_size = basis_size;
    if ( elem->basis_used > elem->basis_size ) {
        elem->basis_used = elem->basis_size;
    }
}

/**
Use either galerkinCreateToplevelElement() or CreateRegularSubelement()
*/
static ELEMENT *
galerkinCreateElement() {
    ELEMENT *element = (ELEMENT *)malloc(sizeof(ELEMENT));

    colorClear(element->Ed);
    colorClear(element->Rd);
    element->id = nr_elements + 1;    /* let the IDs start from 1, not 0 */
    element->radiance = element->received_radiance = element->unshot_radiance = (COLOR *) nullptr;
    element->potential.f = element->received_potential.f = element->unshot_potential.f = 0.;
    element->interactions = InteractionListCreate();
    element->pog.patch = (Patch *) nullptr;
    element->pog.geom = (Geometry *) nullptr;
    element->parent = (ELEMENT *) nullptr;
    element->regular_subelements = (ELEMENT **) nullptr;
    element->irregular_subelements = ElementListCreate();
    element->uptrans = (Matrix2x2 *) nullptr;
    element->area = 0.;
    element->flags = 0x00;
    element->childnr = -1;    /* means: "not a regular subelement" */
    element->basis_size = 0;
    element->basis_used = 0;
    element->nrpatches = 1;    /* correct for surface elements and it will be
				 * computed later for clusters */
    element->minarea = HUGE;
    element->tmp = 0;
    element->bsize = 0.;        /* correct eq. blocker size will be computer later on */

    nr_elements++;
    return element;
}

/**
Creates the toplevel element for the patch
*/
ELEMENT *
galerkinCreateToplevelElement(Patch *patch) {
    ELEMENT *element = galerkinCreateElement();
    element->pog.patch = patch;
    element->minarea = element->area = patch->area;
    element->bsize = 2.0f * (float)std::sqrt(element->area / M_PI);
    element->direct_potential.f = patch->directPotential;

    element->Rd = patchAverageNormalAlbedo(patch, BRDF_DIFFUSE_COMPONENT);
    if ( patch->surface && patch->surface->material &&
         patch->surface->material->edf ) {
        element->flags |= IS_LIGHT_SOURCE;
        element->Ed = patchAverageEmittance(patch, DIFFUSE_COMPONENT);
        colorScaleInverse(M_PI, element->Ed, element->Ed);
    }

    patch->radiance_data = element;
    ElementReallocCoefficients(element);

    return element;
}

/**
Creates a cluster element for the given geometry
 */
ELEMENT *
galerkinCreateClusterElement(Geometry *geometry) {
    ELEMENT *element = galerkinCreateElement();
    element->pog.geom = geometry;
    element->area = 0.;    /* needs to be computed after the whole cluster
			 * hierarchy has been constructed */
    element->flags |= IS_CLUSTER;
    ElementReallocCoefficients(element);

    colorSetMonochrome(element->Rd, 1.);

    /* whether the cluster contains light sources or not is also determined
     * after the hierarchy is constructed */
    nr_clusters++;
    return element;
}

/**
Regularly subdivides the given element. A pointer to an array of
4 pointers to sub-elements is returned
*/
ELEMENT **
galerkinRegularSubdivideElement(ELEMENT *element) {
    ELEMENT **subElement;
    int i;

    if ( isCluster(element)) {
        logFatal(-1, "galerkinRegularSubdivideElement", "Cannot regularly subdivide cluster elements");
        return (ELEMENT **) nullptr;
    }

    if ( element->regular_subelements ) {
        return element->regular_subelements;
    }

    subElement = (ELEMENT **)malloc(4 * sizeof(ELEMENT *));

    for ( i = 0; i < 4; i++ ) {
        subElement[i] = galerkinCreateElement();
        subElement[i]->pog.patch = element->pog.patch;
        subElement[i]->parent = element;
        subElement[i]->uptrans =
                element->pog.patch->numberOfVertices == 3 ? &triupxfm[i] : &quadupxfm[i];
        subElement[i]->area = 0.25f * element->area;  /* we always use a uniform mapping */
        subElement[i]->bsize = 2.0f * (float)std::sqrt(subElement[i]->area / M_PI);
        subElement[i]->childnr = (char)i;
        ElementReallocCoefficients(subElement[i]);

        basisGalerkinPush(element, element->radiance, subElement[i], subElement[i]->radiance);

        subElement[i]->potential.f = element->potential.f;
        subElement[i]->direct_potential.f = element->direct_potential.f;

        if ( GLOBAL_galerkin_state.iteration_method == SOUTHWELL ) {
            basisGalerkinPush(element, element->unshot_radiance, subElement[i], subElement[i]->unshot_radiance);
            subElement[i]->unshot_potential.f = element->unshot_potential.f;
        }

        subElement[i]->flags |= (element->flags & IS_LIGHT_SOURCE);

        subElement[i]->Rd = element->Rd;
        subElement[i]->Ed = element->Ed;

        renderSetColor(&renderopts.outline_color);
        DrawElementOutline(subElement[i]);
    }

    return (element->regular_subelements = subElement);
}

static void
galerkinDoDestroyElement(ELEMENT *element) {
    InteractionListIterate(element->interactions, InteractionDestroy);
    InteractionListDestroy(element->interactions);

    if ( element->radiance ) {
        free(element->radiance);
    }
    if ( element->received_radiance ) {
        free(element->received_radiance);
    }
    if ( element->unshot_radiance ) {
        free(element->unshot_radiance);
    }
    free(element);
    nr_elements--;
}

/* destroys the toplevel surface element and it's subelements (recursive) */
void
galerkinDestroyToplevelElement(ELEMENT *element) {
    if ( !element ) {
        return;
    }

    if ( element->regular_subelements ) {
        ITERATE_REGULAR_SUBELEMENTS(element, galerkinDestroyToplevelElement);
        free(element->regular_subelements);
    }

    if ( element->irregular_subelements ) {
        ITERATE_IRREGULAR_SUBELEMENTS(element, galerkinDestroyToplevelElement);
        ElementListDestroy(element->irregular_subelements);
    }

    galerkinDoDestroyElement(element);
}

/* destroys the cluster element, not recursive. */
void
galerkinDestroyClusterElement(ELEMENT *element) {
    galerkinDoDestroyElement(element);
    nr_clusters--;
}

/* prints the element data to the file 'out' */
void
galerkinPrintElement(FILE *out, ELEMENT *element) {
    fprintf(out, "Element %d: ", element->id);
    if ( isCluster(element)) {
        fprintf(out, "cluster element, ");
    } else {
        fprintf(out, "belongs to patch ID %d, ",
                element->pog.patch ? element->pog.patch->id : -1);
    }
    fprintf(out, "parent element ID = %d, child nr = %d\n",
            element->parent ? element->parent->id : -1,
            element->childnr);
    fprintf(out, "area = %g, bsize = %g, basis size = %d\n",
            element->area, element->bsize, element->basis_size);

    if ( element->uptrans ) {
        fprintf(out, "up-transform:\n");
        PRINT_TRANSFORM2D(out, *element->uptrans);
    } else {
        fprintf(out, "no up-transform.\n");
    }

    if ( element->radiance ) {
        fprintf(out, "radiance = ");
        printCoefficients(out, element->radiance, element->basis_size);
        fprintf(out, "\n");
    } else {
        fprintf(out, "No radiance coefficients.\n");
    }

    if ( element->received_radiance ) {
        fprintf(out, "received_radiance = ");
        printCoefficients(out, element->received_radiance, element->basis_size);
        fprintf(out, "\n");
    } else {
        fprintf(out, "No received_radiance coefficients.\n");
    }

    if ( element->unshot_radiance ) {
        fprintf(out, "unshot_radiance = ");
        printCoefficients(out, element->unshot_radiance, element->basis_size);
        fprintf(out, "\n");
    } else {
        fprintf(out, "No unshot_radiance coefficients.\n");
    }

    fprintf(out, "potential.f = %g, received_potential.f = %g, unshot_potential.f = %g, directPotential = %g\n",
            element->potential.f, element->received_potential.f, element->unshot_potential.f,
            element->direct_potential.f);

    fprintf(out, "interactions_created = %s",
            element->flags & INTERACTIONS_CREATED ? "TRUE" : "FALSE");

    if ( element->interactions ) {
        fprintf(out, ", interactions:\n");
        InteractionListIterate1B(element->interactions, InteractionPrint, out);
    } else {
        fprintf(out, ", no interactions.\n");
    }

    if ( element->regular_subelements ) {
        int i;
        fprintf(out, "regular subelements: ");
        for ( i = 0; i < 4; i++ ) {
            fprintf(out, "%d, ", element->regular_subelements[i]->id);
        }
        fprintf(out, "\n");
    } else {
        fprintf(out, "No regular subelements.\n");
    }

    if ( element->irregular_subelements ) {
        ELEMENTLIST *elist;
        fprintf(out, "irregular subelements: ");
        for ( elist = element->irregular_subelements; elist; elist = elist->next ) {
            fprintf(out, "%d, ", elist->element->id);
        }
        fprintf(out, "\n");
    } else {
        fprintf(out, "No irregular subelements.\n");
    }
}

/* prints the patch id and the child numbers of the element and its parents. */
void
galerkinPrintElementId(FILE *out, ELEMENT *elem) {
    if ( isCluster(elem)) {
        fprintf(out, "geom %d cluster", elem->pog.geom->id);
    } else {
        if ( elem->uptrans ) {
            galerkinPrintElementId(out, elem->parent);
            fprintf(out, "%d", elem->childnr + 1);
        } else {
            fprintf(out, "patch %d element ", elem->pog.patch->id);
        }
    }
}

/* Computes the transform relating a surface element to the toplevel element 
 * in the patch hierarchy by concatenaing the up-transforms of the element 
 * and all parent alements. If the element is a toplevel element, 
 * (Matrix4x4 *)nullptr is
 * returned and nothing is filled in in xf (no trnasform is necessary
 * to transform positions on the element to the corresponding point on the toplevel
 * element). In the other case, the composed transform is filled in in xf and
 * xf (pointer to the transform) is returned. */
Matrix2x2 *
galerkinElementToTopTransform(ELEMENT *element, Matrix2x2 *xf) {
    /* toplevel element: no transform necessary to transform to top */
    if ( !element->uptrans ) {
        return (Matrix2x2 *) nullptr;
    }

    *xf = *element->uptrans;
    while ((element = element->parent) && element->uptrans ) {
        PRECONCAT_TRANSFORM2D(*element->uptrans, *xf, *xf);
    }

    return xf;
}

/* Determines the regular subelement at point (u,v) of the given parent
 * element. Returns the parent element itself if there are no regular subelements. 
 * The point is transformed to the corresponding point on the subelement. */
ELEMENT *
galerkinRegularSubelementAtPoint(ELEMENT *parent, double *u, double *v) {
    ELEMENT *child = (ELEMENT *) nullptr;
    double _u = *u, _v = *v;

    if ( isCluster(parent) || !parent->regular_subelements ) {
        return parent;
    }

    /* Have a look at the drawings above to understand what is done exactly. */
    switch ( parent->pog.patch->numberOfVertices ) {
        case 3:
            if ( _u + _v <= 0.5 ) {
                child = parent->regular_subelements[0];
                *u = _u * 2.;
                *v = _v * 2.;
            } else if ( _u > 0.5 ) {
                child = parent->regular_subelements[1];
                *u = (_u - 0.5) * 2.;
                *v = _v * 2.;
            } else if ( _v > 0.5 ) {
                child = parent->regular_subelements[2];
                *u = _u * 2.;
                *v = (_v - 0.5) * 2.;
            } else {
                child = parent->regular_subelements[3];
                *u = (0.5 - _u) * 2.;
                *v = (0.5 - _v) * 2.;
            }
            break;
        case 4:
            if ( _v <= 0.5 ) {
                if ( _u < 0.5 ) {
                    child = parent->regular_subelements[0];
                    *u = _u * 2.;
                } else {
                    child = parent->regular_subelements[1];
                    *u = (_u - 0.5) * 2.;
                }
                *v = _v * 2.;
            } else {
                if ( _u < 0.5 ) {
                    child = parent->regular_subelements[2];
                    *u = _u * 2.;
                } else {
                    child = parent->regular_subelements[3];
                    *u = (_u - 0.5) * 2.;
                }
                *v = (_v - 0.5) * 2.;
            }
            break;
        default:
            logFatal(-1, "galerkinRegularSubelementAtPoint", "Can handle only triangular or quadrilateral elements");
    }

    return child;
}

/* Returns the leaf regular subelement of 'top' at the point (u,v) (uniform 
 * coordinates!). (u,v) is transformed to the coordinates of the corresponding
 * point on the leaf element. 'top' is a surface element, not a cluster. */
ELEMENT *
RegularLeafElementAtPoint(ELEMENT *top, double *u, double *v) {
    ELEMENT *leaf;

    /* find leaf element of 'top' at (u,v) */
    leaf = top;
    while ( leaf->regular_subelements ) {
        leaf = galerkinRegularSubelementAtPoint(leaf, u, v);
    }

    return leaf;
}

/* Computes the vertices of a surface element (3 or 4 vertices) or
 * cluster element (8 vertices). The number of vertices is returned. */
int
ElementVertices(ELEMENT *elem, Vector3D *p) {
    if ( isCluster(elem)) {
        BOUNDINGBOX vol;

        ElementBounds(elem, vol);

        VECTORSET(p[0], vol[MIN_X], vol[MIN_Y], vol[MIN_Z]);
        VECTORSET(p[1], vol[MIN_X], vol[MIN_Y], vol[MAX_Z]);
        VECTORSET(p[2], vol[MIN_X], vol[MAX_Y], vol[MIN_Z]);
        VECTORSET(p[3], vol[MIN_X], vol[MAX_Y], vol[MAX_Z]);
        VECTORSET(p[4], vol[MAX_X], vol[MIN_Y], vol[MIN_Z]);
        VECTORSET(p[5], vol[MAX_X], vol[MIN_Y], vol[MAX_Z]);
        VECTORSET(p[6], vol[MAX_X], vol[MAX_Y], vol[MIN_Z]);
        VECTORSET(p[7], vol[MAX_X], vol[MAX_Y], vol[MAX_Z]);

        return 8;
    } else {
        Matrix2x2 toptrans;
        Vector2D uv;

        if ( elem->uptrans ) {
            galerkinElementToTopTransform(elem, &toptrans);
        }

        uv.u = 0.;
        uv.v = 0.;
        if ( elem->uptrans ) TRANSFORM_POINT_2D(toptrans, uv, uv);
        patchUniformPoint(elem->pog.patch, uv.u, uv.v, &p[0]);

        uv.u = 1.;
        uv.v = 0.;
        if ( elem->uptrans ) TRANSFORM_POINT_2D(toptrans, uv, uv);
        patchUniformPoint(elem->pog.patch, uv.u, uv.v, &p[1]);

        if ( elem->pog.patch->numberOfVertices == 4 ) {
            uv.u = 1.;
            uv.v = 1.;
            if ( elem->uptrans ) TRANSFORM_POINT_2D(toptrans, uv, uv);
            patchUniformPoint(elem->pog.patch, uv.u, uv.v, &p[2]);

            uv.u = 0.;
            uv.v = 1.;
            if ( elem->uptrans ) TRANSFORM_POINT_2D(toptrans, uv, uv);
            patchUniformPoint(elem->pog.patch, uv.u, uv.v, &p[3]);
        } else {
            uv.u = 0.;
            uv.v = 1.;
            if ( elem->uptrans ) TRANSFORM_POINT_2D(toptrans, uv, uv);
            patchUniformPoint(elem->pog.patch, uv.u, uv.v, &p[2]);

            VECTORSET(p[3], 0., 0., 0.);
        }

        return elem->pog.patch->numberOfVertices;
    }
}

/* Computes the midpoint of the element. */
Vector3D
galerkinElementMidpoint(ELEMENT *elem) {
    Vector3D c;

    if ( isCluster(elem)) {
        float *bbox = geomBounds(elem->pog.geom);

        VECTORSET(c,
                  (bbox[MIN_X] + bbox[MAX_X]) / 2.,
                  (bbox[MIN_Y] + bbox[MAX_Y]) / 2.,
                  (bbox[MIN_Z] + bbox[MAX_Z]) / 2.);
    } else {
        Vector3D p[4];
        int i, nrverts;

        nrverts = ElementVertices(elem, p);

        VECTORSET(c, 0., 0., 0.);
        for ( i = 0; i < nrverts; i++ )
                VECTORADD (c, p[i], c);
        VECTORSCALE((1. / (float) nrverts), c, c);
    }

    return c;
}

/* Computes a bounding box for the element. */
float *
ElementBounds(ELEMENT *elem, float *bounds) {
    if ( isCluster(elem)) {
        boundsCopy(geomBounds(elem->pog.geom), bounds);
    } else {
        Vector3D p[4];
        int i, nrverts;

        nrverts = ElementVertices(elem, p);

        boundsInit(bounds);
        for ( i = 0; i < nrverts; i++ ) {
            boundsEnlargePoint(bounds, &p[i]);
        }
    }

    return bounds;
}

/* Computes a polygon description for shaft culling for the surface 
 * element. Cannot be used for clusters. */
POLYGON *
ElementPolygon(ELEMENT *elem, POLYGON *poly) {
    int i;

    if ( isCluster(elem)) {
        logFatal(-1, "ElementPolygon", "Cannot use this function for cluster elements");
        return (POLYGON *) nullptr;
    }

    poly->normal = elem->pog.patch->normal;
    poly->plane_constant = elem->pog.patch->planeConstant;
    poly->index = elem->pog.patch->index;
    poly->nrvertices = ElementVertices(elem, poly->vertex);

    boundsInit(poly->bounds);
    for ( i = 0; i < poly->nrvertices; i++ ) {
        boundsEnlargePoint(poly->bounds, &poly->vertex[i]);
    }

    return poly;
}

static void
DrawElement(ELEMENT *element, int mode) {
    Vector3D p[4];
    int nrverts;

    if ( isCluster(element)) {
        if ( mode & OUTLINE || mode & STRONG ) {
            RenderBounds(geomBounds(element->pog.geom));
        }
        return;
    }

    nrverts = ElementVertices(element, p);

    if ( mode & FLAT ) {
        RGB color;
        COLOR rho = REFLECTIVITY(element->pog.patch);

        if ( GLOBAL_galerkin_state.use_ambient_radiance ) {
            COLOR rad_vis;
            colorProduct(rho, GLOBAL_galerkin_state.ambient_radiance, rad_vis);
            colorAdd(rad_vis, element->radiance[0], rad_vis);
            radianceToRgb(rad_vis, &color);
        } else {
            radianceToRgb(element->radiance[0], &color);
        }
        renderSetColor(&color);
        renderPolygonFlat(nrverts, p);
    } else if ( mode & GOURAUD ) {
        RGB vertcol[4];
        COLOR vertrad[4];
        int i;

        if ( nrverts == 3 ) {
            vertrad[0] = basisGalerkinRadianceAtPoint(element, element->radiance, 0., 0.);
            vertrad[1] = basisGalerkinRadianceAtPoint(element, element->radiance, 1., 0.);
            vertrad[2] = basisGalerkinRadianceAtPoint(element, element->radiance, 0., 1.);
        } else {
            vertrad[0] = basisGalerkinRadianceAtPoint(element, element->radiance, 0., 0.);
            vertrad[1] = basisGalerkinRadianceAtPoint(element, element->radiance, 1., 0.);
            vertrad[2] = basisGalerkinRadianceAtPoint(element, element->radiance, 1., 1.);
            vertrad[3] = basisGalerkinRadianceAtPoint(element, element->radiance, 0., 1.);
        }

        if ( GLOBAL_galerkin_state.use_ambient_radiance ) {
            COLOR rho = REFLECTIVITY(element->pog.patch), ambient;

            colorProduct(rho, GLOBAL_galerkin_state.ambient_radiance, ambient);
            for ( i = 0; i < nrverts; i++ ) {
                colorAdd(vertrad[i], ambient, vertrad[i]);
            }
        }

        for ( i = 0; i < nrverts; i++ ) {
            radianceToRgb(vertrad[i], &vertcol[i]);
        }

        renderPolygonGouraud(nrverts, p, vertcol);
    }

    /* modifies the positions, that's why it comes last */
    if ( mode & OUTLINE ) {
        int i;

        for ( i = 0; i < nrverts; i++ ) {
            /* move the point a bit closer the the eye point to avoid aliasing */
            Vector3D d;
            VECTORSUBTRACT(GLOBAL_camera_mainCamera.eyep, p[i], d);
            VECTORSUMSCALED(p[i], 0.01, d, p[i]);
        }

        renderSetColor(&renderopts.outline_color);
        renderLine(&p[0], &p[1]);
        renderLine(&p[1], &p[2]);
        if ( nrverts == 3 ) {
            renderLine(&p[2], &p[0]);
        } else {
            renderLine(&p[2], &p[3]);
            renderLine(&p[3], &p[0]);
        }

        if ( mode & STRONG ) {
            if ( nrverts == 3 ) {
                Vector3D d, pt;

                VECTORSUBTRACT(p[2], p[1], d);

                for ( i = 1; i < 4; i++ ) {
                    VECTORSUMSCALED(p[1], i * 0.25, d, pt);
                    renderLine(&p[0], &pt);
                }
            } else if ( nrverts == 4 ) {
                Vector3D d1, d2, p1, p2;

                VECTORSUBTRACT(p[1], p[0], d1);
                VECTORSUBTRACT(p[3], p[2], d2);


                for ( i = 0; i < 5; i++ ) {
                    VECTORSUMSCALED(p[0], i * 0.25, d1, p1);
                    VECTORSUMSCALED(p[2], (1.0 - i * 0.25), d2, p2);
                    renderLine(&p1, &p2);
                }
            }
        }
    }
}

void
DrawElementOutline(ELEMENT *elem) {
    DrawElement(elem, OUTLINE);
}

void
RenderElement(ELEMENT *elem) {
    int rendercode = 0;

    if ( renderopts.draw_outlines ) {
        rendercode |= OUTLINE;
    }

    if ( renderopts.smooth_shading ) {
        rendercode |= GOURAUD;
    } else {
        rendercode |= FLAT;
    }

    DrawElement(elem, rendercode);
}

void
ForAllLeafElements(ELEMENT *top, void (*func)(ELEMENT *)) {
    ForAllIrregularSubelements(child, top)
                {
                    ForAllLeafElements(child, func);
                }
    EndForAll;

    ForAllRegularSubelements(child, top)
                {
                    ForAllLeafElements(child, func);
                }
    EndForAll;

    if ( !top->irregular_subelements && !top->regular_subelements ) {
        func(top);
    }
}
