package vsdk.toolkit.scene;

import java.util.ArrayList;
import vsdk.toolkit.common.Error;
import vsdk.toolkit.common.linealAlgebra.Numeric;
import vsdk.toolkit.common.linealAlgebra.Ray;
import vsdk.toolkit.common.linealAlgebra.Vector3D;
import vsdk.toolkit.skin.BoundingBox;
import vsdk.toolkit.skin.Compound;
import vsdk.toolkit.skin.Geometry;
import vsdk.toolkit.skin.MinMaxBox;
import vsdk.toolkit.skin.Patch;
import vsdk.toolkit.skin.RayHit;

/**
Uniform voxel grid to optimize intersection operations

Ray-grid intersection: [SNYD1987] Snyder & Barr, SIGGRAPH '87, p123, with several
optimisations/enhancements from ray shade 4.0.6 by Graig Kolb, Stanford U
*/
public class VoxelGrid {
    private static final int MINIMUM_ELEMENT_COUNT_PER_CELL = 10;
    private static final float DELTA_BOUND_FACTOR = 1e-4f;

    private static ArrayList<VoxelGrid> subGridsToDelete;
    private static ArrayList<VoxelData> voxelCellsToDelete;
    private static int constructorLevel = 0;
    private static int randomRayCounter = 0;

    private short xSize;
    private short ySize;
    private short zSize;
    private Vector3D voxelSize;
    private ArrayList<VoxelData>[] volumeListsOfItems; // 3D array of item lists
    private Object gridItemPool;
    private BoundingBox boundingBox;
    private MinMaxBox rayIntersectionBox;

    private static void addToSubGridsDeletionCache(VoxelGrid voxelGrid) {
        if (subGridsToDelete == null) {
            subGridsToDelete = new ArrayList<>();
        }
        subGridsToDelete.add(voxelGrid);
    }

    private static void addToCellsDeletionCache(VoxelData cell) {
        if (voxelCellsToDelete == null) {
            voxelCellsToDelete = new ArrayList<>();
        }
        voxelCellsToDelete.add(cell);
    }

    private static short clampVoxel(short v, short max) {
        if (v < 0) {
            return 0;
        }
        if (v >= max) {
            return (short)(max - 1);
        }
        return v;
    }

    private static boolean shouldSubdivide(Geometry geometry) {
        return geometry.itemCount >= MINIMUM_ELEMENT_COUNT_PER_CELL;
    }

    private float voxel2x(float px) {
        return px * voxelSize.x + boundingBox.minX();
    }

    private float voxel2y(float py) {
        return py * voxelSize.y + boundingBox.minY();
    }

    private float voxel2z(float pz) {
        return pz * voxelSize.z + boundingBox.minZ();
    }

    private short x2voxel(float px) {
        return (short)((voxelSize.x < Numeric.EPSILON)
            ? 0
            : (px - boundingBox.minX()) / voxelSize.x);
    }

    private short y2voxel(float py) {
        return (short)((voxelSize.y < Numeric.EPSILON)
            ? 0
            : (py - boundingBox.minY()) / voxelSize.y);
    }

    private short z2voxel(float pz) {
        return (short)((voxelSize.z < Numeric.EPSILON)
            ? 0
            : (pz - boundingBox.minZ()) / voxelSize.z);
    }

    private int cellIndexAddress(int a, int b, int c) {
        return (a * ySize + b) * zSize + c;
    }

    private void putGeometryInsideVoxelGrid(Geometry geometry, short na, short nb, short nc) {
        if (na <= 0 || nb <= 0 || nc <= 0) {
            Error.error("VoxelGrid::putGeometryInsideVoxelGrid", "Invalid grid dimensions");
            System.exit(1);
        }

        // Enlarge the getBoundingBox by a small amount
        boundingBox.copyFrom(geometry.boundingBox);
        boundingBox.enlargeByFactor(DELTA_BOUND_FACTOR);
        if (rayIntersectionBox == null) {
            rayIntersectionBox = new MinMaxBox(boundingBox);
        }
        else {
            rayIntersectionBox.updateFromBoundingBox(boundingBox);
        }
        xSize = na;
        ySize = nb;
        zSize = nc;

        voxelSize = boundingBox.voxelSize(na, nb, nc);

        @SuppressWarnings("unchecked")
        ArrayList<VoxelData>[] cells = (ArrayList<VoxelData>[])new ArrayList[na * nb * nc];
        volumeListsOfItems = cells;
        gridItemPool = null;
        putSubGeometryInsideVoxelGrid(geometry);
    }

    private boolean isSmall(BoundingBox bb) {
        return bb.dx() <= voxelSize.x &&
            bb.dy() <= voxelSize.y &&
            bb.dz() <= voxelSize.z;
    }

