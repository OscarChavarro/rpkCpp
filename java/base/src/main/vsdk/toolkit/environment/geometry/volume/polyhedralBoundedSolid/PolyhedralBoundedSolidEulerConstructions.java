package vsdk.toolkit.environment.geometry.volume.polyhedralBoundedSolid;

import java.util.ArrayList;

import vsdk.toolkit.common.linealAlgebra.Matrix4x4d;
import vsdk.toolkit.common.linealAlgebra.Vector3Dd;
import vsdk.toolkit.environment.geometry.volume.polyhedralBoundedSolid.nodes._PolyhedralBoundedSolidFace;
import vsdk.toolkit.environment.geometry.volume.polyhedralBoundedSolid.nodes._PolyhedralBoundedSolidHalfEdge;
import vsdk.toolkit.environment.geometry.volume.polyhedralBoundedSolid.nodes._PolyhedralBoundedSolidLoop;
import vsdk.toolkit.environment.geometry.volume.polyhedralBoundedSolid.nodes._PolyhedralBoundedSolidVertex;

/**
Constructions of [MANT1988] chapter 12 composed only of Euler operators: arcs
and circular laminas (`mvfs`, `smev`, `smef`, section 12.2) and translational
sweeps of a face (`lmev`, `lmef`, section 12.3.1). Solids that model their own
boundary representation (`Cone`, `Arrow`) build on them.
*/
public class PolyhedralBoundedSolidEulerConstructions
{
    /**
    [MANT1988] section 12.2, program 12.1: appends `n` vertices of an arc on
    plane `z = height`, centered at (`cx`, `cy`), from `phi1` to `phi2`
    (degrees, counterclockwise, 0 degrees on +X), starting at vertex
    `vertexId` of face `faceId`.
    */
    public static void addArcToExistingFace(
        PolyhedralBoundedSolid solid,
        int faceId,
        int vertexId,
        double cx,
        double cy,
        double radius,
        double height,
        double phi1,
        double phi2,
        int n)
    {
        double x;
        double y;
        double angle;
        double inc;
        int prev;
        int i;
        int nextVertexId;

        angle = Math.toRadians(phi1);
        inc = Math.toRadians((phi2 - phi1) / (n));
        prev = vertexId;
        for ( i = 0; i < n; i++ ) {
            angle += inc;
            // Snap trig values to 1e-10 grid so that equal angular positions on
            // two separately constructed circles always produce bit-identical
            // coordinates, preventing near-coincident vertices downstream.
            x = Math.round((cx + radius * Math.cos(angle)) * 1.0e10) / 1.0e10;
            y = Math.round((cy + radius * Math.sin(angle)) * 1.0e10) / 1.0e10;
            nextVertexId = solid.getMaxVertexId() + 1;
            PolyhedralBoundedSolidEulerOperators.smev(solid, faceId, prev, nextVertexId, new Vector3Dd(x, y, height));
            prev = nextVertexId;
        }
        PolyhedralBoundedSolidValidationEngine.validateIntermediate(solid);
    }

    /**
    [MANT1988] section 12.2, program 12.2: a planar circular lamina of `n`
    vertices as a single face (`mvfs`, arc, `smef`).
    @return a new solid, owned by the caller
    */
    public static PolyhedralBoundedSolid createCircularLamina(
        double cx, double cy, double rad, double h, int n)
    {
        PolyhedralBoundedSolid solid;

        solid = new PolyhedralBoundedSolid();
        PolyhedralBoundedSolidEulerOperators.mvfs(solid, new Vector3Dd(cx + rad, cy, h), 1, 1);
        addArcToExistingFace(solid, 1, 1, cx, cy, rad, h, 0,
            (n - 1) * 360.0 / n, n-1);
        PolyhedralBoundedSolidEulerOperators.smef(solid, 1, n, 1, 2);
        PolyhedralBoundedSolidValidationEngine.validateIntermediate(solid);
        return solid;
    }

