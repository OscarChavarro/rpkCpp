#include "material/RendererConfiguration.h"

#ifdef RAYTRACING_ENABLED
#include "raycasting/stochasticRaytracing/StochasticRadiosityElement.h"
#include "java/util/ArrayList.txx"
#include "common/logging/Logger.h"
#include "numericalAnalysis/PatchVisitor.h"
#include "raycasting/stochasticRaytracing/McradP.h"
#include "raycasting/stochasticRaytracing/Hierarchy.h"

/**
Orientation and position of regular sub-elements is fully determined by the
transformations stored in StochasticRadiosityBasisState. A uniform mapping of
parameter domain to the elements is supposed (i.o.w. use uniformPoint() to map
(u,v) coordinates on the toplevel element to a 3D point on the patch). The
sub-elements have equal area. No explicit Jacobian stuff needed to compute
integrals etc.. etc.

Do not change the conventions below without knowing VERY well what
you are doing.
*/
int StochasticRadiosityElement::coefficientPoolsInitialized = false;

bool
StochasticRadiosityElement::coefficientPoolsAreInitialized() {
    return coefficientPoolsInitialized;
}

void
StochasticRadiosityElement::markCoefficientPoolsInitialized() {
    coefficientPoolsInitialized = true;
}

void
StochasticRadiosityElement::vertexAttachElement(Vertex *v, StochasticRadiosityElement *elem) {
    elem->className = ELEMENT_STOCHASTIC_RADIOSITY;
    if ( v->radianceData == NULL ) {
        v->radianceData = new ArrayList<Element *>();
    }
    v->radianceData->add(elem);
}

/**
Basically sets rad to NULL
*/
void
Coefficientsmcrad::initCoefficients(StochasticRadiosityElement *elem) {
    if ( !StochasticRadiosityElement::coefficientPoolsAreInitialized() ) {
        StochasticRadiosityElement::markCoefficientPoolsInitialized();
    }

    elem->radiance = NULL;
    elem->unShotRadiance = NULL;
    elem->receivedRadiance = NULL;
    elem->basis = &StochasticRadiosityBasisState::activeState().dummyBasis;
}

StochasticRadiosityElement::StochasticRadiosityElement():
    patch(),
    geometry(),
    rayIndex(),
    quality(),
    samplingProbability(),
    ng(),
    basis(),
    importance(),
    unShotImportance(),
    receivedImportance(),
    sourceImportance(),
    importanceRayIndex(),
    vertices(),
    childNumber(),
    numberOfVertices()
{
    className = ELEMENT_STOCHASTIC_RADIOSITY;
}

StochasticRadiosityElement *
StochasticRadiosityElement::createElement() {
    static long id = 1;
    StochasticRadiosityElement *elem = new StochasticRadiosityElement();

    elem->patch = NULL;
    elem->geometry = NULL;
    elem->id = ((int)(id));
    id++;
    elem->area = 0.0;
    Coefficientsmcrad::initCoefficients(elem); // Allocation of the coefficients is left until just before the first iteration
    // in Mcrad::monteCarloRadiosityReInit()

    elem->Ed.clear();
    elem->Rd.clear();

    elem->rayIndex = 0;
    elem->quality = 0;
    elem->ng = 0.0;

    elem->importance = 0.0;
    elem->unShotImportance = 0.0;
    elem->sourceImportance = 0.0;
    elem->importanceRayIndex = 0;

    elem->midPoint.set(0.0, 0.0, 0.0);
    elem->vertices[0] = elem->vertices[1] = elem->vertices[2] = elem->vertices[3] = NULL;
    elem->parent = NULL;
    elem->regularSubElements = NULL;
    elem->irregularSubElements = NULL;
    elem->transformToParent = NULL;
    elem->childNumber = -1;
    elem->numberOfVertices = 0;
    elem->flags = 0x00;

    ElementHierarchyState::activeState().nr_elements++;

    return elem;
}

StochasticRadiosityElement *
StochasticRadiosityElement::stchsRadElemCreateFromPtch(Patch *patch) {
    StochasticRadiosityElement *elem = createElement();
    elem->patch = patch;
    elem->flags = 0x00;
    elem->area = patch->area;
    elem->midPoint = patch->midPoint;
    elem->numberOfVertices = patch->numberOfVertices;
    for ( int i = 0; i < elem->numberOfVertices; i++ ) {
        elem->vertices[i] = patch->vertex[i];
        vertexAttachElement(elem->vertices[i], elem);
    }

    Coefficientsmcrad::allocCoefficients(elem); // May need reallocation before the start of the computations
    Coefficientsmcrad::stchsRadClearCoeff(elem->radiance, elem->basis);
    Coefficientsmcrad::stchsRadClearCoeff(elem->unShotRadiance, elem->basis);
    Coefficientsmcrad::stchsRadClearCoeff(elem->receivedRadiance, elem->basis);

    elem->Ed = PatchVisitor::averageEmittance(patch, DIFFUSE_COMPONENT);
    elem->Ed.scaleInverse(((float)(M_PI)), elem->Ed);
    elem->Rd = PatchVisitor::averageNormalAlbedo(patch, BRDF_DIFFUSE_COMPONENT);

    return elem;
}

