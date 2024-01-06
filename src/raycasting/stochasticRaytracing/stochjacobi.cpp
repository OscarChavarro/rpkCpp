/* stochjacobi.c: generic stochastic Jacobi iteration (local lines) */
/* TODO: combined radiance/importance propagation */
/* TODO: hierarchical refinement for importance propagation */
/* TODO: re-incorporate the rejection sampling technique for
 * sampling positions on shooters with higher order radiosity approximation
 * (lower variance) */
/* TODO: global lines and global line bundles. */

#include <cstdlib>

#include "material/statistics.h"
#include "common/error.h"
#include "QMC/niederreiter.h"
#include "scene/scene.h"
#include "scene/spherical.h"
#include "raycasting/stochasticRaytracing/mcradP.h"
#include "raycasting/stochasticRaytracing/hierarchy.h"
#include "raycasting/stochasticRaytracing/stochjacobi.h"
#include "raycasting/stochasticRaytracing/ccr.h"
#include "raycasting/stochasticRaytracing/localline.h"

/* returns radiance or importance to be propagated */
static COLOR *(*get_radiance)(ELEMENT *);

static float (*get_importance)(ELEMENT *);

/* Converts received radiance or importance into a new approximation for
 * total and unshot radiance or importance */
static void (*reflect)(ELEMENT *, double) = (void (*)(ELEMENT *, double)) nullptr;

static int do_control_variate;    /* whether or not to use a constant control variate */
static int nr_rays;        /* nr of rays to shoot in the iteration */
static double sum_probs = 0.0;    /* sum of unnormalised sampling "probabilities" */

static void InitGlobals(int nrrays,
                        COLOR *(*GetRadiance)(ELEMENT *),
                        float (*GetImportance)(ELEMENT *),
                        void (*Update)(ELEMENT *P, double w)) {
    nr_rays = nrrays;
    get_radiance = GetRadiance;
    get_importance = GetImportance;
    reflect = Update;
    /* only use control variates for proagating radiance, not for importance */
    do_control_variate = (mcr.constant_control_variate && (GetRadiance));

    if ( get_radiance ) {
        mcr.prev_traced_rays = mcr.traced_rays;
    }
    if ( get_importance ) {
        mcr.prev_imp_traced_rays = mcr.imp_traced_rays;
    }
}

static void PrintMessage(long nr_rays) {
    fprintf(stderr, "%s-directional ",
            mcr.bidirectional_transfers ? "Bi" : "Uni");
    if ( get_radiance && get_importance ) {
        fprintf(stderr, "combined ");
    }
    if ( get_radiance ) {
        fprintf(stderr, "%sradiance ",
                mcr.importance_driven ? "importance-driven " : "");
    }
    if ( get_radiance && get_importance ) {
        fprintf(stderr, "and ");
    }
    if ( get_importance ) {
        fprintf(stderr, "%simportance ",
                mcr.radiance_driven ? "radiance-driven " : "");
    }
    fprintf(stderr, "propagation");
    if ( do_control_variate ) {
        fprintf(stderr, "using a constant control variate ");
    }
    fprintf(stderr, "(%ld rays):\n", nr_rays);
}

/* compute (unnormalised) probability of shooting a ray from elem */
static double Probability(ELEMENT *elem) {
    double prob = 0.;

    if ( get_radiance ) {
        /* probability proportional to power to be propagated. */
        COLOR radiance = get_radiance(elem)[0];
        if ( mcr.constant_control_variate ) {
            colorSubtract(radiance, mcr.control_radiance, radiance);
        }
        prob = /* M_PI * */ elem->area * colorSumAbsComponents(radiance);
        if ( mcr.importance_driven ) {
            /* weight with received importance */
            float w = (elem->imp - elem->source_imp);
            prob *= ((w > 0.) ? w : 0.);
        }
    }

    if ( get_importance && mcr.importance_driven ) {
        double prob2 = elem->area * fabs(get_importance(elem)) * ElementScalarReflectance(elem);

        if ( mcr.radiance_driven ) {
            /* received-radiance weighted importance transport */
            COLOR received_radiance;
            colorSubtract(elem->rad[0], elem->source_rad, received_radiance);
            prob2 *= colorSumAbsComponents(received_radiance);
        }

        /* equal weight to importance and radiance propagation for constant approximation,
         * but higher weight to radiance for higher order approximations. Still OK
         * if only propagating importance. */
        prob = prob * approxdesc[mcr.approx_type].basis_size + prob2;
    }

    return prob;
}