    private void putSubGeometryInsideVoxelGrid(Geometry geometry) {
        if (isSmall(geometry.boundingBox)) {
            if (shouldSubdivide(geometry)) {
                insertSubGrid(geometry);
            }
            else {
                insertGeometryAsVoxelData(geometry);
            }
            return;
        }

        if (geometry.isCompound()) {
            processCompoundGeometry(geometry);
            return;
        }
        processPatches(geometry);
    }

    private void putItemInsideVoxelGrid(VoxelData item, BoundingBox itemBounds) {
        BoundingBox boundaries = new BoundingBox();
        boundaries.copyFrom(itemBounds);
        boundaries.enlargeByFactor(DELTA_BOUND_FACTOR);

        Vector3D minVoxel = toVoxelClamped(boundaries.minPoint());
        Vector3D maxVoxel = toVoxelClamped(boundaries.maxPoint());

        short minA = (short)minVoxel.x;
        short minB = (short)minVoxel.y;
        short minC = (short)minVoxel.z;

        short maxA = (short)maxVoxel.x;
        short maxB = (short)maxVoxel.y;
        short maxC = (short)maxVoxel.z;

        // Insert the current item in to all voxels that intersects with bounding box
        for (short a = minA; a <= maxA; a++) {
            for (short b = minB; b <= maxB; b++) {
                for (short c = minC; c <= maxC; c++) {
                    int index = cellIndexAddress(a, b, c);
                    ArrayList<VoxelData> voxelList = volumeListsOfItems[index];

                    if (voxelList == null) {
                        voxelList = new ArrayList<>(1);
                        volumeListsOfItems[index] = voxelList;
                    }

                    if (item != null) {
                        voxelList.add(item);
                    }
                }
            }
        }
    }

    private void putPatchInsideVoxelGrid(Patch patch) {
        BoundingBox localBounds = new BoundingBox();
        if (patch.boundingBox != null) {
            localBounds.copyFrom(patch.boundingBox);
        }
        else {
            patch.computeAndGetBoundingBox(localBounds);
        }

        VoxelData voxelData = new VoxelData(patch, VoxelDataFlags.VOXEL_DATA_PATCH_MASK);
        putItemInsideVoxelGrid(voxelData, localBounds);
        addToCellsDeletionCache(voxelData);
    }

    private Vector3D toVoxelClamped(Vector3D p) {
        return new Vector3D(
            clampVoxel(x2voxel(p.x), xSize),
            clampVoxel(y2voxel(p.y), ySize),
            clampVoxel(z2voxel(p.z), zSize));
    }

    private void insertGeometryAsVoxelData(Geometry geometry) {
        VoxelData voxelData = new VoxelData(geometry, VoxelDataFlags.VOXEL_DATA_GEOMETRY_MASK);
        putItemInsideVoxelGrid(voxelData, geometry.boundingBox);
        addToCellsDeletionCache(voxelData);
    }

    private void processCompoundGeometry(Geometry geometry) {
        ArrayList<Geometry> geometryList = ((Compound)geometry).children;

        for (int i = 0; geometryList != null && i < geometryList.size(); i++) {
            putSubGeometryInsideVoxelGrid(geometryList.get(i));
        }
    }

    private void processPatches(Geometry geometry) {
        ArrayList<Patch> patches = Geometry.patchListReference(geometry);

        for (int i = 0; patches != null && i < patches.size(); i++) {
            putPatchInsideVoxelGrid(patches.get(i));
        }
    }

    private void insertSubGrid(Geometry geometry) {
        VoxelGrid subGrid = new VoxelGrid(geometry);
        VoxelData voxelData = new VoxelData(subGrid, VoxelDataFlags.VOXEL_DATA_GRID_MASK);

        putItemInsideVoxelGrid(voxelData, subGrid.boundingBox);

        addToSubGridsDeletionCache(subGrid);
        addToCellsDeletionCache(voxelData);
    }

    /**
    Compute t0, ray's minimal intersection with the whole grid and
    position P of this intersection. Returns true if the grid getBoundingBox are
    intersected and false if the ray passes along the voxel grid
    */
    private boolean gridBoundsIntersect(
        Ray ray,
        float minimumDistance,
        float maximumDistance,
        float[] t0,
        Vector3D position) {
        t0[0] = minimumDistance;
        position.sumScaled(ray.position, t0[0], ray.direction);
        if (boundingBox.outOfBounds(position)) {
            t0[0] = maximumDistance;
            if (rayIntersectionBox == null) {
                rayIntersectionBox = new MinMaxBox(boundingBox);
            }
            if (!rayIntersectionBox.intersect(ray, minimumDistance, t0)) {
                return false;
            }
            position.sumScaled(ray.position, t0[0], ray.direction);
        }

        return true;
    }

