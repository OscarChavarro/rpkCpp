#include "common/error.h"
#include "material/statistics.h"
#include "skin/Patch.h"
#include "skin/Geometry.h"
#include "shared/shadowcaching.h"
#include "GALERKIN/formfactor.h"
#include "GALERKIN/galerkinP.h"
#include "GALERKIN/basisgalerkin.h"
#include "GALERKIN/mrvisibility.h"

/* Returns true if P is at least partly in front the plane of Q. Returns false
 * if P is coplanar with or behind Q. It suffices to test the vertices of P w.r.t. 
 * the plane of Q. */
int IsAtLeastPartlyInFront(PATCH *P, PATCH *Q) {
    int i;

    for ( i = 0; i < P->nrvertices; i++ ) {
        Vector3D *vp = P->vertex[i]->point;
        double ep = VECTORDOTPRODUCT(Q->normal, *vp) + Q->plane_constant,
                tolp = Q->tolerance + VECTORTOLERANCE(*vp);
        if ( ep > tolp ) {    /* P is at least partly in front of Q */
            return true;
        }
    }
    return false;        /* P is behind or coplanar with Q */
}

/* Returns true if the two patches can "see" each other: P and Q see each 
 * other if at least a part of P is in front of Q and vice versa. */
int Facing(PATCH *P, PATCH *Q) {
    return (IsAtLeastPartlyInFront(P, Q) && IsAtLeastPartlyInFront(Q, P));
}

/* This routine determines the cubature rule to be used on the given element
 * and computes the positions on the elements patch that correspond to the nodes
 * of the cubature rule on the element. The role (RECEIVER or SOURCE) is only
 * relevant for surface elements. */
static void DetermineNodes(ELEMENT *elem, CUBARULE **cr, Vector3D x[CUBAMAXNODES], ROLE role) {
    Matrix2x2 topxf;
    int k;

    if ( IsCluster(elem)) {
        BOUNDINGBOX vol;
        double dx, dy, dz;

        *cr = GLOBAL_galerkin_state.clusRule;

        ElementBounds(elem, vol);
        dx = vol[MAX_X] - vol[MIN_X];
        dy = vol[MAX_Y] - vol[MIN_Y];
        dz = vol[MAX_Z] - vol[MIN_Z];
        for ( k = 0; k < (*cr)->nrnodes; k++ ) {
            VECTORSET(x[k],
                      vol[MIN_X] + (*cr)->u[k] * dx,
                      vol[MIN_Y] + (*cr)->v[k] * dy,
                      vol[MIN_Z] + (*cr)->t[k] * dz);
        }
    } else {
        /* what cubature rule should be used over the element */
        switch ( elem->pog.patch->nrvertices ) {
            case 3:
                *cr = role == RECEIVER ? GLOBAL_galerkin_state.rcv3rule : GLOBAL_galerkin_state.src3rule;
                break;
            case 4:
                *cr = role == RECEIVER ? GLOBAL_galerkin_state.rcv4rule : GLOBAL_galerkin_state.src4rule;
                break;
            default:
                Fatal(4, "DetermineNodes", "Can only handle triangular and quadrilateral patches");
        }

        /* compute the transform relating positions on the element to positions on
         * the patch to which it belongs. */
        if ( elem->uptrans ) {
            ElementToTopTransform(elem, &topxf);
        }

        /* compute the positions x[k] corresponding to the nodes of the cubature rule
         * in the unit square or triangle used to parametrise the element. */
        for ( k = 0; k < (*cr)->nrnodes; k++ ) {
            Vector2D node;
            node.u = (*cr)->u[k];
            node.v = (*cr)->v[k];
            if ( elem->uptrans ) TRANSFORM_POINT_2D(topxf, node, node);
            PatchUniformPoint(elem->pog.patch, node.u, node.v, &x[k]);
        }
    }
}

/* Evaluates the radiosity kernel (*not* taking into account the reflectivity of
 * the receiver) at positions x on the receiver rcv and y on
 * source src. ShadowList is a list of potential occluders.
 * Shadowcaching is used to speed up the visibility detection
 * between x and y. The shadow cache is initialized before doing the first
 * evaluation of the kernel between each pair of patches. The unoccluded
 * point-to-point form factor is returned. Visibility is filled in vis. */
