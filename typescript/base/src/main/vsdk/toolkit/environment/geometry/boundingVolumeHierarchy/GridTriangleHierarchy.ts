//= References:                                                             =
//= [SNYD1987] Snyder, John M. and Barr, Alan H. "Ray Tracing Complex       =
//=     Models Containing Surface Tessellations". SIGGRAPH '87, Computer    =
//=     Graphics 21(4), 1987, pp. 119-128.                                  =
//=     PDF: doc/references/[SNYD1987]_*.pdf                                =

import { ArrayList } from "../../../../../java/util/ArrayList.js";
import { RaytraceStatistics } from "../../../common/statistics/RaytraceStatistics.js";
import type { Ray } from "../element/Ray.js";
import { GridSpatialIndex } from "../spatialIndex/GridSpatialIndex.js";
import type { PrimitiveRayTest } from "../spatialIndex/PrimitiveRayTest.js";
import { SpatialGroupTree } from "../spatialIndex/SpatialGroupTree.js";
import { SpatialIndexWorkspace } from "../spatialIndex/SpatialIndexWorkspace.js";
import { SpatialQueryMode } from "../spatialIndex/SpatialQueryMode.js";
import { TriangleAccelerationStructure } from "./TriangleAccelerationStructure.js";
import { TriangleMeshAccelerationMethod } from "./TriangleMeshAccelerationMethod.js";

/**
The exact test of the grid query: `Triangle.intersectRay` on one triangle,
accepted when strictly closer than the hit so far, or at the same parameter
with a lower triangle index (the brute force tie rule). Keeps the face
coordinates of the accepted hit.
*/
export class GridTriangleRayTest implements PrimitiveRayTest {
    private readonly ray: Ray;
    private readonly positions: ArrayLike<number>;
    private readonly indexes: ArrayLike<number>;
    private readonly rayParameter: number[] = [0];
    private readonly u: number[] = [0];
    private readonly v: number[] = [0];

    public best: number;
    public bestU: number;
    public bestV: number;

    public constructor(inRay: Ray, inPositions: ArrayLike<number>, inIndexes: ArrayLike<number>) {
        this.ray = inRay;
        this.positions = inPositions;
        this.indexes = inIndexes;
        this.best = -1;
        this.bestU = 0;
        this.bestV = 0;
    }

    public test(primitive: number, minimumT: number, maximumT: number, hitT: number[]): boolean {
        if (
            !TriangleAccelerationStructure.testTriangle(
                this.ray,
                this.positions,
                this.indexes,
                primitive,
                this.rayParameter,
                this.u,
                this.v,
            )
        ) {
            return false;
        }
        const t = this.rayParameter[0]!;
        if (t < minimumT || t > maximumT) {
            return false;
        }
        if (t === maximumT && !(this.best >= 0 && primitive < this.best)) {
            return false;
        }
        this.best = primitive;
        this.bestU = this.u[0]!;
        this.bestV = this.v[0]!;
        hitT[0] = t;
        return true;
    }
}

/**
`TriangleMeshAccelerationMethod.VOXEL_GRID` for the triangles of one mesh:
a `GridSpatialIndex` ([SNYD1987], RenderPark's voxel grid) over the padded
triangle boxes, all of them members of one group, so the mesh gets one
uniform grid of floor(n^(1/3) + 1) cells along each axis.

Exactness: the leaf test is `Triangle.intersectRay` over the mesh arrays,
as the brute force loop; a hit found in an earlier cell may lie beyond it
([SNYD1987] Figure 11),
but the walk goes on until it enters a cell beyond the closest hit, and the
cell holding the closest hit point lists every triangle through that point,
so the closest hit and its ties are always tested. Ties go to the lowest
triangle index: the result is the brute force one.

The query uses a workspace without mailbox, so `firstHit` allocates nothing
and is safe from several threads; a triangle listed in several cells the
ray crosses is tested again in each (the repeated test gives the same hit,
which the tie rule does not take twice).
*/
export class GridTriangleHierarchy extends TriangleAccelerationStructure {
    /** Padded triangle boxes, six values per triangle, kept for `copy` */
    private readonly triangleBoxes: ArrayList<number>;
    private index: GridSpatialIndex | null;

