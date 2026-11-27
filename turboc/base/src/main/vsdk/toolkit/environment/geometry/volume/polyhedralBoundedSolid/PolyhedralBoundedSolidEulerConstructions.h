#ifndef __PBS_ELR_CNSTRCTNS__
#define __PBS_ELR_CNSTRCTNS__

#include "vsdk/toolkit/common/linealAlgebra/Matrix4x4d.h"

class PolyhedralBoundedSolid;
class _PolyhedralBoundedSolidFace;

/**
Constructions of [MANT1988] chapter 12 composed only of Euler operators: arcs
and circular laminas (`mvfs`, `smev`, `smef`, section 12.2) and translational
sweeps of a face (`lmev`, `lmef`, section 12.3.1). Solids that model their own
boundary representation (`Cone`, `Arrow`) build on them.
*/
class PolyhedralBoundedSolidEulerConstructions {
public:
    /**
    [MANT1988] section 12.2, program 12.1: appends `n` vertices of an arc on
    plane `z = height`, centered at (`cx`, `cy`), from `phi1` to `phi2`
    (degrees, counterclockwise, 0 degrees on +X), starting at vertex
    `vertexId` of face `faceId`.
    */
    static void addArcToExistingFace(PolyhedralBoundedSolid* solid,
        int faceId, int vertexId, double cx, double cy, double radius,
        double height, double phi1, double phi2, int n);

    /**
    [MANT1988] section 12.2, program 12.2: a planar circular lamina of `n`
    vertices as a single face (`mvfs`, arc, `smef`).
    @return a new solid, owned by the caller
    */
    static PolyhedralBoundedSolid* createCircularLamina(
        double cx, double cy, double rad, double h, int n);

    /**
    Generalized translational sweep of a face, inspired by [MANT1988] 12.3.1.

    Instead of pure translation, transform `T` can include translation,
    rotation, and scale in the face-local context.

    PRE: `face` is closed and planar.
    */
    static void translationalSweepExtrudeFace(PolyhedralBoundedSolid* solid,
        _PolyhedralBoundedSolidFace* face,
        const Matrix4x4d& transformationMatrix);

    /**
    Variant of `translationalSweepExtrudeFace` with an extra planar check
    pass.

    After sweep generation, each created side face is tested for planarity.
    If a face is non-planar, it is split once (`lmef`) to triangulate it.
    */
    static void translationalSweepExtrudeFacePlanar(
        PolyhedralBoundedSolid* solid,
        _PolyhedralBoundedSolidFace* face,
        const Matrix4x4d& transformationMatrix);
};

#endif
