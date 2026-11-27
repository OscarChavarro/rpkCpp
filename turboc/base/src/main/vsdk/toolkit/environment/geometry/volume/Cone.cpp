#include <stddef.h>
#include <math.h>

#include "java/util/ArrayList.txx"
#include "vsdk/toolkit/common/VSDK.h"
#include "vsdk/toolkit/environment/geometry/element/Ray.h"
#include "vsdk/toolkit/environment/geometry/element/RayHit.h"
#include "vsdk/toolkit/common/linealAlgebra/Matrix4x4d.h"
#include "vsdk/toolkit/environment/geometry/volume/Cone.h"
#include "vsdk/toolkit/environment/geometry/volume/Tube.h"
#include "vsdk/toolkit/environment/geometry/element/GeometryBounds.h"
#include "java/lang/Double.h"
#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/PolyhedralBoundedSolid.h"
#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/PolyhedralBoundedSolidEulerOperators.h"
#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/PolyhedralBoundedSolidEulerConstructions.h"
#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/PolyhedralBoundedSolidValidationEngine.h"
#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/nodes/_PolyhedralBoundedSolidFace.h"
#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/nodes/_PolyhedralBoundedSolidHalfEdge.h"
#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/nodes/_PolyhedralBoundedSolidLoop.h"
#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/nodes/_PolyhedralBoundedSolidVertex.h"
#include "vsdk/toolkit/common/Cpp98Support.h"
Cone::Cone(double bottomRadius, double topRadius, double height) :
    bottomRadius(bottomRadius), topRadius(topRadius), height(height) {
    // Turbo C++ 3.0 crashes with the new of a template class as an argument
    MemberEntityControlAccessor<Cone, double> *accessor;
    accessor = new MemberEntityControlAccessor<Cone, double>(
        &Cone::getBottomRadius, &Cone::setBottomRadius);
    addControlSpecification("double;bottomRadius;(0, INFINITE)", accessor);
    accessor = new MemberEntityControlAccessor<Cone, double>(
        &Cone::getTopRadius, &Cone::setTopRadius);
    addControlSpecification("double;topRadius;[0, INFINITE)", accessor);
    accessor = new MemberEntityControlAccessor<Cone, double>(
        &Cone::getHeight, &Cone::setHeight);
    addControlSpecification("double;height;(0, INFINITE)", accessor);
}

double Cone::sq(double v){ return v*v; }
BOOLEAN Cone::approxEq(double a, double b){ return Cpp98Support::abs(a-b) <= VSDK::EPSILON; }

double Cone::getBottomRadius() const { return bottomRadius; }
double Cone::getTopRadius() const { return topRadius; }
double Cone::getHeight() const { return height; }
void Cone::setBottomRadius(double value) { bottomRadius = value; }
void Cone::setTopRadius(double value) { topRadius = value; }
void Cone::setHeight(double value) { height = value; }

Ray* Cone::doIntersectionCylinder(const Ray& inOutRay, double inR, double inH, RayHit* outInfo) {
    double ox=inOutRay.getOrigin().x(), oy=inOutRay.getOrigin().y(), oz=inOutRay.getOrigin().z();
    double dx=inOutRay.getDirection().x(), dy=inOutRay.getDirection().y(), dz=inOutRay.getDirection().z();
    double A = sq(dx)+sq(dy); if (Cpp98Support::abs(A) <= VSDK::EPSILON) return NULL;
    double B = 2*((dx*ox)+(dy*oy));
    double C = sq(ox)+sq(oy)-sq(inR);
    double disc = sq(B)-4*A*C; if (disc <= VSDK::EPSILON) return NULL;
    double t0 = (-B-sqrt(disc))/(2*A);
    if (t0 > VSDK::EPSILON) {
        double pz = oz + dz*t0;
        if (pz > inH || pz < 0) return NULL;
        if (outInfo != NULL) {
            double px=ox+dx*t0, py=oy+dy*t0;
            outInfo->point = Vector3Dd(px,py,pz);
            outInfo->normal = Vector3Dd(px,py,0).normalized();
        }
        Ray hRay = inOutRay.withT(t0);
        return new Ray(hRay);
    }
    return NULL;
}