StochasticRadiosityElement::~StochasticRadiosityElement() {
}

StochasticRadiosityElement *
StochasticRadiosityElement::mntCarloRadCreateClust(Geometry *geometry) {
    StochasticRadiosityElement *elem = createElement();

    elem->geometry = geometry;
    elem->flags = IS_CLUSTER_MASK;

    elem->Rd = ColorRgbMutable(1.0f, 1.0f, 1.0f);
    elem->Ed = ColorRgbMutable(0.0f, 0.0f, 0.0f);

    // elem->area will be computed from the sub-elements in the cluster later
    elem->midPoint = geometry->boundingBox.center();

    Coefficientsmcrad::allocCoefficients(elem); // Always constant approx. so no need to delay allocating the coefficients
    Coefficientsmcrad::stchsRadClearCoeff(elem->radiance, elem->basis);
    Coefficientsmcrad::stchsRadClearCoeff(elem->unShotRadiance, elem->basis);
    Coefficientsmcrad::stchsRadClearCoeff(elem->receivedRadiance, elem->basis);
    elem->importance = 0.0;
    elem->unShotImportance = 0.0;
    elem->receivedImportance = 0.0;

    ElementHierarchyState::activeState().nr_clusters++;

    return elem;
}

void
StochasticRadiosityElement::mntCarloRadCreateSurfElemChld(Patch *patch, StochasticRadiosityElement *parent) {
    StochasticRadiosityElement *elem = ((StochasticRadiosityElement *)(patch->radianceData)); // Created before
    elem->parent = parent;

    elem->className = ELEMENT_STOCHASTIC_RADIOSITY;
    if ( parent->irregularSubElements == NULL ) {
        parent->irregularSubElements = new ArrayList<Element *>();
    }
    parent->irregularSubElements->add(elem);
}

void
StochasticRadiosityElement::mntCarloRadCreateClustChld(Geometry *geom, StochasticRadiosityElement *parent) {
    StochasticRadiosityElement *subCluster = mntCarloRadCreateClustHierRec(geom);
    subCluster->parent = parent;
    subCluster->className = ELEMENT_STOCHASTIC_RADIOSITY;
    if ( parent->irregularSubElements == NULL ) {
        parent->irregularSubElements = new ArrayList<Element *>();
    }
    parent->irregularSubElements->add(subCluster);
}

/**
Initialises parent cluster radiance/importance/area for child voxelData
*/
void
StochasticRadiosityElement::mntCarloRadInitClustPull(StochasticRadiosityElement *parent, const StochasticRadiosityElement *child) {
    parent->area += child->area;
    StochasticRadiosityElement::stchsRadElemPullRadn(parent, child, parent->radiance, child->radiance);
    StochasticRadiosityElement::stchsRadElemPullRadn(parent, child, parent->unShotRadiance, child->unShotRadiance);
    StochasticRadiosityElement::stchsRadElemPullRadn(parent, child, parent->receivedRadiance, child->receivedRadiance);
    StochasticRadiosityElement::stchsRadElemPullImp(parent, child, &parent->importance, &child->importance);
    StochasticRadiosityElement::stchsRadElemPullImp(parent, child, &parent->unShotImportance, &child->unShotImportance);
    StochasticRadiosityElement::stchsRadElemPullImp(parent, child, &parent->receivedImportance, &child->receivedImportance);

    // Needs division by parent->area once it is known after mntCarloRadInitClustPull for
    // all children elements
    parent->Ed.addScaled(parent->Ed, child->area, child->Ed);
}

void
StochasticRadiosityElement::mntCarloRadCreateClustChildren(StochasticRadiosityElement *parent) {
    Geometry *geometry = parent->geometry;

    if ( geometry->isCompound() ) {
        ArrayList<Geometry *> *geometryList = Geometry::primitiveListCopy(geometry);
        for ( int i = 0; geometryList != NULL && i < geometryList->size(); i++ ) {
            mntCarloRadCreateClustChld(geometryList->get(i), parent);
        }
        delete geometryList;
    } else {
        const ArrayList<Patch *> *patchList = Geometry::patchListReference(geometry);
        for ( int i = 0; patchList != NULL && i < patchList->size(); i++ ) {
            mntCarloRadCreateSurfElemChld(patchList->get(i), parent);
        }
    }

    for ( int i = 0; parent->irregularSubElements != NULL && i < parent->irregularSubElements->size(); i++ ) {
        mntCarloRadInitClustPull(parent, ((StochasticRadiosityElement *)(parent->irregularSubElements->get(i))));
    }
    parent->Ed.scaleInverse(parent->area, parent->Ed);
}