/* to be used when integrating over the surface of the source element */
static double PointKernelEval(Vector3D *x, Vector3D *y,
                              ELEMENT *rcv, ELEMENT *src, GeometryListNode *ShadowList,
                              double *vis) {
    double dist, cosp, cosq, ff;
    float fdist;
    Ray ray;
    HITREC hitstore;

    /* Trace the ray from source to receiver (y to x) to handle one-sided surfaces
     * correctly. */
    ray.pos = *y;
    VECTORSUBTRACT(*x, *y, ray.dir);
    dist = VECTORNORM(ray.dir);
    VECTORSCALEINVERSE(dist, ray.dir, ray.dir);

    /* don't allow too nearby nodes to interact */
    if ( dist < EPSILON ) {
        Warning("PointKernelEval", "Nodes too close too each other (receiver id %d, source id %d)", rcv->id, src->id);
        return 0.;
    }

    /* emitter factor times scale, see Sillion, "A Unified Hierarchical ...",
     * IEEE TVCG Vol 1 nr 3, sept 1995 */
    if ( IsCluster(src)) {
        cosq = 0.25;
    } else {
        cosq = VECTORDOTPRODUCT(ray.dir, src->pog.patch->normal);
        if ( cosq <= 0. ) {    /* ray leaves behind the source */
            return 0.;
        }
    }

    /* receiver factor times scale */
    if ( IsCluster(rcv)) {
        cosp = 0.25;
    } else {
        cosp = -VECTORDOTPRODUCT(ray.dir, rcv->pog.patch->normal);
        if ( cosp <= 0. ) {    /* ray hits receiver from the back */
            return 0.;
        }
    }

    /* unoccluded kernel value */
    ff = cosp * cosq / (M_PI * dist * dist);
    fdist = dist * (1. - EPSILON);

    /* determine transmissivity (visibility) */
    if ( !ShadowList ) {
        *vis = 1.;
    } else if ( !GLOBAL_galerkin_state.multires_visibility ) {
        if ( !ShadowTestDiscretisation(&ray, ShadowList, fdist, &hitstore)) {
            *vis = 1.;
        } else {
            *vis = 0.;
        }
    } else if ( CacheHit(&ray, &fdist, &hitstore)) {
        *vis = 0.;
    } else {
        float min_feature_size = 2. * sqrt(GLOBAL_statistics_totalArea * GLOBAL_galerkin_state.rel_min_elem_area / M_PI);
        *vis = GeomListMultiResolutionVisibility(ShadowList, &ray, fdist, src->bsize, min_feature_size);
    }

    return ff;
}

/* Higher order area to area form factor computation. See
 *
 * - Ph. Bekaert, Y. D. Willems, "Error Control for Radiosity", Eurographics Rendering
 *	Workshop, Porto, Portugal, June 1996, p 158.
 */