Ray* Cone::doIntersectionCone(const Ray& inOutRay, double inR, double inH, RayHit* outInfo) {
    double ox=inOutRay.getOrigin().x(), oy=inOutRay.getOrigin().y(), oz=inOutRay.getOrigin().z();
    double dx=inOutRay.getDirection().x(), dy=inOutRay.getDirection().y(), dz=inOutRay.getDirection().z();
    if (inH <= VSDK::EPSILON) return NULL;
    double shiftedOz = oz - inH;
    double ratio = inR / inH;
    double ratioSquared = sq(ratio);
    double A = sq(dx)+sq(dy)-sq(dz*ratio); if (Cpp98Support::abs(A)<=VSDK::EPSILON) return NULL;
    double B = 2*((dx*ox)+(dy*oy)-(dz*shiftedOz*ratioSquared));
    double C = sq(ox)+sq(oy)-sq(shiftedOz*ratio);
    double disc = sq(B)-4*A*C; if (disc <= VSDK::EPSILON) return NULL;
    double t0 = (-B-sqrt(disc))/(2*A);
    if (t0 > VSDK::EPSILON) {
        double shiftedPz = shiftedOz + dz*t0;
        if (shiftedPz > 0 || shiftedPz < -inH) return NULL;
        if (outInfo != NULL) {
            double px=ox+dx*t0, py=oy+dy*t0;
            outInfo->point = Vector3Dd(px, py, shiftedPz + inH);
            outInfo->normal = Vector3Dd(px, py, -shiftedPz * ratioSquared).normalized();
        }
        Ray hRay = inOutRay.withT(t0);
        return new Ray(hRay);
    }
    return NULL;
}

Ray* Cone::doIntersectionTap(const Ray& inOutRay, double inR, double inH, RayHit* outInfo) {
    double dx=inOutRay.getDirection().x(), dy=inOutRay.getDirection().y(), dz=inOutRay.getDirection().z();
    double ox=inOutRay.getOrigin().x(), oy=inOutRay.getOrigin().y(), oz=inOutRay.getOrigin().z();
    if (Cpp98Support::abs(dz) > VSDK::EPSILON) {
        double t=(inH-oz)/dz;
        if (t > VSDK::EPSILON) {
            double px=ox+dx*t, py=oy+dy*t;
            if (sq(px)+sq(py) < sq(inR)) {
                if (outInfo != NULL) {
                    outInfo->normal = Vector3Dd(0,0,1);
                    outInfo->point = Vector3Dd(px,py,inH);
                }
                Ray hRay = inOutRay.withT(t);
                return new Ray(hRay);
            }
        }
    }
    return NULL;
}

Ray* Cone::doIntersectionFirstHit(const Ray& inOutRay) {
    RayHit hit;
    if (doIntersectionFirstHit(inOutRay, &hit) && hit.getRay() != NULL) {
        return new Ray(*hit.getRay());
    }
    return NULL;
}

BOOLEAN Cone::doIntersectionFirstHit(const Ray& inOutRay, RayHit* outHit) {
    RayHit infoTap1, infoTap2, infoBody;
    Ray* bodyHit = NULL;
    Ray* tap1Hit = NULL;
    Ray* tap2Hit = NULL;
    Ray* winner = NULL;
    RayHit* winnerInfo = NULL;

    if (topRadius < VSDK::EPSILON && bottomRadius > VSDK::EPSILON) {
        bodyHit = doIntersectionCone(inOutRay, bottomRadius, height, &infoBody);
        tap1Hit = doIntersectionTap(inOutRay, bottomRadius, 0, &infoTap1);
        if ((tap1Hit != NULL && bodyHit == NULL) ||
            (tap1Hit != NULL && bodyHit != NULL && tap1Hit->getT() < bodyHit->getT())) {
            infoTap1.normal = infoTap1.normal.multiply(-1);
            winner = new Ray(inOutRay.withT(tap1Hit->getT()));
            winnerInfo = &infoTap1;
        }
        else if (bodyHit != NULL) {
            winner = new Ray(inOutRay.withT(bodyHit->getT()));
            winnerInfo = &infoBody;
        }
    }
    else if (approxEq(bottomRadius, topRadius)) {
        int nearest = -1;
        bodyHit = doIntersectionCylinder(inOutRay, bottomRadius, height, &infoBody);
        tap1Hit = doIntersectionTap(inOutRay, bottomRadius, 0, &infoTap1);
        tap2Hit = doIntersectionTap(inOutRay, bottomRadius, height, &infoTap2);

        if (bodyHit != NULL &&
            ((tap1Hit != NULL && bodyHit->getT() < tap1Hit->getT()) || tap1Hit == NULL) &&
            ((tap2Hit != NULL && bodyHit->getT() < tap2Hit->getT()) || tap2Hit == NULL)) nearest = 1;
        else if (tap1Hit != NULL &&
            ((bodyHit != NULL && tap1Hit->getT() < bodyHit->getT()) || bodyHit == NULL) &&
            ((tap2Hit != NULL && tap1Hit->getT() < tap2Hit->getT()) || tap2Hit == NULL)) nearest = 3;
        else if (tap2Hit != NULL) nearest = 2;

        if (nearest == 1) { winner = new Ray(inOutRay.withT(bodyHit->getT())); winnerInfo = &infoBody; }
        else if (nearest == 2) { winner = new Ray(inOutRay.withT(tap2Hit->getT())); winnerInfo = &infoTap2; }
        else if (nearest == 3) { winner = new Ray(inOutRay.withT(tap1Hit->getT())); winnerInfo = &infoTap1; }
    }

    if (bodyHit) delete bodyHit;
    if (tap1Hit) delete tap1Hit;
    if (tap2Hit) delete tap2Hit;

    if (winner == NULL) return FALSE;

    if (outHit != NULL) {
        outHit->setRay(*winner);
        outHit->point = Vector3Dd(winnerInfo->point);
        outHit->normal = Vector3Dd(winnerInfo->normal).normalized();
        outHit->tangent = Vector3Dd(winnerInfo->tangent);
        outHit->u = winnerInfo->u;
        outHit->v = winnerInfo->v;
        outHit->material = winnerInfo->material;
        outHit->texture = winnerInfo->texture;
        outHit->normalMap = winnerInfo->normalMap;
    }
    delete winner;
    return TRUE;
}