/* clear accumulators of all kinds of sample weights and contributions */
static void ElementClearAccumulators(ELEMENT *elem) {
    if ( get_radiance ) {
        stochasticRadiosityClearCoefficients(elem->received_rad, elem->basis);
    }
    if ( get_importance ) {
        elem->received_imp = 0.;
    }
}

/* Clears received radiance and importance and accumulates the unnormalised
 * sampling probabilities at leaf elements. */
static void ElementSetup(ELEMENT *elem) {
    elem->prob = 0.;
    if ( !ForAllChildrenElements(elem, ElementSetup)) {
        /* elem is a leaf element. We need to compute the sum of the unnormalized
         * sampling "probabilities" at the leaf elements */
        elem->prob = Probability(elem);
        sum_probs += elem->prob;
    }
    if ( elem->parent ) {
        /* the probability of sampling a non-leaf element is the sum of the
         * probabilities of sampling the subelements */
        elem->parent->prob += elem->prob;
    }

    ElementClearAccumulators(elem);
}

/* returns true if succes, that is: sum of sampling probabilities is nonzero */
static int Setup() {
    /* determine constant control radiosity if required. */
    colorClear(mcr.control_radiance);
    if ( do_control_variate ) {
        mcr.control_radiance = DetermineControlRadiosity(get_radiance, nullptr);
    }

    sum_probs = 0.;
    ElementSetup(hierarchy.topcluster);

    if ( sum_probs < EPSILON * EPSILON ) {
        Warning("Iteration", "No sources");
        return false;
    }
    return true;
}

/* returns radiance to be propagated from the given location of the element */
static COLOR GetSourceRadiance(ELEMENT *src, double us, double vs) {
    COLOR *srcrad = get_radiance(src);
    return ColorAtUV(src->basis, srcrad, us, vs);
}

static void PropagateRadianceToSurface(ELEMENT *rcv, double ur, double vr,
                                       COLOR raypow, ELEMENT *src,
                                       double fraction, double weight) {
    int i;
    for ( i = 0; i < rcv->basis->size; i++ ) {
        double dual = rcv->basis->dualfunction[i](ur, vr) / rcv->area;
        double w = dual * fraction / (double) nr_rays;
        colorAddScaled(rcv->received_rad[i], w, raypow, rcv->received_rad[i]);
    }
}

static void PropagateRadianceToClusterIsotropic(ELEMENT *cluster, COLOR raypow, ELEMENT *src,
                                                double fraction, double weight) {
    double w = fraction / cluster->area / (double) nr_rays;
    colorAddScaled(cluster->received_rad[0], w, raypow, cluster->received_rad[0]);
}

static void PropagateRadianceToClusterOriented(ELEMENT *cluster, COLOR raypow, Ray *ray, float dir,
                                               ELEMENT *src, double projarea,
                                               double fraction, double weight) {
    REC_ForAllClusterSurfaces(rcv, cluster)
            {
                double c = -dir * VECTORDOTPRODUCT(rcv->pog.patch->normal, ray->dir);
                if ( c > 0. ) {
                    double afrac = fraction * (c * rcv->area / projarea);
                    double w = afrac / rcv->area / (double) nr_rays;
                    colorAddScaled(rcv->received_rad[0], w, raypow, rcv->received_rad[0]);
                }
            }
    REC_EndForAllClusterSurfaces;
}

static double ReceiverProjectedArea(ELEMENT *cluster, Ray *ray, float dir) {
    double area = 0.;
    REC_ForAllClusterSurfaces(rcv, cluster)
            {
                double c = -dir * VECTORDOTPRODUCT(rcv->pog.patch->normal, ray->dir);
                if ( c > 0. ) {
                    area += c * rcv->area;
                }
            }
    REC_EndForAllClusterSurfaces;
    return area;
}

