package vsdk.toolkit.galerkin;

import java.util.ArrayList;
import vsdk.toolkit.common.linealAlgebra.Vector3D;
import vsdk.toolkit.scene.Polygon;
import vsdk.toolkit.skin.BoundingBox;
import vsdk.toolkit.skin.Geometry;
import vsdk.toolkit.skin.Patch;

/**
References:

- [HAIN1991] Haines, E. A. and Wallace, J. R. "Shaft culling for efficient ray-traced radiosity",
  2nd Euro-graphics Workshop on Rendering, Barcelona, Spain, May 1991
*/

/**
The shaft is the region bounded by extent and referenceItem1 and referenceItem2 (if defined)
and on the negative side of the numberOfPlanesInSet

Note: the name "Shaft" can be misleading. From [HAIN1991] can be seen that this is more
a kind of convex envelope.
*/
public class Shaft {
    private static final int MAX_SKIP_ELEMENTS = 2;
    // Maximum 16 numberOfPlanesInSet in plane-set: maximum 8 for a box-to-box shaft (see figure [HAIN1991].2),
    // maximum 2 times the total number of vertices for a patch-to-patch shaft
    private static final int SHAFT_MAX_PLANES = 16;
    private static final int MIN_MAX_DIMENSIONS = 6;
    private static final int NONE = -1;

    private BoundingBox referenceItem1; // Bounding boxes of the reference items
    private BoundingBox referenceItem2;
    private BoundingBox extentBoundingBox;
    private ShaftPlane[] planeSet;
    private long numberOfPlanesInSet; // Number of planes in plane-set

    private int[] patchIdsToOmit; // Geometries to be ignored during shaft culling, maximum 2
    private int numberOfGeometriesToOmit;
    private int[] geometryIdsToAvoidOpening; // Geometries not to be opened during shaft culling, maximum 2
    private int numberOfGeometriesToAvoidOpen;

    private Vector3D center1; // The line segment from center1 to center2 is guaranteed
    // to lay within the shaft
    private Vector3D center2;
    private boolean cut; // A boolean initialized to FALSE when the shaft is created and set
    // to TRUE during shaft culling if there are patches that cut the shaft. If
    // after shaft culling, this flag is TRUE, there is full occlusion due to
    // one occluder.
    // As soon as such a situation is detected, shaft culling ends and the
    // occluder in question is the first patch in the returned candidate list.
    // The candidate list does not contain all occluder!

    public Shaft() {
        referenceItem1 = null;
        referenceItem2 = null;
        extentBoundingBox = new BoundingBox();
        planeSet = new ShaftPlane[SHAFT_MAX_PLANES];
        for ( int i = 0; i < planeSet.length; i++ ) {
            planeSet[i] = new ShaftPlane();
        }
        numberOfPlanesInSet = 0;
        patchIdsToOmit = new int[MAX_SKIP_ELEMENTS];
        geometryIdsToAvoidOpening = new int[MAX_SKIP_ELEMENTS];
        numberOfGeometriesToOmit = 0;
        numberOfGeometriesToAvoidOpen = 0;
        center1 = new Vector3D();
        center2 = new Vector3D();
        cut = false;
    }

    public static void freeCandidateList(ArrayList<Geometry> candidateList) {
        if ( candidateList != null ) {
            candidateList.clear();
        }
    }

    public boolean isCut() {
        return cut;
    }

    public void constructFromBoundingBoxes(BoundingBox boundingBox1, BoundingBox boundingBox2) {
        numberOfGeometriesToOmit = 0;
        numberOfGeometriesToAvoidOpen = 0;
        cut = false;
        referenceItem1 = boundingBox1;
        referenceItem2 = boundingBox2;
        if ( boundingBox1 == null || boundingBox2 == null ) {
            numberOfPlanesInSet = 0;
            return;
        }
        center1 = boundingBox1.center();
        center2 = boundingBox2.center();
        extentBoundingBox.setAsUnion(boundingBox1, boundingBox2);
        numberOfPlanesInSet = 0;
    }