void Cone::doExtraInformation(const Ray& inRay, double, RayHit* outData) {
    if (outData == NULL) return;
    RayHit hit;
    if (doIntersectionFirstHit(inRay.withT(1e308), &hit)) {
        outData->clone(hit);
    }
}

/**
Check the general interface contract in superclass method
Geometry.doContainmentTest. The cone (or truncated cone, or cylinder)
has its axis along Z, its base of radius `bottomRadius` at z = 0 and its
top of radius `topRadius` at z = `height`. The distance to the side is
measured perpendicular to the side, so the tolerance is the same for the
side and for the caps.
@param p point to classify, in the space of the cone
@param distanceTolerance distance to the surface of the cone under which
the point is classified as `LIMIT`
@return INSIDE, OUTSIDE or LIMIT constant value
*/
int Cone::doContainmentTest(const Vector3Dd& p, double distanceTolerance)
{
    if ( height <= 0 ) {
        return OUTSIDE;
    }
    double z = p.z();
    double radialDistance = sqrt(p.x() * p.x() + p.y() * p.y());
    double radiusAtZ = bottomRadius + (topRadius - bottomRadius) * (z / height);
    double radiusChange = bottomRadius - topRadius;
    double sideCosine = height / sqrt(height * height + radiusChange * radiusChange);
    double sideDistance = (radialDistance - radiusAtZ) * sideCosine;

    if ( z < -distanceTolerance || z > height + distanceTolerance ||
         sideDistance > distanceTolerance ) {
        return OUTSIDE;
    }
    if ( z > distanceTolerance && z < height - distanceTolerance &&
         sideDistance < -distanceTolerance ) {
        return INSIDE;
    }
    return LIMIT;
}

void Cone::getBounds(GeometryBounds& result) const {
    const double r = (bottomRadius > topRadius) ? bottomRadius : topRadius;
    if (!Double::isFinite(r) || !Double::isFinite(height) || height <= 0 ||
        bottomRadius < 0 || topRadius < 0) { result.setIndeterminate(); return; }
    result.setFinite(Vector3Dd(-r,-r,0), Vector3Dd(r,r,height));
}

double* Cone::getMinMax() {
    double* m = new double[6];
    double r = (bottomRadius > topRadius) ? bottomRadius : topRadius;
    m[0]=-r; m[1]=-r; m[2]=0; m[3]=r; m[4]=r; m[5]=height;
    return m;
}

PolyhedralBoundedSolid* Cone::exportToPolyhedralBoundedSolid()
{
    return buildPolyhedralBoundedSolid(
        DEFAULT_CIRCUMFERENCE_DIVISIONS, DEFAULT_HEIGHT_DIVISIONS);
}

PolyhedralBoundedSolid* Cone::exportToPolyhedralBoundedSolid(
    int circumferenceDivisions, int heightDivisions, BOOLEAN withCaps)
{
    int normalizedCircumferenceDivisions =
        circumferenceDivisions > MIN_CIRCUMFERENCE_DIVISIONS ?
        circumferenceDivisions : MIN_CIRCUMFERENCE_DIVISIONS;
    int normalizedHeightDivisions =
        heightDivisions > MIN_HEIGHT_DIVISIONS ?
        heightDivisions : MIN_HEIGHT_DIVISIONS;

    if ( !withCaps ) {
        return buildOpenLamina(normalizedCircumferenceDivisions);
    }
    if ( normalizedCircumferenceDivisions == DEFAULT_CIRCUMFERENCE_DIVISIONS &&
         normalizedHeightDivisions == DEFAULT_HEIGHT_DIVISIONS ) {
        return exportToPolyhedralBoundedSolid();
    }

    return buildPolyhedralBoundedSolid(normalizedCircumferenceDivisions,
        normalizedHeightDivisions);
}