    public constructor();
    /** Copy (cpp copy constructor): the index is built again */
    public constructor(other: GridTriangleHierarchy);
    public constructor(other?: GridTriangleHierarchy) {
        if (other === undefined) {
            super();
            this.triangleBoxes = new ArrayList<number>();
            this.index = null;
            return;
        }
        super(other);
        this.triangleBoxes = new ArrayList<number>(other.triangleBoxes);
        this.index = null;
        this.buildIndex();
    }

    public getMethod(): TriangleMeshAccelerationMethod {
        return TriangleMeshAccelerationMethod.VOXEL_GRID;
    }

    private buildIndex(): void {
        this.index = null;
        const triangleCount = Math.trunc(this.triangleBoxes.size() / 6);
        if (triangleCount === 0) {
            return;
        }
        const rootBox: number[] = [1e308, 1e308, 1e308, -1e308, -1e308, -1e308];
        for (let i = 0; i < triangleCount; i++) {
            for (let a = 0; a < 3; a++) {
                const lo = this.triangleBoxes.get(6 * i + a);
                const hi = this.triangleBoxes.get(6 * i + 3 + a);
                rootBox[a] = lo < rootBox[a]! ? lo : rootBox[a]!;
                rootBox[a + 3] = hi > rootBox[a + 3]! ? hi : rootBox[a + 3]!;
            }
        }
        const tree = new SpatialGroupTree(this.triangleBoxes, rootBox);
        for (let i = 0; i < triangleCount; i++) {
            tree.addPrimitive(0, i);
        }
        this.index = new GridSpatialIndex(tree);
    }

    public build(positions: ArrayLike<number>, indexes: ArrayLike<number>): void {
        this.recordBuiltMesh(positions, indexes);
        const triangleCount = Math.trunc(indexes.length / 3);
        this.triangleBoxes.clear();
        const lo: number[] = [0, 0, 0];
        const hi: number[] = [0, 0, 0];
        for (let i = 0; i < triangleCount; i++) {
            TriangleAccelerationStructure.triangleBox(positions, indexes, i, lo, hi);
            for (let a = 0; a < 3; a++) {
                this.triangleBoxes.add(lo[a]!);
            }
            for (let a = 0; a < 3; a++) {
                this.triangleBoxes.add(hi[a]!);
            }
        }
        this.buildIndex();
    }

    public firstHit(
        ray: Ray,
        positions: ArrayLike<number>,
        indexes: ArrayLike<number>,
        outT: number[],
        outU: number[],
        outV: number[],
        outTriangle: number[],
    ): boolean {
        if (this.builtTriangleCount <= 0 || this.index === null) {
            return false;
        }
        const test = new GridTriangleRayTest(ray, positions, indexes);
        // No mailbox: nothing to allocate per query (see the class comment).
        const workspace = new SpatialIndexWorkspace(0);
        // Same initial bound as the brute force loop; Triangle.intersectRay
        // accepts only parameters above VSDK.EPSILON, so 0 loses no hit.
        const result = this.index.queryWith(ray, 0.0, 1e308, SpatialQueryMode.CLOSEST_HIT, test, workspace);
        RaytraceStatistics.recordPrimitiveLevelTraversal(workspace.getNodeVisits(), workspace.getPrimitiveTests());
        if (!result.hasHit()) {
            return false;
        }
        outT[0] = result.getParameter();
        outU[0] = test.bestU;
        outV[0] = test.bestV;
        outTriangle[0] = result.getPrimitive();
        return true;
    }

    public copy(): TriangleAccelerationStructure {
        return new GridTriangleHierarchy(this);
    }

    /** Number of grids (one for a mesh) */
    public getNodeCount(): number {
        return this.index === null ? 0 : this.index.getNumberOfGrids();
    }

    public getDepth(): number {
        return this.index === null ? 0 : 1;
    }

    /** Cells along each axis of the grid; 0 when empty */
    public getResolution(): number {
        return this.index === null ? 0 : this.index.getResolution(0);
    }
}

TriangleAccelerationStructure.installGridTriangleHierarchyFactory(() => new GridTriangleHierarchy());