/* transfer radiance from src to rcv.
 * src_prob = unnormalised src birth probability / src area
 * rcv_prob = unnormalised rcv birth probability / rcv area for bidirectional transfers
 *       or = 0 for unidirectional transfers
 * score will be weighted with sum_probs / nr_rays (both are global).
 * ray->dir and dir are used in order to determine projected cluster area
 * and cosine of incident direction of cluster surface elements when
 * the receiver is a cluster. */
static void PropagateRadiance(ELEMENT *src, double us, double vs,
                              ELEMENT *rcv, double ur, double vr,
                              double src_prob, double rcv_prob,
                              Ray *ray, float dir) {
    COLOR rad, raypow;
    double area,
            weight = sum_probs / src_prob,        /* src area / normalised src prob */
    fraction = src_prob / (src_prob + rcv_prob);    /* 1 for uni-directional transfers */

    if ( src_prob < EPSILON * EPSILON /* this should never happen */
         || fraction < EPSILON ) { /* reverse transfer from a black surface */
        return;
    }

    rad = GetSourceRadiance(src, us, vs);
    if ( mcr.constant_control_variate ) {
        colorSubtract(rad, mcr.control_radiance, rad);
    }
    colorScale(weight, rad, raypow);

    if ( !rcv->iscluster ) {
        PropagateRadianceToSurface(rcv, ur, vr, raypow, src, fraction, weight);
    } else {
        switch ( hierarchy.clustering ) {
            case NO_CLUSTERING:
                Fatal(-1, "Propagate", "Refine() returns cluster although clustering is disabled.\n");
                break;
            case ISOTROPIC_CLUSTERING:
                PropagateRadianceToClusterIsotropic(rcv, raypow, src, fraction, weight);
                break;
            case ORIENTED_CLUSTERING:
                area = ReceiverProjectedArea(rcv, ray, dir);
                if ( area > EPSILON ) {
                    PropagateRadianceToClusterOriented(rcv, raypow, ray, dir, src, area, fraction, weight);
                }
                break;
            default:
                Fatal(-1, "Propagate", "Invalid clustering mode %d\n", (int) hierarchy.clustering);
        }
    }
}

/* idem but for importance */
static void PropagateImportance(ELEMENT *src, double us, double vs,
                                ELEMENT *rcv, double ur, double vr,
                                double src_prob, double rcv_prob,
                                Ray *ray, float dir) {
    double w = sum_probs / (src_prob + rcv_prob) / rcv->area / (double) nr_rays;
    rcv->received_imp += w * ElementScalarReflectance(src) * get_importance(src);

    if ( hierarchy.do_h_meshing || hierarchy.clustering != NO_CLUSTERING ) {
        Fatal(-1, "Propagate", "Importance propagation not implemented in combination with hierarchical refinement");
    }
}

/* src is the leaf element containing the point from which to propagate
 * radiance on P. P and Q are toplevel surface elements. Transfer 
 * is from P to Q. */
static void RefineAndPropagateRadiance(ELEMENT *src, double us, double vs,
                                       ELEMENT *P, double up, double vp,
                                       ELEMENT *Q, double uq, double vq,
                                       double src_prob, double rcv_prob,
                                       Ray *ray, float dir) {
    LINK link;
    link = TopLink(Q, P);
    Refine(&link, Q, &uq, &vq, P, &up, &vp, hierarchy.oracle);
    /* propagate from the leaf element src to the admissable receiver element
     * containing/contained by Q */
    PropagateRadiance(src, us, vs, link.rcv, uq, vq, src_prob, rcv_prob, ray, dir);
}

static void RefineAndPropagateImportance(ELEMENT *src, double us, double vs,
                                         ELEMENT *P, double up, double vp,
                                         ELEMENT *Q, double uq, double vq,
                                         double src_prob, double rcv_prob,
                                         Ray *ray, float dir) {
    /* no refinement (yet) for importance: propagate between toplevel surfaces */
    PropagateImportance(P, up, vp, Q, uq, vq, src_prob, rcv_prob, ray, dir);
}

/* ray is a ray connecting the positions with given (u,v) parameters
 * on the toplevel surface element P to Q. This routine refines the
 * imaginary interaction between these elements and performs
 * radiance or importance transfer along the ray, taking into account
 * bidirectionality if requested. */