/**
Current implementation of the cylinder follows the idea suggested on
section [MANT1988].12.3.1 and program [MANT1988].12.4, where the
cylinder is built upon a circular lamina base and an extrusion
(translational sweep) operation. The cone case is done manually,
*/
void Cone::closeTopFaceToApex(PolyhedralBoundedSolid* solid, double apexZ)
{
    closeFaceToApex(solid, 1, apexZ);
}

void Cone::closeFaceToApex(PolyhedralBoundedSolid* solid, int faceId,
    double apexZ)
{
    Tube::splitFaceIntoFan(solid, faceId, Vector3Dd(0.0, 0.0, apexZ));
}

PolyhedralBoundedSolid* Cone::buildPolyhedralBoundedSolid(int nsides,
    int heightDivisions)
{
    PolyhedralBoundedSolid* solid;
    Matrix4x4d T;
    Matrix4x4d S;
    Matrix4x4d M;

    solid = PolyhedralBoundedSolidEulerConstructions::createCircularLamina(
        0.0, 0.0, bottomRadius, 0.0, nsides);

    if ( topRadius > VSDK::EPSILON && bottomRadius > VSDK::EPSILON ) {
        double prevRadius = bottomRadius;
        double zStep = height / ((double)heightDivisions);
        int i;
        for ( i = 1; i <= heightDivisions; i++ ) {
            double nextRadius = bottomRadius + (topRadius - bottomRadius) *
                (((double)i) / ((double)heightDivisions));
            double f = nextRadius / prevRadius;
            T = Matrix4x4d();
            T = T.translation(0.0, 0.0, zStep);
            S = Matrix4x4d();
            S = S.scale(f, f, 1.0);
            M = T.multiply(S);
            PolyhedralBoundedSolidEulerConstructions::translationalSweepExtrudeFacePlanar(
                solid, solid->findFace(1), M);
            prevRadius = nextRadius;
        }
    }
    else if ( topRadius <= VSDK::EPSILON && bottomRadius > VSDK::EPSILON ) {
        // Cone case, with optional vertical subdivisions.
        double prevRadius = bottomRadius;
        double zStep = height / ((double)heightDivisions);
        int i;
        for ( i = 1; i < heightDivisions; i++ ) {
            double nextRadius = bottomRadius *
                (1.0 - (((double)i) / ((double)heightDivisions)));
            double f = nextRadius / prevRadius;
            T = Matrix4x4d();
            T = T.translation(0.0, 0.0, zStep);
            S = Matrix4x4d();
            S = S.scale(f, f, 1.0);
            M = T.multiply(S);
            PolyhedralBoundedSolidEulerConstructions::translationalSweepExtrudeFacePlanar(
                solid, solid->findFace(1), M);
            prevRadius = nextRadius;
        }
        closeTopFaceToApex(solid, height);
    }
    return solid;
}

PolyhedralBoundedSolid* Cone::buildOpenLamina(int nsides)
{
    PolyhedralBoundedSolid* solid;
    int k;

    if ( topRadius <= VSDK::EPSILON ) {
        //- Cone with apex: both faces of a disk as fans, center lifted ---
        // The fans of faces 1 and 2 end at the vertices n+1 and n+2
        solid = Tube::createWasherLamina(0.0, bottomRadius, 0.0, nsides,
            TRUE);
        solid->findVertex(nsides + 1)->position = Vector3Dd(0.0, 0.0, height);
        solid->findVertex(nsides + 2)->position = Vector3Dd(0.0, 0.0, height);
        PolyhedralBoundedSolidValidationEngine::validateIntermediate(solid);
        return solid;
    }

    //- Washer split in quadrilaterals, internal circle lifted ----------
    // The external circle has the vertices 1..n at z = 0; the internal
    // vertex at the angle of the external vertex k+1 is
    // n+1+((n-k) mod n). Each one is placed above its external pair,
    // scaled to the top radius, so both have exactly the same longitude
    // and the quadrilaterals between them are planar.
    solid = Tube::createWasherLamina(0.5 * bottomRadius, bottomRadius, 0.0,
        nsides, TRUE);
    double scale = topRadius / bottomRadius;
    for ( k = 0; k < nsides; k++ ) {
        _PolyhedralBoundedSolidVertex* bottom = solid->findVertex(k + 1);
        _PolyhedralBoundedSolidVertex* top =
            solid->findVertex(nsides + 1 + ((nsides - k) % nsides));
        top->position = Vector3Dd(bottom->position.x() * scale,
            bottom->position.y() * scale, height);
    }
    PolyhedralBoundedSolidValidationEngine::validateIntermediate(solid);
    return solid;
}

const Class Cone::CLASS = { "Cone", &Solid::CLASS };

const Class *
Cone::getClass() const
{
    return &CLASS;
}