StochasticRadiosityElement *
StochasticRadiosityElement::mntCarloRadCreateClustHierRec(Geometry *world) {
    StochasticRadiosityElement *topCluster = mntCarloRadCreateClust(world);
    world->radianceData = topCluster;
    mntCarloRadCreateClustChildren(topCluster);
    return topCluster;
}

StochasticRadiosityElement *
StochasticRadiosityElement::stchsRadElemCreateFromGeom(Geometry *world) {
    if ( !world ) {
        return NULL;
    } else {
        return mntCarloRadCreateClustHierRec(world);
    }
}

/**
Determine the (u, v) coordinate range of the element w.r.t. the patch to
which it belongs when using regular quadtree subdivision in
order to efficiently generate samples with Niederreiter::NextNiedInRange()
in the Niederreiter core implementation. Niederreiter::NextNiedInRange() creates a sample on a quadrilateral
subdomain, called a "dyadic box" in QMC literature. All samples in
such a dyadic box have the same most significant bits. This routine
basically computes what these most significant bits are. The computation
is based on the regular quadtree subdivision of a quadrilateral, as
shown above. If a triangular element is to be sampled, the quadrilateral
sample needs to be "folded" into the triangle. FoldSample() in sample4d.c
does this
*/
void
StochasticRadiosityElement::stochasticRadiosityElementRange(
        StochasticRadiosityElement *elem,
        int *numberOfBits,
        NiederreiterIndex *mostSignificantBits1,
        NiederreiterIndex *rMostSignificantBits2)
{
    int nb;
    NiederreiterIndex b1;
    NiederreiterIndex b2;

    nb = 0;
    b1 = 0;
    b2 = 0;
    while ( elem->childNumber >= 0 ) {
        nb++;
        b1 = (b1 << 1) | ((unsigned long)(elem->childNumber & 1));
        b2 = (b2 >> 1) | (((unsigned long)(elem->childNumber & 2)) << (NBITS - 2));
        elem = ((StochasticRadiosityElement *)(elem->parent));
    }

    *numberOfBits = nb;
    *mostSignificantBits1 = b1;
    *rMostSignificantBits2 = b2;
}

/**
Determines the regular sub-element at point (u,v) of the given parent
element. Returns the parent element itself if there are no regular sub-elements.
The point is transformed to the corresponding point on the sub-element
*/
StochasticRadiosityElement *
StochasticRadiosityElement::stchsRadElemRegSubElemAPnt(
    const StochasticRadiosityElement *parent,
    double *u,
    double *v)
{
    StochasticRadiosityElement *child = NULL;
    const double _u = *u;
    const double _v = *v;

    if ( parent->isCluster() || !parent->regularSubElements ) {
        return NULL;
    }

    // Have a look at the drawings above to understand what is done exactly
    switch ( parent->numberOfVertices ) {
        case 3:
            if ( _u + _v <= 0.5 ) {
                child = ((StochasticRadiosityElement *)(parent->regularSubElements[0]));
                *u = _u * 2.0;
                *v = _v * 2.0;
            } else if ( _u > 0.5 ) {
                    child = ((StochasticRadiosityElement *)(parent->regularSubElements[1]));
                    *u = (_u - 0.5) * 2.0;
                    *v = _v * 2.0;
                } else if ( _v > 0.5 ) {
                        child = ((StochasticRadiosityElement *)(parent->regularSubElements[2]));
                        *u = _u * 2.0;
                        *v = (_v - 0.5) * 2.0;
                    } else {
                        child = ((StochasticRadiosityElement *)(parent->regularSubElements[3]));
                        *u = (0.5 - _u) * 2.0;
                        *v = (0.5 - _v) * 2.0;
                    }
            break;
        case 4:
            if ( _v <= 0.5 ) {
                if ( _u < 0.5 ) {
                    child = ((StochasticRadiosityElement *)(parent->regularSubElements[0]));
                    *u = _u * 2.0;
                } else {
                    child = ((StochasticRadiosityElement *)(parent->regularSubElements[1]));
                    *u = (_u - 0.5) * 2.0;
                }
                *v = _v * 2.0;
            } else {
                if ( _u < 0.5 ) {
                    child = ((StochasticRadiosityElement *)(parent->regularSubElements[2]));
                    *u = _u * 2.0;
                } else {
                    child = ((StochasticRadiosityElement *)(parent->regularSubElements[3]));
                    *u = (_u - 0.5) * 2.0;
                }
                *v = (_v - 0.5) * 2.0;
            }
            break;
        default:
            Logger::fatal(-1, "glrknElemRegSubElemAPnt", "Can handle only triangular or quadrilateral elements");
    }

    return child;
}

