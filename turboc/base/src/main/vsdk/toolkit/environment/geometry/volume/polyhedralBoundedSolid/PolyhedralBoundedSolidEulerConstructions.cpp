#include <math.h>
#include "java/util/ArrayList.txx"
#include "vsdk/toolkit/common/logging/Logger.h"
#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/PolyhedralBoundedSolid.h"
#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/PolyhedralBoundedSolidEulerOperators.h"
#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/PolyhedralBoundedSolidGeometricValidator.h"
#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/PolyhedralBoundedSolidEulerConstructions.h"
#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/PolyhedralBoundedSolidValidationEngine.h"
#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/nodes/_PolyhedralBoundedSolidFace.h"
#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/nodes/_PolyhedralBoundedSolidHalfEdge.h"
#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/nodes/_PolyhedralBoundedSolidLoop.h"
#include "vsdk/toolkit/environment/geometry/volume/polyhedralBoundedSolid/nodes/_PolyhedralBoundedSolidVertex.h"

void PolyhedralBoundedSolidEulerConstructions::addArcToExistingFace(
    PolyhedralBoundedSolid* solid, int faceId, int vertexId, double cx,
    double cy, double radius, double height, double phi1, double phi2, int n)
{
    double x;
    double y;
    double angle;
    double inc;
    int prev;
    int i;
    int nextVertexId;

    angle = phi1 * M_PI / 180.0;
    inc = ((phi2 - phi1) / (n)) * M_PI / 180.0;
    prev = vertexId;
    for ( i = 0; i < n; i++ ) {
        angle += inc;
        // Snap trig values to 1e-10 grid so that equal angular positions on
        // two separately constructed circles always produce bit-identical
        // coordinates, preventing near-coincident vertices downstream.
        // std::floor(v + 0.5) mimics Java's Math.round.
        x = floor((cx + radius * cos(angle)) * 1.0e10 + 0.5) / 1.0e10;
        y = floor((cy + radius * sin(angle)) * 1.0e10 + 0.5) / 1.0e10;
        nextVertexId = solid->getMaxVertexId() + 1;
        PolyhedralBoundedSolidEulerOperators::smev(solid, faceId, prev,
            nextVertexId, Vector3Dd(x, y, height));
        prev = nextVertexId;
    }
    PolyhedralBoundedSolidValidationEngine::validateIntermediate(solid);
}

PolyhedralBoundedSolid* PolyhedralBoundedSolidEulerConstructions::createCircularLamina(
    double cx, double cy, double rad, double h, int n)
{
    PolyhedralBoundedSolid* solid;

    solid = new PolyhedralBoundedSolid();
    PolyhedralBoundedSolidEulerOperators::mvfs(solid,
        Vector3Dd(cx + rad, cy, h), 1, 1);
    addArcToExistingFace(solid, 1, 1, cx, cy, rad, h, 0,
        (n - 1) * 360.0 / n, n-1);
    PolyhedralBoundedSolidEulerOperators::smef(solid, 1, n, 1, 2);
    PolyhedralBoundedSolidValidationEngine::validateIntermediate(solid);
    return solid;
}

void PolyhedralBoundedSolidEulerConstructions::translationalSweepExtrudeFace(
    PolyhedralBoundedSolid* solid, _PolyhedralBoundedSolidFace* face,
    const Matrix4x4d& transformationMatrix)
{
    _PolyhedralBoundedSolidLoop* l;
    _PolyhedralBoundedSolidHalfEdge* first;
    _PolyhedralBoundedSolidHalfEdge* scan;
    _PolyhedralBoundedSolidVertex* v;
    Vector3Dd newPos;
    int i;

    for ( i = 0; i < face->boundariesList.size(); i++ ) {
        l = face->boundariesList.get(i);
        first = l->boundaryStartHalfEdge;
        scan = first->next();
        v = scan->startingVertex;
        newPos = transformationMatrix.multiply(v->position);
        PolyhedralBoundedSolidEulerOperators::lmev(solid, scan, scan,
            solid->getMaxVertexId()+1, newPos);
        while ( scan != first ) {
            v = scan->next()->startingVertex;
            newPos = transformationMatrix.multiply(v->position);
            PolyhedralBoundedSolidEulerOperators::lmev(solid, scan->next(),
                scan->next(), solid->getMaxVertexId()+1, newPos);
            PolyhedralBoundedSolidEulerOperators::lmef(solid,
                scan->previous(), scan->next()->next(),
                solid->getMaxFaceId()+1);
            scan = (scan->next()->mirrorHalfEdge())->next();
        }
        PolyhedralBoundedSolidEulerOperators::lmef(solid, scan->previous(),
            scan->next()->next(), solid->getMaxFaceId()+1);
    }
    PolyhedralBoundedSolidValidationEngine::validateIntermediate(solid);
}

void PolyhedralBoundedSolidEulerConstructions::translationalSweepExtrudeFacePlanar(
    PolyhedralBoundedSolid* solid, _PolyhedralBoundedSolidFace* face,
    const Matrix4x4d& transformationMatrix)
{
    _PolyhedralBoundedSolidLoop* l;
    _PolyhedralBoundedSolidHalfEdge* first;
    _PolyhedralBoundedSolidHalfEdge* scan;
    _PolyhedralBoundedSolidVertex* v;
    Vector3Dd newPos;
    ArrayList<int> newFaces;
    int i;
    int newFaceId;

    for ( i = 0; i < face->boundariesList.size(); i++ ) {
        l = face->boundariesList.get(i);
        first = l->boundaryStartHalfEdge;
        scan = first->next();
        v = scan->startingVertex;
        newPos = transformationMatrix.multiply(v->position);
        PolyhedralBoundedSolidEulerOperators::lmev(solid, scan, scan,
            solid->getMaxVertexId()+1, newPos);
        while ( scan != first ) {
            v = scan->next()->startingVertex;
            newPos = transformationMatrix.multiply(v->position);
            PolyhedralBoundedSolidEulerOperators::lmev(solid, scan->next(),
                scan->next(), solid->getMaxVertexId()+1, newPos);
            newFaceId = solid->getMaxFaceId()+1;
            PolyhedralBoundedSolidEulerOperators::lmef(solid,
                scan->previous(), scan->next()->next(), newFaceId);
            newFaces.add(newFaceId);
            scan = (scan->next()->mirrorHalfEdge())->next();
        }
        newFaceId = solid->getMaxFaceId()+1;
        PolyhedralBoundedSolidEulerOperators::lmef(solid, scan->previous(),
            scan->next()->next(), newFaceId);
        newFaces.add(newFaceId);
    }

    _PolyhedralBoundedSolidFace* newFace;
    for ( i = 0; i < newFaces.size(); i++ ) {
        newFaceId = newFaces.get(i);
        newFace = solid->findFace(newFaceId);
        if ( newFace == NULL ) {
            Logger::reportMessage("PolyhedralBoundedSolidEulerConstructions",
                Logger::WARNING, "translationalSweepExtrudeFacePlanar",
                "Swept face not found, planarity check skipped.");
            continue;
        }
        if ( !PolyhedralBoundedSolidGeometricValidator::validateFaceIsPlanar(
                 newFace) ) {
            scan = newFace->boundariesList.get(0)->boundaryStartHalfEdge;
            newFaceId = solid->getMaxFaceId()+1;
            PolyhedralBoundedSolidEulerOperators::lmef(solid, scan->next(),
                scan->previous(), newFaceId);
        }
    }

    newFaces.clear();

    PolyhedralBoundedSolidValidationEngine::validateIntermediate(solid);
}