static void DoHigherOrderAreaToAreaFormFactor(INTERACTION *link,
                                              CUBARULE *crrcv, Vector3D *x,
                                              CUBARULE *crsrc, Vector3D *y,
                                              double Gxy[CUBAMAXNODES][CUBAMAXNODES]) {
    static COLOR deltarad[CUBAMAXNODES];      /* see Bekaert & Willems, p159 bottom    */
    static double rcvphi[MAXBASISSIZE][CUBAMAXNODES];
    static double srcphi[CUBAMAXNODES];
    static double G_beta[CUBAMAXNODES];       /* G_beta[k] = G_{j,\beta}(x_k)          */
    static double delta_beta[CUBAMAXNODES];   /* delta_beta[k] = \delta_{j,\beta}(x_k) */

    ELEMENT *rcv = link->rcv, *src = link->src;
    GalerkinBasis *rcvbasis, *srcbasis;
    double G_alpha_beta, Gmin, Gmax, Gav;
    int k, l, alpha, beta;
    COLOR *srcrad = (GLOBAL_galerkin_state.iteration_method == SOUTHWELL) ?
                    src->unshot_radiance : src->radiance;

    /* receiver and source basis description */
    if ( IsCluster(rcv)) {
        /* no basis description for clusters: we always use a constant approximation on
         * clusters. */
        rcvbasis = (GalerkinBasis *) nullptr;
    } else {
        rcvbasis = (rcv->pog.patch->nrvertices == 3 ? &triBasis : &quadBasis);
    }

    if ( IsCluster(src)) {
        srcbasis = (GalerkinBasis *) nullptr;
    } else {
        srcbasis = (src->pog.patch->nrvertices == 3 ? &triBasis : &quadBasis);
    }

    /* determine basis function values \phi_{i,\alpha}(x_k) at sample positions on the
     * receiver patch for all basis functions \alpha */
    for ( k = 0; k < crrcv->nrnodes; k++ ) {
        if ( IsCluster(rcv)) {
            /* constant approximation on clusters */
            if ( link->nrcv != 1 ) {
                Fatal(-1, "DoHigherOrderAreaToAreaFormFactor",
                      "non-constant approximation on receiver cluster is not possible");
            }
            rcvphi[0][k] = 1.;
        } else {
            for ( alpha = 0; alpha < link->nrcv; alpha++ ) {
                rcvphi[alpha][k] = rcvbasis->function[alpha](crrcv->u[k], crrcv->v[k]);
            }
        }
        colorClear(deltarad[k]);
    }

    Gmin = HUGE;
    Gmax = -HUGE;
    for ( beta = 0; beta < link->nsrc; beta++ ) {
        /* determine basis function values \phi_{j,\beta}(x_l) at sample positions on the
         * source patch */
        if ( IsCluster(src)) {
            if ( beta > 0 ) {
                Fatal(-1, "DoHigherOrderAreaToAreaFormFactor",
                      "non-constant approximation on source cluster is not possible");
            }
            for ( l = 0; l < crsrc->nrnodes; l++ ) {
                srcphi[l] = 1.;
            }
        } else {
            for ( l = 0; l < crsrc->nrnodes; l++ ) {
                srcphi[l] = srcbasis->function[beta](crsrc->u[l], crsrc->v[l]);
            }
        }

        for ( k = 0; k < crrcv->nrnodes; k++ ) {
            /* compute point-to-patch form factors for positions x_k on receiver and
             * basis function \beta on the source */
            G_beta[k] = 0.;
            for ( l = 0; l < crsrc->nrnodes; l++ ) {
                G_beta[k] += crsrc->w[l] * Gxy[k][l] * srcphi[l];
            }
            G_beta[k] *= src->area;

            /* first part of error estimate at receiver node x_k */
            delta_beta[k] = -G_beta[k];
        }

        for ( alpha = 0; alpha < link->nrcv; alpha++ ) {
            /* compute patch-to-patch form factor for basis function alpha on the
             * receiver and beta on the source. */
            G_alpha_beta = 0.;
            for ( k = 0; k < crrcv->nrnodes; k++ ) {
                G_alpha_beta += crrcv->w[k] * rcvphi[alpha][k] * G_beta[k];
            }
            link->K.p[alpha * link->nsrc + beta] = rcv->area * G_alpha_beta;

            /* second part of error estimate at receiver node x_k */
            for ( k = 0; k < crrcv->nrnodes; k++ ) {
                delta_beta[k] += G_alpha_beta * rcvphi[alpha][k];
            }
        }

        for ( k = 0; k < crrcv->nrnodes; k++ ) COLORADDSCALED(deltarad[k], delta_beta[k], srcrad[beta], deltarad[k]);

        if ( beta == 0 ) {
            /* determine minimum and maximum point-to-patch form factor */
            for ( k = 0; k < crrcv->nrnodes; k++ ) {
                if ( G_beta[k] < Gmin ) {
                    Gmin = G_beta[k];
                }
                if ( G_beta[k] > Gmax ) {
                    Gmax = G_beta[k];
                }
            }
        }
    }

    if ( colorNull(srcrad[0])) {
        /* no source radiance: use constant radiance error approximation */
        Gav = link->K.p[0] / rcv->area;
        link->deltaK.f = Gmax - Gav;
        if ( Gav - Gmin > link->deltaK.f ) {
            link->deltaK.f = Gav - Gmin;
        }
    } else {
        link->deltaK.f = 0.;
        for ( k = 0; k < crrcv->nrnodes; k++ ) {
            double delta;

            COLORDIV(deltarad[k], srcrad[0], deltarad[k]);
            if ((delta = fabs(COLORMAXCOMPONENT(deltarad[k]))) > link->deltaK.f ) {
                link->deltaK.f = delta;
            }
        }
    }
    link->crcv = 1;    /* one error estimation coefficient */
}