/**
Returns the leaf regular sub-element of 'top' at the point (u,v) (uniform
coordinates!). (u,v) is transformed to the coordinates of the corresponding
point on the leaf element. 'top' is a surface element, not a cluster
*/
StochasticRadiosityElement *
StochasticRadiosityElement::stchsRadElemRegLeafElemAPnt(StochasticRadiosityElement *top, double *u, double *v) {
    StochasticRadiosityElement *leaf;

    // Find leaf element of 'top' at (u,v)
    leaf = top;
    while ( leaf->regularSubElements ) {
        leaf = StochasticRadiosityElement::stchsRadElemRegSubElemAPnt(leaf, u, v);
    }

    return leaf;
}

Vector3D *
StochasticRadiosityElement::mntCarloRadInstCoord(const Vector3D *coord) {
    Vector3D *v = new Vector3D(coord->x, coord->y, coord->z);
    ElementHierarchyState::activeState().coords->add(v);
    return v;
}

Vector3D *
StochasticRadiosityElement::mntCarloRadInstNorm(const Vector3D *norm) {
    Vector3D *v = new Vector3D(norm->x, norm->y, norm->z);
    ElementHierarchyState::activeState().normals->add(v);
    return v;
}

Vector3D *
StochasticRadiosityElement::mntCarloRadInstTexCoord(const Vector3D *texCoord) {
    Vector3D *t = new Vector3D(texCoord->x, texCoord->y, texCoord->z);
    ElementHierarchyState::activeState().texCoords->add(t);
    return t;
}

Vertex *
StochasticRadiosityElement::mntCarloRadInstVtx(Vector3D *coord, Vector3D *norm, Vector3D *texCoord) {
    ArrayList<Patch *> *newPatchList = new ArrayList<Patch *>();
    Vertex *v = new Vertex(coord, norm, texCoord, newPatchList);
    ElementHierarchyState::activeState().vertices->add(v);
    return v;
}

Vertex *
StochasticRadiosityElement::mntCarloRadNewMidVtx(StochasticRadiosityElement *elem, const Vertex *v1, const Vertex *v2) {
    Vector3D coord;
    Vector3D norm;
    Vector3D texCoord;
    Vector3D *p;
    Vector3D *n;
    Vector3D *t;

    coord.midPoint(*(v1->point), *(v2->point));
    p = mntCarloRadInstCoord(&coord);

    if ( v1->normal && v2->normal ) {
        norm.midPoint(*(v1->normal), *(v2->normal));
        n = mntCarloRadInstNorm(&norm);
    } else {
        n = &elem->patch->normal;
    }

    if ( v1->textureCoordinates && v2->textureCoordinates ) {
        texCoord.midPoint(*(v1->textureCoordinates), *(v2->textureCoordinates));
        t = mntCarloRadInstTexCoord(&texCoord);
    } else {
        t = NULL;
    }

    return mntCarloRadInstVtx(p, n, t);
}

/**
Finds the surface element adjacent to 'elem' across the edge with index
'edgeNumber'. That edge is defined by:
  elem->vertices[edgeNumber]
  elem->vertices[(edgeNumber + 1) % elem->numberOfVertices]
The method searches the stochastic radiosity elements attached to the second
vertex and returns the first element (different from 'elem') that contains the
same edge with opposite orientation. Returns NULL when no neighbour is
found.
*/
StochasticRadiosityElement *
StochasticRadiosityElement::mntCarloRadElemNghbr(const StochasticRadiosityElement *elem, int edgeNumber) {
    const Vertex *from = elem->vertices[edgeNumber];
    const Vertex *to = elem->vertices[(edgeNumber + 1) % elem->numberOfVertices];

    for ( int i = 0; to->radianceData != NULL && i < to->radianceData->size(); i++ ) {
        Element *element = to->radianceData->get(i);
        if ( element->className != ELEMENT_STOCHASTIC_RADIOSITY ) {
            continue;
        }
        StochasticRadiosityElement *e = ((StochasticRadiosityElement *)(element));
        if ( e != elem &&
             ((e->numberOfVertices == 3 &&
               ((e->vertices[0] == to && e->vertices[1] == from) ||
                (e->vertices[1] == to && e->vertices[2] == from) ||
                (e->vertices[2] == to && e->vertices[0] == from)))
              || (e->numberOfVertices == 4 &&
                  ((e->vertices[0] == to && e->vertices[1] == from) ||
                   (e->vertices[1] == to && e->vertices[2] == from) ||
                   (e->vertices[2] == to && e->vertices[3] == from) ||
                   (e->vertices[3] == to && e->vertices[0] == from)))) ) {
            return e;
        }
    }

    return NULL;
}