static void RefineAndPropagate(ELEMENT *P, double up, double vp,
                               ELEMENT *Q, double uq, double vq,
                               Ray *ray) {
    double src_prob;
    double us = up, vs = vp;
    ELEMENT *src = RegularLeafElementAtPoint(P, &us, &vs);
    src_prob = (double) src->prob / (double) src->area;
    if ( mcr.bidirectional_transfers ) {
        double rcv_prob;
        double ur = uq, vr = vq;
        ELEMENT *rcv = RegularLeafElementAtPoint(Q, &ur, &vr);
        rcv_prob = (double) rcv->prob / (double) rcv->area;

        if ( get_radiance ) {
            RefineAndPropagateRadiance(src, us, vs, P, up, vp, Q, uq, vq, src_prob, rcv_prob, ray, +1);
            RefineAndPropagateRadiance(rcv, ur, vr, Q, uq, vq, P, up, vp, rcv_prob, src_prob, ray, -1);
        }
        if ( get_importance ) {
            RefineAndPropagateImportance(src, us, vs, P, up, vp, Q, uq, vq, src_prob, rcv_prob, ray, +1);
            RefineAndPropagateImportance(rcv, ur, vr, Q, uq, vq, P, up, vp, rcv_prob, src_prob, ray, -1);
        }
    } else {
        if ( get_radiance ) {
            RefineAndPropagateRadiance(src, us, vs, P, up, vp, Q, uq, vq, src_prob, 0., ray, +1);
        }
        if ( get_importance ) {
            RefineAndPropagateImportance(src, us, vs, P, up, vp, Q, uq, vq, src_prob, 0., ray, +1);
        }
    }
}

static double *NextSample(ELEMENT *elem,
                          int nmsb, niedindex msb1, niedindex rmsb2,
                          double *zeta) {
    niedindex *xi = nullptr, u, v;
    /* use different ray index for propagating importance and radiance */
    niedindex *ray_index = get_radiance ? &elem->ray_index : &elem->imp_ray_index;

    xi = NextNiedInRange(ray_index, +1, nmsb, msb1, rmsb2);

    (*ray_index)++;
    u = (xi[0] & ~3) | 1;        /* avoid positions on subelement boundaries */
    v = (xi[1] & ~3) | 1;
    if ( elem->nrvertices == 3 )
        FoldSample(&u, &v);
    zeta[0] = (double) u * RECIP;
    zeta[1] = (double) v * RECIP;
    zeta[2] = (double) xi[2] * RECIP;
    zeta[3] = (double) xi[3] * RECIP;
    return zeta;
}

/* determines uniform (u,v) parameters of hit point on hit patch */
static void UniformHitCoordinates(HITREC *hit, double *uhit, double *vhit) {
    if ( hit->flags & HIT_UV ) {    /* (u,v) coordinates obtained as side result
				 * of intersection test */
        *uhit = hit->uv.u;
        *vhit = hit->uv.v;
        if ( hit->patch->jacobian ) {
            BilinearToUniform(hit->patch, uhit, vhit);
        }
    } else {
        PatchUniformUV(hit->patch, &hit->point, uhit, vhit);
    }

    /* clip uv coordinates to lay strictly inside the hit patch */
    if ( *uhit < EPSILON ) {
        *uhit = EPSILON;
    }
    if ( *vhit < EPSILON ) {
        *vhit = EPSILON;
    }
    if ( *uhit > 1. - EPSILON ) {
        *uhit = 1. - EPSILON;
    }
    if ( *vhit > 1. - EPSILON ) {
        *vhit = 1. - EPSILON;
    }
}

/* traces a local line from 'src' and propagates radiance and/or importance from P to
 * hit patch (and back for bidirectional transfers). */
static void ElementShootRay(ELEMENT *src,
                            int nmsb, niedindex msb1, niedindex rmsb2) {
    Ray ray;
    HITREC *hit, hitstore;
    double zeta[4];

    if ( get_radiance ) {
        mcr.traced_rays++;
    }
    if ( get_importance ) {
        mcr.imp_traced_rays++;
    }

    ray = McrGenerateLocalLine(src->pog.patch,
                               NextSample(src, nmsb, msb1, rmsb2, zeta));
    hit = McrShootRay(src->pog.patch, &ray, &hitstore);
    if ( hit ) {
        double uhit = 0., vhit = 0.;
        UniformHitCoordinates(hit, &uhit, &vhit);
        RefineAndPropagate(TOPLEVEL_ELEMENT(src->pog.patch), zeta[0], zeta[1],
                           TOPLEVEL_ELEMENT(hit->patch), uhit, vhit, &ray);
    } else {
        mcr.nrmisses++;
    }
}