/* Like above, but with a constant approximation on both the receiver and source 
 * element, which makes things slightly simpler. */
static void DoConstantAreaToAreaFormFactor(INTERACTION *link,
                                           CUBARULE *crrcv, Vector3D *x,
                                           CUBARULE *crsrc, Vector3D *y,
                                           double Gxy[CUBAMAXNODES][CUBAMAXNODES]) {
    ELEMENT *rcv = link->rcv, *src = link->src;
    double G, Gx, Gmin, Gmax;
    int k, l;

    G = 0.;
    Gmin = HUGE;
    Gmax = -HUGE;
    for ( k = 0; k < crrcv->nrnodes; k++ ) {
        Gx = 0.;
        for ( l = 0; l < crsrc->nrnodes; l++ ) {
            Gx += crsrc->w[l] * Gxy[k][l];
        }
        Gx *= src->area;

        G += crrcv->w[k] * Gx;

        if ( Gx > Gmax ) {
            Gmax = Gx;
        }
        if ( Gx < Gmin ) {
            Gmin = Gx;
        }
    }
    link->K.f = rcv->area * G;

    link->deltaK.f = G - Gmin;
    if ( Gmax - G > link->deltaK.f ) {
        link->deltaK.f = Gmax - G;
    }

    link->crcv = 1;
}

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