    /**
    Initializes grid tracing
    */
    private void gridTraceSetup(
        Ray ray,
        float t0,
        Vector3D p,
        int[] g,
        Vector3D tDelta,
        Vector3D tNext,
        int[] step,
        int[] out) {
        // Compute the grid cell g where this intersection occurs
        g[0] = x2voxel(p.x);
        if (g[0] >= xSize) {
            g[0] = xSize - 1;
        }
        g[1] = y2voxel(p.y);
        if (g[1] >= ySize) {
            g[1] = ySize - 1;
        }
        g[2] = z2voxel(p.z);
        if (g[2] >= zSize) {
            g[2] = zSize - 1;
        }

        /*
        Setup X:
        tDelta->x is the distance increment along the ray to the adjacent
        voxel in X direction.
        tNext->x is the total distance from the ray origin to the next voxel
        in X direction.
        step[0] is either +1 or -1 according to the ray X direction.
        out[0] is -1 or xSize: the first x grid cell index outside the
        grid.
        */
        if (Math.abs(ray.direction.x) > Numeric.EPSILON) {
            if (ray.direction.x > 0.0) {
                tDelta.x = voxelSize.x / ray.direction.x;
                tNext.x = t0 + (voxel2x((float)g[0] + 1) - p.x) / ray.direction.x;
                step[0] = 1;
                out[0] = xSize;
            }
            else {
                tDelta.x = voxelSize.x / -ray.direction.x;
                tNext.x = t0 + (voxel2x((float)g[0]) - p.x) / ray.direction.x;
                step[0] = out[0] = -1;
            }
        }
        else {
            tDelta.x = 0.0f;
            tNext.x = Numeric.HUGE_FLOAT_VALUE;
        }

        // Setup Y:
        if (Math.abs(ray.direction.y) > Numeric.EPSILON) {
            if (ray.direction.y > 0.0) {
                tDelta.y = voxelSize.y / ray.direction.y;
                tNext.y = t0 + (voxel2y((float)g[1] + 1) - p.y) / ray.direction.y;
                step[1] = 1;
                out[1] = ySize;
            }
            else {
                tDelta.y = voxelSize.y / -ray.direction.y;
                tNext.y = t0 + (voxel2y((float)g[1]) - p.y) / ray.direction.y;
                step[1] = out[1] = -1;
            }
        }
        else {
            tDelta.y = 0.0f;
            tNext.y = Numeric.HUGE_FLOAT_VALUE;
        }

        // Setup Z:
        if (Math.abs(ray.direction.z) > Numeric.EPSILON) {
            if (ray.direction.z > 0.0) {
                tDelta.z = voxelSize.z / ray.direction.z;
                tNext.z = t0 + (voxel2z((float)g[2] + 1) - p.z) / ray.direction.z;
                step[2] = 1;
                out[2] = zSize;
            }
            else {
                tDelta.z = voxelSize.z / -ray.direction.z;
                tNext.z = t0 + (voxel2z((float)g[2]) - p.z) / ray.direction.z;
                step[2] = out[2] = -1;
            }
        }
        else {
            tDelta.z = 0.0f;
            tNext.z = Numeric.HUGE_FLOAT_VALUE;
        }
    }

    /**
    Advances to the next grid cell. Pre-condition: gridTraceSetup was called.
    Returns false if the current voxel was the last voxel in the grid intersected by the ray
    */
    private static boolean nextVoxel(float[] t0, int[] g, Vector3D tNext, Vector3D tDelta, int[] step, int[] out) {
        int inGrid;

        if (tNext.x <= tNext.y && tNext.x <= tNext.z) {
            // tNext->x is smallest
            g[0] += step[0];
            t0[0] = tNext.x;
            tNext.x += tDelta.x;
            inGrid = g[0] - out[0]; // false if g[0]==out[0]
        }
        else if (tNext.y <= tNext.z) {
            // tNext->y is smallest
            g[1] += step[1];
            t0[0] = tNext.y;
            tNext.y += tDelta.y;
            inGrid = g[1] - out[1];
        }
        else {
            // tNext->z is smallest
            g[2] += step[2];
            t0[0] = tNext.z;
            tNext.z += tDelta.z;
            inGrid = g[2] - out[2];
        }
        return inGrid != 0;
    }