static void InitPushRayIndex(ELEMENT *elem) {
    elem->ray_index = elem->parent->ray_index;
    elem->imp_ray_index = elem->parent->imp_ray_index;
    ForAllChildrenElements(elem, InitPushRayIndex);
}

/* determines nr of rays to shoot from elem and shoots this number of rays. */
static void ElementShootRays(ELEMENT *elem, int rays_this_elem) {
    int i;
    int nmsb;        /* determines a range in which to generate a sample */
    niedindex msb1, rmsb2;    /* see ElementRange() and NextSample() */

    /* sample number range for 4D Niederreiter sequence */
    ElementRange(elem, &nmsb, &msb1, &rmsb2);

    /* shoot the rays */
    for ( i = 0; i < rays_this_elem; i++ ) {
        ElementShootRay(elem, nmsb, msb1, rmsb2);
    }

    if ( !ElementIsLeaf(elem)) {
        /* source got subdivided while shooting the rays */
        ForAllChildrenElements(elem, InitPushRayIndex);
    }
}

/* fire off rays from the leaf elements, propagate radiance/importance. */
static void ShootRays() {
    double rnd = drand48();
    long ray_count = 0;
    double p_cumul = 0.0;

    /* loop over all leaf elements in the element hierarchy */
    ForAllPatches(P, GLOBAL_scene_patches)
                {
                    REC_ForAllSurfaceLeafs(leaf, TOPLEVEL_ELEMENT(P))
                            {
                                double p = leaf->prob / sum_probs;
                                long rays_this_leaf =
                                        (long) floor((p_cumul + p) * (double) nr_rays + rnd) - ray_count;

                                if ( rays_this_leaf > 0 ) {
                                    ElementShootRays(leaf, rays_this_leaf);
                                }

                                p_cumul += p;
                                ray_count += rays_this_leaf;
                            }
                    REC_EndForAllSurfaceLeafs;
                }
    EndForAll;

    fprintf(stderr, "\n");
}

/* converts received radiance and importance at a leaf element into a new
 * approximation of total and unshot radiance and importance */
static void UpdateElement(ELEMENT *elem) {
    if ( get_radiance ) {
        if ( do_control_variate ) {
            /* add constant radiosity contribution to received flux */
            colorAdd(elem->received_rad[0], mcr.control_radiance, elem->received_rad[0]);
        }
        /* multiply with reflectivity on leaf elements only */
        MULTCOEFFICIENTS(elem->Rd, elem->received_rad, elem->basis);
    }

    reflect(elem, (double) nr_rays / sum_probs);

    colorAddScaled(mcr.unshot_flux, M_PI * elem->area, elem->unshot_rad[0], mcr.unshot_flux);
    colorAddScaled(mcr.total_flux, M_PI * elem->area, elem->rad[0], mcr.total_flux);
    colorAddScaled(mcr.imp_unshot_flux, M_PI * elem->area * (elem->imp - elem->source_imp), elem->unshot_rad[0],
                   mcr.imp_unshot_flux);
    mcr.unshot_ymp += elem->area * fabs(elem->unshot_imp);
    mcr.total_ymp += elem->area * elem->imp;
}

static void Push(ELEMENT *parent, ELEMENT *child) {
    if ( get_radiance ) {
        COLOR Rd;
        colorClear(Rd);

        if ( parent->iscluster && !child->iscluster ) {
            /* multiply with reflectance (See PropagateRadianceToClusterIsotropic() above) */
            COLOR rad = parent->received_rad[0];
            Rd = child->Rd;
            colorProduct(Rd, rad, rad);
            PushRadiance(parent, child, &rad, child->received_rad);
        } else
            PushRadiance(parent, child, parent->received_rad, child->received_rad);
    }

    if ( get_importance )
        PushImportance(parent, child, &parent->received_imp, &child->received_imp);
}