Vertex *
StochasticRadiosityElement::stchsRadElemEdgeMidVtx(const StochasticRadiosityElement *elem, int edgeNumber) {
    Vertex *v = NULL;
    const Vertex *to = elem->vertices[(edgeNumber + 1) % elem->numberOfVertices];
    const StochasticRadiosityElement *neighbour = mntCarloRadElemNghbr(elem, edgeNumber);

    if ( neighbour && neighbour->regularSubElements ) {
        // Elem has a neighbour at the edge from 'from' to 'to'. This neighbouring
        // element has been subdivided before, so the edge midpoint vertex already
        // exists: it is the midpoint of the neighbour's edge leading from 'to' to
        // 'from'. This midpoint is a vertex of a regular sub-element of 'neighbour'.
        // Which regular sub-element and which vertex is determined from the diagrams
        // above
        int index;

        if ( to == neighbour->vertices[0] ) {
            index = 0;
        } else if ( to == neighbour->vertices[1] ) {
            index = 1;
        } else if ( to == neighbour->vertices[2] ) {
            index = 2;
        } else if ( to == neighbour->vertices[3] ) {
            index = 3;
        } else {
            index = -1;
        }

        switch ( neighbour->numberOfVertices ) {
            case 3:
                switch ( index ) {
                    case 0:
                        v = ((StochasticRadiosityElement *)(neighbour->regularSubElements[0]))->vertices[1];
                        break;
                    case 1:
                        v = ((StochasticRadiosityElement *)(neighbour->regularSubElements[1]))->vertices[2];
                        break;

                    case 2:
                        v = ((StochasticRadiosityElement *)(neighbour->regularSubElements[2]))->vertices[0];
                        break;
                    default:
                        Logger::error("EdgeMidpointVertex", "Invalid vertex index %d", index);
                }
                break;
            case 4:
                switch ( index ) {
                    case 0:
                        v = ((StochasticRadiosityElement *)(neighbour->regularSubElements[0]))->vertices[1];
                        break;
                    case 1:
                        v = ((StochasticRadiosityElement *)(neighbour->regularSubElements[1]))->vertices[2];
                        break;
                    case 2:
                        v = ((StochasticRadiosityElement *)(neighbour->regularSubElements[3]))->vertices[3];
                        break;
                    case 3:
                        v = ((StochasticRadiosityElement *)(neighbour->regularSubElements[2]))->vertices[0];
                        break;
                    default:
                        Logger::error("EdgeMidpointVertex", "Invalid vertex index %d", index);
                }
                break;
            default:
                Logger::fatal(-1, "EdgeMidpointVertex", "only triangular and quadrilateral elements are supported");
        }
    }

    return v;
}

Vertex *
StochasticRadiosityElement::mntCarloRadNewEdgeMidVtx(StochasticRadiosityElement *elem, int edgeNumber) {
    Vertex *v = StochasticRadiosityElement::stchsRadElemEdgeMidVtx(elem, edgeNumber);
    if ( v == NULL ) {
        // First time we split the edge, create the midpoint vertex
        const Vertex *from = elem->vertices[edgeNumber];
        const Vertex *to = elem->vertices[(edgeNumber + 1) % elem->numberOfVertices];
        v = mntCarloRadNewMidVtx(elem, from, to);
    }
    return v;
}

Vector3D
StochasticRadiosityElement::galerkinElementMidpoint(StochasticRadiosityElement *elem) {
    elem->midPoint.set(0.0, 0.0, 0.0);
    for ( int i = 0; i < elem->numberOfVertices; i++ ) {
        elem->midPoint.addition(elem->midPoint, *elem->vertices[i]->point);
    }
    elem->midPoint.inverseScaledCopy(((float)(elem->numberOfVertices)), elem->midPoint, Numeric::EPSILON_FLOAT);

    return elem->midPoint;
}

/**
Only for surface elements
*/
bool
StochasticRadiosityElement::stchsRadElemITex(const StochasticRadiosityElement *elem) {
    if ( elem->isCluster() ) {
        Logger::fatal(-1, "stchsRadElemITex", "this routine should not be called for cluster elements");
        return false;
    }
    const Material *mat = elem->patch->material;
    return mat->getBsdf() != NULL
        && (mat->getBsdf()->splitBsdfIsTextured() || PhongEmitDistFunc::edfIsTextured());
}