    /**
    Generalized translational sweep of a face, inspired by [MANT1988] 12.3.1.

    Instead of pure translation, transform `T` can include translation,
    rotation, and scale in the face-local context.

    PRE: `face` is closed and planar.
    */
    public static void translationalSweepExtrudeFace(
        PolyhedralBoundedSolid solid,
        _PolyhedralBoundedSolidFace face,
        Matrix4x4d transformationMatrix)
    {
        _PolyhedralBoundedSolidLoop l;
        _PolyhedralBoundedSolidHalfEdge first;
        _PolyhedralBoundedSolidHalfEdge scan;
        _PolyhedralBoundedSolidVertex v;
        Vector3Dd newPos;
        int i;

        for ( i = 0; i < face.boundariesList.size(); i++ ) {
            l = face.boundariesList.get(i);
            first = l.boundaryStartHalfEdge;
            scan = first.next();
            v = scan.startingVertex;
            newPos = transformationMatrix.multiply(v.position);
            PolyhedralBoundedSolidEulerOperators.lmev(solid, scan, scan, solid.getMaxVertexId()+1, newPos);
            while ( scan != first ) {
                v = scan.next().startingVertex;
                newPos = transformationMatrix.multiply(v.position);
                PolyhedralBoundedSolidEulerOperators.lmev(solid, scan.next(), scan.next(),
                    solid.getMaxVertexId()+1, newPos);
                PolyhedralBoundedSolidEulerOperators.lmef(solid, scan.previous(), scan.next().next(),
                    solid.getMaxFaceId()+1);
                scan = (scan.next().mirrorHalfEdge()).next();
            }
            PolyhedralBoundedSolidEulerOperators.lmef(solid, scan.previous(), scan.next().next(),
                solid.getMaxFaceId()+1);
        }
        PolyhedralBoundedSolidValidationEngine.validateIntermediate(solid);
    }

    /**
    Variant of `translationalSweepExtrudeFace` with an extra planar check pass.

    After sweep generation, each created side face is tested for planarity.
    If a face is non-planar, it is split once (`lmef`) to triangulate it.
    */
    public static void translationalSweepExtrudeFacePlanar(
        PolyhedralBoundedSolid solid,
        _PolyhedralBoundedSolidFace face,
        Matrix4x4d transformationMatrix)
    {
        _PolyhedralBoundedSolidLoop l;
        _PolyhedralBoundedSolidHalfEdge first;
        _PolyhedralBoundedSolidHalfEdge scan;
        _PolyhedralBoundedSolidVertex v;
        Vector3Dd newPos;
        ArrayList<Integer> newFaces = new ArrayList<>();
        int i;
        int newFaceId;

        for ( i = 0; i < face.boundariesList.size(); i++ ) {
            l = face.boundariesList.get(i);
            first = l.boundaryStartHalfEdge;
            scan = first.next();
            v = scan.startingVertex;
            newPos = transformationMatrix.multiply(v.position);
            PolyhedralBoundedSolidEulerOperators.lmev(solid, scan, scan, solid.getMaxVertexId()+1, newPos);
            while ( scan != first ) {
                v = scan.next().startingVertex;
                newPos = transformationMatrix.multiply(v.position);
                PolyhedralBoundedSolidEulerOperators.lmev(solid, scan.next(), scan.next(),
                    solid.getMaxVertexId()+1, newPos);
                newFaceId = solid.getMaxFaceId()+1;
                PolyhedralBoundedSolidEulerOperators.lmef(solid, scan.previous(), scan.next().next(), newFaceId);
                newFaces.add(newFaceId);
                scan = (scan.next().mirrorHalfEdge()).next();
            }
            newFaceId = solid.getMaxFaceId()+1;
            PolyhedralBoundedSolidEulerOperators.lmef(solid, scan.previous(), scan.next().next(), newFaceId);
            newFaces.add(newFaceId);
        }

        _PolyhedralBoundedSolidFace newFace;
        for ( i = 0; i < newFaces.size(); i++ ) {
            newFaceId = newFaces.get(i);
            newFace = solid.findFace(newFaceId);
            if ( !PolyhedralBoundedSolidGeometricValidator.validateFaceIsPlanar(newFace) ) {
                scan = newFace.boundariesList.get(0).boundaryStartHalfEdge;
                newFaceId = solid.getMaxFaceId()+1;
                PolyhedralBoundedSolidEulerOperators.lmef(solid, scan.next(), scan.previous(), newFaceId);
            }
        }

        while ( !newFaces.isEmpty() ) {
            newFaces.remove(0);
        }

        PolyhedralBoundedSolidValidationEngine.validateIntermediate(solid);
    }

}