    public void constructFromPolygonToPolygon(Polygon polygon1, Polygon polygon2) {
        numberOfGeometriesToOmit = 0;
        numberOfGeometriesToAvoidOpen = 0;
        cut = false;
        numberOfPlanesInSet = 0;
        constructPolygonToPolygonPlanes(polygon1, polygon2);
    }

    /**
Marks a geometry as to be omitted during shaft culling: it will not be added to the
candidate list, even if the geometry overlaps or is inside the shaft
*/
    public void setShaftOmit(Patch patch) {
        if ( patch == null || numberOfGeometriesToOmit >= MAX_SKIP_ELEMENTS ) {
            return;
        }
        patchIdsToOmit[numberOfGeometriesToOmit++] = patch.id;
    }

    /**
Marks a geometry as one not to be opened during shaft culling
*/
    public void setShaftDontOpen(Geometry geometry) {
        if ( geometry == null || numberOfGeometriesToAvoidOpen >= MAX_SKIP_ELEMENTS ) {
            return;
        }
        geometryIdsToAvoidOpening[numberOfGeometriesToAvoidOpen++] = geometry.id;
    }

    public void doCulling(
        ArrayList<Geometry> world,
        ArrayList<Geometry> candidateList,
        ShaftCullStrategy strategy)
    {
        if ( candidateList == null ) {
            return;
        }
        candidateList.clear();
        for ( int i = 0; world != null && i < world.size(); i++ ) {
            cullGeometry(world.get(i), candidateList, strategy);
            if ( cut ) {
                return;
            }
        }
    }

    public void cullGeometry(
        Geometry geometry,
        ArrayList<Geometry> candidateList,
        ShaftCullStrategy strategy)
    {
        if ( geometry == null || candidateList == null ) {
            return;
        }
        if ( geometry.omit ) {
            return;
        }
        keep(geometry, candidateList);
    }

    private static ShaftPlanePosition testPolygonWithRespectToPlane(Polygon polygon, Vector3D normal, double d) {
        return ShaftPlanePosition.OVERLAP;
    }

    private static void fillInPlane(ShaftPlane plane, float nx, float ny, float nz, float d) {
        if ( plane == null ) {
            return;
        }
        plane.n[0] = nx;
        plane.n[1] = ny;
        plane.n[2] = nz;
        plane.d = d;
    }

    private static boolean verifyPolygonWithRespectToPlane(
        Polygon polygon,
        Vector3D normal,
        double d,
        ShaftPlanePosition side)
    {
        return true;
    }

    private static ShaftPlanePosition testPointWithRespectToPlane(Vector3D p, Vector3D normal, double d) {
        return ShaftPlanePosition.OVERLAP;
    }

    private static int compareShaftPlanes(ShaftPlane plane1, ShaftPlane plane2) {
        if ( plane1 == null || plane2 == null ) {
            return NONE;
        }
        return 0;
    }

    private static void keep(Geometry geometry, ArrayList<Geometry> candidateList) {
        if ( geometry == null || candidateList == null ) {
            return;
        }
        candidateList.add(geometry);
    }

    private void constructPolygonToPolygonPlanes(Polygon p1, Polygon p2) {
    }

    private ShaftPlanePosition shaftPatchTest(Patch patch) {
        return ShaftPlanePosition.OVERLAP;
    }

    private boolean closedGeometry(Geometry geometry) {
        return true;
    }

    private int uniqueShaftPlane(ShaftPlane parameterPlane) {
        return 1;
    }

    private ShaftPlanePosition boundingBoxTest(BoundingBox parameterBoundingBox) {
        return ShaftPlanePosition.OVERLAP;
    }

    private ArrayList<Patch> cullPatches(ArrayList<Patch> patchList) {
        return patchList;
    }

    private boolean patchIsOnOmitSet(int id) {
        for ( int i = 0; i < numberOfGeometriesToOmit; i++ ) {
            if ( patchIdsToOmit[i] == id ) {
                return true;
            }
        }
        return false;
    }

    private void shaftCullOpen(Geometry geometry, ArrayList<Geometry> candidateList, ShaftCullStrategy strategy) {
        keep(geometry, candidateList);
    }
}