    /**
    Finds the nearest intersection of the ray with an item (Geometry or Patch) in
    a voxel's item list. If there is an intersection, maximumDistance will contain
    the distance to the intersection point measured from the ray origin
    as usual. If there is no intersection, maximumDistance remains unmodified
    */
    private static RayHit voxelIntersect(
        ArrayList<VoxelData> items,
        Ray ray,
        int counter,
        float minimumDistance,
        float[] maximumDistance,
        int hitFlags,
        RayHit hitStore) {
        RayHit hit = null;

        for (int i = 0; items != null && i < items.size(); i++) {
            VoxelData item = items.get(i);
            if (item.lastRayId() != counter) {
                // Avoid testing objects multiple times
                RayHit h = null;
                if (item.isPatch()) {
                    h = item.patch.intersect(ray, minimumDistance, maximumDistance, hitFlags, hitStore);
                }
                else if (item.isGeom()) {
                    h = item.geometry.discretizationIntersect(ray, minimumDistance, maximumDistance, hitFlags, hitStore);
                }
                else if (item.isGrid()) {
                    h = item.voxelGrid.gridIntersect(ray, minimumDistance, maximumDistance, hitFlags, hitStore);
                }
                if (h != null) {
                    hit = h;
                }

                item.updateRayId(counter);
            }
        }

        return hit;
    }

    private static int randomRayId() {
        randomRayCounter++;
        return (randomRayCounter & VoxelDataFlags.VOXEL_DATA_RAY_COUNT_MASK);
    }

    /**
    Traces a ray through a voxel grid. Returns nearest intersection or nullptr
    */
    public RayHit gridIntersect(
        Ray ray,
        float minimumDistance,
        float[] maximumDistance,
        int hitFlags,
        RayHit hitStore) {
        Vector3D tNext = new Vector3D();
        Vector3D tDelta = new Vector3D();
        Vector3D p = new Vector3D();
        int[] step = new int[] {0, 0, 0};
        int[] out = new int[3];
        int[] g = new int[] {0, 0, 0};
        RayHit hit = null;
        float[] t0 = new float[1];

        if (!gridBoundsIntersect(ray, minimumDistance, maximumDistance[0], t0, p)) {
            return null;
        }

        gridTraceSetup(ray, t0[0], p, g, tDelta, tNext, step, out);

        // Ray counter in order to avoid testing objects spanning several voxel grid cells multiple times
        final int counter = randomRayId();

        do {
            ArrayList<VoxelData> list = volumeListsOfItems[cellIndexAddress(g[0], g[1], g[2])];
            if (list != null) {
                RayHit h = voxelIntersect(list, ray, counter, t0[0], maximumDistance, hitFlags, hitStore);
                if (h != null) {
                    hit = h;
                }
            }
        } while (nextVoxel(t0, g, tNext, tDelta, step, out) && t0[0] <= maximumDistance[0]);

        return hit;
    }

    /**
    Constructs a recursive grid structure containing the whole geometry
    */
    public VoxelGrid(Geometry geometry) {
        boundingBox = new BoundingBox();
        rayIntersectionBox = null;
        xSize = 0;
        ySize = 0;
        zSize = 0;
        voxelSize = new Vector3D();
        volumeListsOfItems = null;
        gridItemPool = null;

        final double p = Math.pow(geometry.itemCount, 0.33333) + 1;
        final short gridSize = (short)Math.floor(p);
        System.err.printf("Setting %d volumeListsOfItems in %d^3 cells level %d voxel grid ... \n", geometry.itemCount, gridSize, constructorLevel);
        constructorLevel++;

        putGeometryInsideVoxelGrid(geometry, gridSize, gridSize, gridSize);

        constructorLevel--;
    }

    public static void freeVoxelGridElements() {
        if (voxelCellsToDelete != null) {
            voxelCellsToDelete.clear();
            voxelCellsToDelete = null;
        }

        if (subGridsToDelete != null) {
            for (int i = 0; i < subGridsToDelete.size(); i++) {
                VoxelGrid subGrid = subGridsToDelete.get(i);
                subGrid.gridItemPool = null;
                subGrid.volumeListsOfItems = null;
            }
            subGridsToDelete.clear();
            subGridsToDelete = null;
        }
    }

    public void print() {
        System.out.printf("DX: %d, DY: %d, DZ: %d\n", (int)xSize, (int)ySize, (int)zSize);

        for (short z = 0; z < zSize; z++) {
            System.out.printf("Z level %d of %d\n", (int)z + 1, (int)zSize);

            for (short y = 0; y < ySize; y++) {
                System.out.printf("  | ");
                for (short x = 0; x < xSize; x++) {
                    ArrayList<VoxelData> list = volumeListsOfItems[cellIndexAddress(z, y, x)];
                    if (list == null) {
                        System.out.printf("[  ]");
                    }
                    else {
                        System.out.printf("(%2d)", list.size());
                    }
                    System.out.printf(" ");
                }
                System.out.printf(" |\n");
            }
        }
    }
}