/**
Uses elem->Rd for surface elements
*/
float
StochasticRadiosityElement::stchsRadElemSclrRefl(const StochasticRadiosityElement *elem) {
    float rd;

    if ( elem->isCluster() ) {
        return 1.0;
    }

    rd = elem->Rd.maximumComponent();
    if ( rd < Numeric::EPSILON ) {
        // Avoid divisions by zero
        rd = Numeric::EPSILON_FLOAT;
    }
    return rd;
}

/**
Computes average reflectance and emittance of a surface sub-element
*/
void
StochasticRadiosityElement::mntCarloRadElemCompAvgReflAEmit(StochasticRadiosityElement *elem) {
    Patch *patch = elem->patch;
    int numberOfSamples;
    bool isTextured;
    int nbits;
    NiederreiterIndex msb1;
    NiederreiterIndex rMostSignificantBit2;
    NiederreiterIndex n;
    ColorRgbMutable albedo;
    ColorRgbMutable emittance;
    RayHit hit;
    hit.init(patch, &patch->midPoint, &patch->normal, patch->material);

    isTextured = StochasticRadiosityElement::stchsRadElemITex(elem);
    numberOfSamples = isTextured ? 100 : 1;
    albedo.clear();
    emittance.clear();
    StochasticRadiosityElement::stochasticRadiosityElementRange(elem, &nbits, &msb1, &rMostSignificantBit2);

    n = 1;
    for ( int i = 0; i < numberOfSamples; i++, n++ ) {
        ColorRgbMutable sample;
        NiederreiterIndex *xi = Niederreiter::NextNiedInRange(&n, +1, nbits, msb1, rMostSignificantBit2);
        hit.setUv(((double)(xi[0])) * RECIP, ((double)(xi[1])) * RECIP);
        unsigned int newFlags = hit.getFlags() | UV;
        hit.setFlags(newFlags);
        Vector3D position = hit.getPoint();
        patch->uniformPoint(hit.getUv().u, hit.getUv().v, &position);
        if ( patch->material->getBsdf() ) {
            sample.clear();
            if ( patch->material->getBsdf() != NULL ) {
                sample = patch->material->getBsdf()->splitBsdfScatteredPower(&hit, BRDF_DIFFUSE_COMPONENT);
            }
            albedo.add(albedo, sample);
        }
        if ( patch->material->getEdf() ) {
            sample = patch->material->getEdf()->phongEmittance(&hit, DIFFUSE_COMPONENT);
            emittance.add(emittance, sample);
        }
    }
    elem->Rd.scaleInverse(((float)(numberOfSamples)), albedo);
    elem->Ed.scaleInverse(((float)(numberOfSamples)), emittance);
}

/**
Initial push operation for surface sub-elements
*/
void
StochasticRadiosityElement::mntCarloRadInitSurfPush(const StochasticRadiosityElement *parent, StochasticRadiosityElement *child) {
    child->sourceRad = parent->sourceRad;
    StochasticRadiosityElement::stchsRadElemPushRadn(parent, child, parent->radiance, child->radiance);
    StochasticRadiosityElement::stchsRadElemPushRadn(parent, child, parent->unShotRadiance, child->unShotRadiance);
    StochasticRadiosityElement::stchsRadElemPushImp(&parent->importance, &child->importance);
    StochasticRadiosityElement::stchsRadElemPushImp(&parent->sourceImportance, &child->sourceImportance);
    StochasticRadiosityElement::stchsRadElemPushImp(&parent->unShotImportance, &child->unShotImportance);
    child->rayIndex = parent->rayIndex;
    child->quality = parent->quality;
    child->samplingProbability = parent->samplingProbability * child->area / parent->area;

    child->Rd = parent->Rd;
    child->Ed = parent->Ed;
    mntCarloRadElemCompAvgReflAEmit(child);
}

/**
Creates a sub-element of the element "*parent", stores it as the
sub-element number "childNumber". Tha value of "v3" is unused in the
process of triangle subdivision.
*/
StochasticRadiosityElement *
StochasticRadiosityElement::mntCarloRadCreateSurfSubElem(
        StochasticRadiosityElement *parent,
        int childNumber,
        Vertex *v0,
        Vertex *v1,
        Vertex *v2,
        Vertex *v3)
{
    StochasticRadiosityBasisState &basisState = StochasticRadiosityBasisState::activeState();
    StochasticRadiosityElement *elem = createElement();
    parent->regularSubElements[childNumber] = elem;

    elem->patch = parent->patch;
    elem->numberOfVertices = parent->numberOfVertices;
    elem->vertices[0] = v0;
    elem->vertices[1] = v1;
    elem->vertices[2] = v2;
    elem->vertices[3] = v3;
    for ( int i = 0; i < elem->numberOfVertices; i++ ) {
        vertexAttachElement(elem->vertices[i], elem);
    }

    elem->area = 0.25f * parent->area; // Regular elements, regular subdivision
    elem->midPoint = galerkinElementMidpoint(elem);

    elem->parent = parent;
    elem->childNumber = ((char)(childNumber));
    elem->transformToParent = elem->numberOfVertices == 3
                              ? &basisState.triangleUpTransform[childNumber]
                              : &basisState.quadUpTransform[childNumber];

    Coefficientsmcrad::allocCoefficients(elem);
    Coefficientsmcrad::stchsRadClearCoeff(elem->radiance, elem->basis);
    Coefficientsmcrad::stchsRadClearCoeff(elem->unShotRadiance, elem->basis);
    Coefficientsmcrad::stchsRadClearCoeff(elem->receivedRadiance, elem->basis);
    elem->importance = elem->unShotImportance = elem->receivedImportance = 0.0;
    mntCarloRadInitSurfPush(parent, elem);

    return elem;
}