static void Pull(ELEMENT *parent, ELEMENT *child) {
    if ( get_radiance ) {
        PullRadiance(parent, child, parent->rad, child->rad);
        PullRadiance(parent, child, parent->unshot_rad, child->unshot_rad);
    }
    if ( get_importance ) {
        PullImportance(parent, child, &parent->imp, &child->imp);
        PullImportance(parent, child, &parent->unshot_imp, &child->unshot_imp);
    }
}

/* clears everything to be pulled from children elements to zero */
static void ClearElement(ELEMENT *parent) {
    if ( get_radiance ) {
        stochasticRadiosityClearCoefficients(parent->rad, parent->basis);
        stochasticRadiosityClearCoefficients(parent->unshot_rad, parent->basis);
    }
    if ( get_importance ) {
        parent->imp = parent->unshot_imp = 0.;
    }
}

static void PushUpdatePull(ELEMENT *elem);

static void PushUpdatePullChild(ELEMENT *child) {
    ELEMENT *parent = child->parent;
    Push(parent, child);
    PushUpdatePull(child);
    Pull(parent, child);
}

static void PushUpdatePull(ELEMENT *elem) {
    if ( ElementIsLeaf(elem)) {
        UpdateElement(elem);
    } else {    /* not a leaf element */
        ClearElement(elem);
        ForAllChildrenElements(elem, PushUpdatePullChild);
    }
}

static void PullRdEd(ELEMENT *elem);

static void PullRdEdFromChild(ELEMENT *child) {
    ELEMENT *parent = child->parent;

    PullRdEd(child);

    colorAddScaled(parent->Ed, child->area / parent->area, child->Ed, parent->Ed);
    colorAddScaled(parent->Rd, child->area / parent->area, child->Rd, parent->Rd);
    if ( parent->iscluster )
        colorSetMonochrome(parent->Rd, 1.);
}

static void PullRdEd(ELEMENT *elem) {
    if ( ElementIsLeaf(elem) || (!elem->iscluster && !ElementIsTextured(elem))) {
        return;
    }

    colorClear(elem->Ed);
    colorClear(elem->Rd);
    ForAllChildrenElements(elem, PullRdEdFromChild);
}

static void PushUpdatePullSweep() {
    /* update radiance, compute new total and unshot flux. */
    colorClear(mcr.unshot_flux);
    mcr.unshot_ymp = 0.;
    colorClear(mcr.total_flux);
    mcr.total_ymp = 0.;
    colorClear(mcr.imp_unshot_flux);

    /* update reflectances and emittances (refinement yields more accurate estimates
     * on textured surfaces) */
    PullRdEd(hierarchy.topcluster);

    PushUpdatePull(hierarchy.topcluster);
}

/*
 * Generic routine for Stochastic Jacobi iterations:
 * - nr_rays: nr of rays to use
 * - GetRadiance: routine returning radiance (total or unshot) to be 
 * propagated for a given element, or nullptr if no radiance propagation is
 * required.
 * - GetImportance: same, but for importance.
 * - Update: routine updating total, unshot and source radiance and/or
 * importance based on result received during the iteration. 
 *
 * The operation of this routine is further controlled by global parameters
 * - mcr.do_control_radiosity: perform constant control variate variance reduction
 * - mcr.bidirectional_transfers: for using lines bidirectionally
 * - mcr.importance_driven: importance-driven radiance propagation
 * - mcr.radiance_driven: radiance-driven importance propagation
 * - hierarchy.do_h_meshing, hierarchy.clustering: hierarchical refinement/clustering
 *
 * This routine updates global ray counts and total/unshot power/importance statistics.
 *
 * CAVEAT: propagate either radiance or importance alone. Simultaneous
 * propagation of importance and radiance does not work yet.
 */
void
DoStochasticJacobiIteration(
    long nr_rays,
    COLOR *(*GetRadiance)(ELEMENT *),
    float (*GetImportance)(ELEMENT *),
    void (*Update)(ELEMENT *P, double w))
{
    InitGlobals(nr_rays, GetRadiance, GetImportance, Update);
    PrintMessage(nr_rays);
    if ( !Setup()) {
        return;
    }
    ShootRays();
    PushUpdatePullSweep();
}