unsigned AreaToAreaFormFactor(INTERACTION *link, GeometryListNode *shadowlist) {
    /* Very often, the source or receievr element is the same as the one in
     * the previous call of the function. We cache cubature rules and nodes
     * in order to prevent recomputation. */
    static CUBARULE *crrcv = (CUBARULE *) nullptr,  /* cubature rules to be used over the */
    *crsrc = (CUBARULE *) nullptr;  /* receiving patch and source patch.  */
    static Vector3D x[CUBAMAXNODES], y[CUBAMAXNODES];
    static double Gxy[CUBAMAXNODES][CUBAMAXNODES], kval, vis, maxkval, maxptff;
    static unsigned viscount;        /* nr. of rays that "pass" occluders */
    ELEMENT *rcv = link->rcv, *src = link->src;
    int k, l;

    if ( IsCluster(rcv) || IsCluster(src)) {
        BOUNDINGBOX rcvbounds, srcbounds;
        ElementBounds(rcv, rcvbounds);
        ElementBounds(src, srcbounds);

        /* do not allow interactions between a pair of overlapping source and receiver */
        if ( !DisjunctBounds(rcvbounds, srcbounds) ) {
            /* take 0. as form factor */
            if ( link->nrcv == 1 && link->nsrc == 1 ) {
                link->K.f = 0.;
            } else {
                int i;
                for ( i = 0; i < link->nrcv * link->nsrc; i++ ) {
                    link->K.p[i] = 0.;
                }
            }

            /* and a large error on the form factor */
            link->deltaK.f = 1.;
            link->crcv = 1;

            /* and half visibility */
            return link->vis = 128;
        }
    } else {
        /* We assume that no light transport can take place between a surface element
         * and itself. It would cause trouble if we would go on b.t.w. */
        if ( rcv == src ) {
            /* take 0. as form factor */
            if ( link->nrcv == 1 && link->nsrc == 1 ) {
                link->K.f = 0.;
            } else {
                int i;
                for ( i = 0; i < link->nrcv * link->nsrc; i++ ) {
                    link->K.p[i] = 0.;
                }
            }

            /* and a 0 error on the form factor */
            link->deltaK.f = 0.;
            link->crcv = 1;

            /* and full occlusion */
            return link->vis = 0;
        }
    }

    /* If the receiver is an other one than before, determine the cubature
     * rule to be used on it and the nodes (positions on the patch). */
    if ( rcv != GLOBAL_galerkin_state.fflastrcv ) {
        DetermineNodes(rcv, &crrcv, x, RECEIVER);
    }

    /* Same for the source element */
    if ( src != GLOBAL_galerkin_state.fflastsrc ) {
        DetermineNodes(src, &crsrc, y, SOURCE);
    }

    /* Evaluate the radiosity kernel between each pair of nodes on the source
     * and the receiver element if at least receiver or source changed since
     * last time. */
    if ( rcv != GLOBAL_galerkin_state.fflastrcv || src != GLOBAL_galerkin_state.fflastsrc ) {
        /* Use shadow caching for accelerating occlusion detection */
        InitShadowCache();

        /* Mark the patches in order to avoid immediate selfintersections. */
        PatchDontIntersect(4, IsCluster(rcv) ? (PATCH *) nullptr : rcv->pog.patch,
                           IsCluster(rcv) ? (PATCH *) nullptr : rcv->pog.patch->twin,
                           IsCluster(src) ? (PATCH *) nullptr : src->pog.patch,
                           IsCluster(src) ? (PATCH *) nullptr : src->pog.patch->twin);
        geomDontIntersect(IsCluster(rcv) ? rcv->pog.geom : (Geometry *) nullptr,
                          IsCluster(src) ? src->pog.geom : (Geometry *) nullptr);

        maxkval = 0.;    /* compute maximum unoccluded kernel value */
        maxptff = 0.;    /* maximum unoccluded point-on-receiver to source form factor */
        viscount = 0;    /* count nr of rays that "pass" occluders */
        for ( k = 0; k < crrcv->nrnodes; k++ ) {
            double f;

            f = 0.;
            for ( l = 0; l < crsrc->nrnodes; l++ ) {
                kval = PointKernelEval(&x[k], &y[l], rcv, src, shadowlist, &vis);
                Gxy[k][l] = kval * vis;
                f += crsrc->w[l] * kval;

                if ( kval > maxkval ) {
                    maxkval = kval;
                }
                if ( Gxy[k][l] != 0. ) {
                    viscount++;
                }
            }
            if ( f > maxptff ) {
                maxptff = f;
            }
        }
        maxptff *= src->area;

        /* Unmark the patches, so they are considered for ray-patch intersections again
         * in future. */
        PatchDontIntersect(0);
        geomDontIntersect((Geometry *) nullptr, (Geometry *) nullptr);
    }

    if ( viscount != 0 ) {
        /* Actually compute the form factors. */
        if ( link->nrcv == 1 && link->nsrc == 1 ) {
            DoConstantAreaToAreaFormFactor(link, crrcv, x, crsrc, y, Gxy);
        } else {
            DoHigherOrderAreaToAreaFormFactor(link, crrcv, x, crsrc, y, Gxy);
        }
    }

    /* remember receiver and source for next time. */
    GLOBAL_galerkin_state.fflastrcv = rcv;
    GLOBAL_galerkin_state.fflastsrc = src;

    if ( GLOBAL_galerkin_state.clustering_strategy == ISOTROPIC &&
         (IsCluster(rcv) || IsCluster(src))) {
        link->deltaK.f = maxkval * src->area;
    }

    /* Returns the visibility: basically the fraction of rays that did not
     * hit an occluder. */
    link->vis = (unsigned) (255. * (double) viscount /
                            (double) (crrcv->nrnodes * crsrc->nrnodes));

    if ( GLOBAL_galerkin_state.exact_visibility && shadowlist != (GeometryListNode *) nullptr && link->vis == 255 ) {
        link->vis = 254;
    }    /* not full visibility, we missed the shadow! */

    return link->vis;
}