/**
Create sub-elements: regular subdivision, see drawings above
*/
StochasticRadiosityElement **
StochasticRadiosityElement::mntCarloRadRegSbdvdTri(StochasticRadiosityElement *element, const RenderOptions *renderOptions) {
    (void)renderOptions;

    Vertex *v0 = element->vertices[0];
    Vertex *v1 = element->vertices[1];
    Vertex *v2 = element->vertices[2];
    Vertex *m0 = mntCarloRadNewEdgeMidVtx(element, 0);
    Vertex *m1 = mntCarloRadNewEdgeMidVtx(element, 1);
    Vertex *m2 = mntCarloRadNewEdgeMidVtx(element, 2);

    mntCarloRadCreateSurfSubElem(element, 0, v0, m0, m2, NULL);
    mntCarloRadCreateSurfSubElem(element, 1, m0, v1, m1, NULL);
    mntCarloRadCreateSurfSubElem(element, 2, m2, m1, v2, NULL);
    mntCarloRadCreateSurfSubElem(element, 3, m1, m2, m0, NULL);

    return ((StochasticRadiosityElement **)(element->regularSubElements));
}

StochasticRadiosityElement **
StochasticRadiosityElement::mntCarloRadRegSbdvdQuad(StochasticRadiosityElement *element, const RenderOptions *renderOptions) {
    (void)renderOptions;

    Vertex *v0 = element->vertices[0];
    Vertex *v1 = element->vertices[1];
    Vertex *v2 = element->vertices[2];
    Vertex *v3 = element->vertices[3];
    Vertex *m0 = mntCarloRadNewEdgeMidVtx(element, 0);
    Vertex *m1 = mntCarloRadNewEdgeMidVtx(element, 1);
    Vertex *m2 = mntCarloRadNewEdgeMidVtx(element, 2);
    Vertex *m3 = mntCarloRadNewEdgeMidVtx(element, 3);
    Vertex *mm = mntCarloRadNewMidVtx(element, m0, m2);

    mntCarloRadCreateSurfSubElem(element, 0, v0, m0, mm, m3);
    mntCarloRadCreateSurfSubElem(element, 1, m0, v1, m1, mm);
    mntCarloRadCreateSurfSubElem(element, 2, m3, mm, m2, v3);
    mntCarloRadCreateSurfSubElem(element, 3, mm, m1, v2, m2);

    return ((StochasticRadiosityElement **)(element->regularSubElements));
}

/**
Subdivides given triangle or quadrangle into four sub-elements if not yet
done so before. Returns the list of created sub-elements
*/
StochasticRadiosityElement **
StochasticRadiosityElement::stchsRadElemRegSbdvdElem(
    StochasticRadiosityElement *element,
    const RenderOptions *renderOptions)
{
    if ( element->regularSubElements ) {
        return ((StochasticRadiosityElement **)(element->regularSubElements));
    }

    if ( element->isCluster() ) {
        Logger::fatal(-1, "galerkinElementRegularSubDivide", "Cannot regularly subdivide cluster elements");
        return NULL;
    }

    if ( element->patch->jacobian ) {
        static bool flag = false;
        if ( !flag ) {
            Logger::warning("galerkinElementRegularSubDivide",
                       "irregular quadrilateral patches are not correctly handled (but you probably won't notice it)");
        }
        flag = true;
    }

    // Create the sub-elements
    element->regularSubElements = ((Element **)(new StochasticRadiosityElement *[4]));
    switch ( element->numberOfVertices ) {
        case 3:
            mntCarloRadRegSbdvdTri(element, renderOptions);
            break;
        case 4:
            mntCarloRadRegSbdvdQuad(element, renderOptions);
            break;
        default:
            Logger::fatal(-1, "galerkinElementRegularSubDivide", "invalid element: not 3 or 4 vertices");
    }
    return ((StochasticRadiosityElement **)(element->regularSubElements));
}

void
StochasticRadiosityElement::mntCarloRadDestroyElem(StochasticRadiosityElement *elem) {
    if ( elem->isCluster() ) {
        ElementHierarchyState::activeState().nr_clusters--;
    }
    ElementHierarchyState::activeState().nr_elements--;

    if ( elem->irregularSubElements ) {
        for ( int j = 0; elem->irregularSubElements != NULL && j < elem->irregularSubElements->size(); j++ ) {
            delete elem->irregularSubElements->get(j);
        }
        delete elem->irregularSubElements;
        elem->irregularSubElements = NULL;
    }

    if ( elem->regularSubElements ) {
        delete[] elem->regularSubElements;
    }
    for ( int i = 0; i < elem->numberOfVertices; i++ ) {
        for ( int j = 0; elem->vertices[i]->radianceData != NULL && j < elem->vertices[i]->radianceData->size(); j++ ) {
            delete elem->vertices[i]->radianceData->get(j);
        }
        delete elem->vertices[i]->radianceData;
        elem->vertices[i]->radianceData = NULL;
    }
    Coefficientsmcrad::disposeCoefficients(elem);
    delete elem;
}

void
StochasticRadiosityElement::mntCarloRadDestroySurfElem(StochasticRadiosityElement *elem) {
    if ( !elem ) {
        return;
    }
    if ( elem->regularSubElements != NULL ) {
        for ( int i = 0; i < 4; i++ ) {
            mntCarloRadDestroySurfElem(((StochasticRadiosityElement *)(elem->regularSubElements[i])));
        }
    }
    mntCarloRadDestroyElem(elem);
}

void
StochasticRadiosityElement::stchsRadElemDestroy(StochasticRadiosityElement *elem) {
    mntCarloRadDestroySurfElem(elem);
}

void
StochasticRadiosityElement::stchsRadElemDestroyClustHier(StochasticRadiosityElement *top) {
    if ( top == NULL || !top->isCluster() ) {
        return;
    }
    for ( int i = 0; top->irregularSubElements != NULL && i < top->irregularSubElements->size(); i++ ) {
        StochasticRadiosityElement *element = ((StochasticRadiosityElement *)(top->irregularSubElements->get(i)));
        if ( element->isCluster() ) {
            StochasticRadiosityElement::stchsRadElemDestroyClustHier(element);
        }
    }
    mntCarloRadDestroyElem(top);
}

inline bool
StochasticRadiosityElement::regularChild(const StochasticRadiosityElement *child) {
    return (child->childNumber >= 0 && child->childNumber <= 3);
}

void
StochasticRadiosityElement::stchsRadElemPushRadn(
    const StochasticRadiosityElement *parent,
    StochasticRadiosityElement *child,
    const ColorRgbMutable *parentRadiance,
    ColorRgbMutable *childRadiance)
{
    if ( parent->isCluster() || child->basis->size == 1 ) {
        childRadiance[0].add(childRadiance[0], parentRadiance[0]);
    } else if ( regularChild(child) && child->basis == parent->basis ) {
        Basismcrad::filterColorDown((const ColorRgb *)parentRadiance, &(*child->basis->regularFilter)[child->childNumber], (ColorRgb *)childRadiance,
                        child->basis->size);
    } else {
        Logger::fatal(-1, "stchsRadElemPushRadn",
                 "Not implemented for higher order approximations on irregular child elements or for different parent and child basis");
    }
}

void
StochasticRadiosityElement::stchsRadElemPushImp(const float *parentImportance, float *childImportance) {
    *childImportance += *parentImportance;
}

void
StochasticRadiosityElement::stchsRadElemPullRadn(
    const StochasticRadiosityElement *parent,
    const StochasticRadiosityElement *child,
    ColorRgbMutable *parentRad,
    const ColorRgbMutable *childRad)
{
    float areaFactor = child->area / parent->area;
    if ( parent->isCluster() || child->basis->size == 1 ) {
        parentRad[0].addScaled(parentRad[0], areaFactor, childRad[0]);
    } else if ( regularChild(child) && child->basis == parent->basis ) {
        Basismcrad::filterColorUp((const ColorRgb *)childRad, &(*child->basis->regularFilter)[child->childNumber],
                      (ColorRgb *)parentRad, child->basis->size, areaFactor);
    } else {
        Logger::fatal(-1, "stchsRadElemPullRadn",
                 "Not implemented for higher order approximations on irregular child elements or for different parent and child basis");
    }
}

void
StochasticRadiosityElement::stchsRadElemPullImp(const StochasticRadiosityElement *parent, const StochasticRadiosityElement *child, float *parent_imp, const float *child_imp) {
    *parent_imp += child->area / parent->area * (*child_imp);
}

#endif
