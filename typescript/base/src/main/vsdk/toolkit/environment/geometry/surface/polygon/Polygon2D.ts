import { Surface } from "../Surface.js";
import { _Polygon2DContour } from "./_Polygon2DContour.js";
import { BinaryTreeNode } from "../../../../common/dataStructures/BinaryTreeNode.js";
import type { Ray } from "../../element/Ray.js";
import type { RayHit } from "../../element/RayHit.js";
import type { Vertex2D } from "../../element/Vertex2D.js";
import { Double } from "../../../../../../java/lang/Double.js";
import { Collections } from "../../../../../../java/util/Collections.js";

/**
Planar polygon made of contours. Java's `List<_Polygon2DContour> loops` (an
`ArrayList`) is an array here.
*/
export class Polygon2D extends Surface<Ray, RayHit> {
    public loops: _Polygon2DContour[];
    private currentLoop!: _Polygon2DContour;

    // headNode is the head node of the n-ary tree codified in a binary tree
    // in left child-right sibling way.
    private headNode: BinaryTreeNode<_Polygon2DContour | null> | null = null;

    public constructor() {
        super();
        this.loops = [];
        this.nextLoop();
    }

    public addVertex(x: number, y: number): void;
    public addVertex(x: number, y: number, r: number, g: number, b: number): void;
    public addVertex(x: number, y: number, r?: number, g?: number, b?: number): void {
        if (r === undefined) {
            this.currentLoop.addVertex(x, y);
            return;
        }
        this.currentLoop.addVertex(x, y, r, g!, b!);
    }

    public pushVertex(x: number, y: number): void {
        this.currentLoop.pushVertex(x, y);
    }

    public nextLoop(): void {
        this.currentLoop = new _Polygon2DContour();
        this.loops.push(this.currentLoop);
    }

    public eraseLastLoop(): void {
        let i: number;
        let loop: _Polygon2DContour;

        if (this.loops.length > 1) {
            // Remove also the holes of that loop. LRR.
            for (i = 0; i < this.loops.length; ++i) {
                loop = this.loops[i]!;
                if (loop.getExteriorContour() === this.loops[this.loops.length - 1]) {
                    this.loops.splice(i, 1);
                    --i;
                }
            }
            if (this.loops.length > 1) {
                this.loops.splice(this.loops.length - 1, 1);
                this.currentLoop = this.loops[this.loops.length - 1]!;
            }
            // Rebuild the n-ary tree codified in a binary tree.
            this.classifyContourHoles();
        }
    }

    public invert(): void {
        for (const p2DContour of this.loops) {
            Collections.reverse(p2DContour.vertices);
        }
    }

    /**
    @return a new 6 valued double array containing the coordinates of a min-max
    bounding box for current geometry.
    */
    public override getMinMax(): number[] {
        let v: Vertex2D;
        let i: number;
        let j: number;

        const minMax: number[] = [0, 0, 0, 0, 0, 0];
        let minX = Double.MAX_VALUE;
        let minY = Double.MAX_VALUE;
        let maxX = -Double.MAX_VALUE;
        let maxY = -Double.MAX_VALUE;

        for (i = 0; i < this.loops.length; i++) {
            for (j = 0; j < this.loops[i]!.vertices.length - 1; j++) {
                v = this.loops[i]!.vertices[j]!;
                if (v.x > maxX) {
                    maxX = v.x;
                }
                if (v.x < minX) {
                    minX = v.x;
                }
                if (v.y > maxY) {
                    maxY = v.y;
                }
                if (v.y < minY) {
                    minY = v.y;
                }
            }
        }
        minMax[0] = minX;
        minMax[1] = minY;
        minMax[2] = 0;
        minMax[3] = maxX;
        minMax[4] = maxY;
        minMax[5] = 0;
        return minMax;
    }

    /** Java overload `doIntersectionFirstHit(Ray)` returns null; the other false. */
    public doIntersectionFirstHit(inOutRay: Ray): Ray | null;
    public override doIntersectionFirstHit(inRay: Ray, outHit: RayHit): boolean;
    public override doIntersectionFirstHit(inRay: Ray, outHit: RayHit | null): boolean;
    public override doIntersectionFirstHit(_inRay: Ray, outHit?: RayHit | null): Ray | null | boolean {
        if (outHit === undefined) {
            return null;
        }
        return false;
    }

    public override doExtraInformation(_inRay: Ray, _intT: number, _outData: RayHit): void {}

    /**
     * @return the headNode
     */
    public getHeadNode(): BinaryTreeNode<_Polygon2DContour | null> | null {
        return this.headNode;
    }

    /**
     * @param headNode the headNode to set
     */
    public setHeadNode(headNode: BinaryTreeNode<_Polygon2DContour | null> | null): void {
        this.headNode = headNode;
    }

    // Contour classification, moved here from the removed PolygonProcessor as
    // cpp did (Polygon2D::classifyContourHoles / isPointInsideContour). The
    // Java bodies are the original algorithm (n-ary contour tree, EPSILON
    // boundary band) that the cpp version simplifies; they are kept as they were.
    private static readonly EPSILON = 1e-18;

    public classifyContourHoles(): void {
        const polygon: Polygon2D = this;
        let i: number;
        let j: number;
        let p2DContour: _Polygon2DContour;
        let p2DContourTest: _Polygon2DContour;

        // Clear temporary tree flags.
        for (i = 0; i < polygon.loops.length; ++i) {
            p2DContour = polygon.loops[i]!;
            // Indicates that the contour is in the binary tree.
            p2DContour.fleetingFlag = false;
        }

        // Head node contains no data.
        polygon.setHeadNode(new BinaryTreeNode<_Polygon2DContour | null>(null));

        const tempList: _Polygon2DContour[] = [];
        // Fill tempList and create the binary tree.
        for (i = 0; i < polygon.loops.length; ++i) {
            p2DContour = polygon.loops[i]!;
            tempList.length = 0;
            tempList.push(p2DContour);
            for (j = i + 1; j < polygon.loops.length; ++j) {
                p2DContourTest = polygon.loops[j]!;
                if (!p2DContourTest.fleetingFlag) {
                    // If p2DContourTest is not in the binary tree:
                    if (Polygon2D.isContourInsideContour(p2DContour.vertices, p2DContourTest.vertices)) {
                        tempList.push(p2DContourTest);
                    }
                }
            }
            // Sorts in descending order (Collections.reverseOrder()).
            Collections.sort(tempList, { compare: (a: _Polygon2DContour, b: _Polygon2DContour) => b.compareTo(a) });
            Polygon2D.insertListInBinaryTree(tempList, polygon.getHeadNode()!);
        }
    }

    private static insertListInBinaryTree(
        list: _Polygon2DContour[],
        headNode: BinaryTreeNode<_Polygon2DContour | null>, // Head node is in the 0 level.
    ): void {
        const levelOfContainingContourNode: number[] = [0];

        if (list.length === 0) return;

        const p2DContour = list[0]!;
        const containingContourNode = Polygon2D.findContainingContourNode(
            p2DContour,
            headNode,
            levelOfContainingContourNode,
        );
        Polygon2D.insertListInBinaryTreeNode(list, containingContourNode!, levelOfContainingContourNode[0]!);
    }

    private static insertListInBinaryTreeNode(
        list: _Polygon2DContour[],
        containingListNode: BinaryTreeNode<_Polygon2DContour | null>,
        levelOfContainingContourNode: number,
    ): void {
        let i: number;
        let child: BinaryTreeNode<_Polygon2DContour | null> | null;
        let lastChild: BinaryTreeNode<_Polygon2DContour | null> | null = null;
        let p2DContour: _Polygon2DContour;
        let levelOfNode: number;

        levelOfNode = levelOfContainingContourNode + 1;
        if (levelOfNode % 2 === 0) list[0]!.setExteriorContour(containingListNode.getData());

        // Find the last child of the node and add a new one.
        child = containingListNode.getChild();
        if (child === null) {
            lastChild = new BinaryTreeNode<_Polygon2DContour | null>(list[0]!);
            containingListNode.setChild(lastChild);
        } else {
            while (child !== null) {
                lastChild = child;
                child = child.getSibling();
            }
            child = new BinaryTreeNode<_Polygon2DContour | null>(list[0]!);
            if (lastChild !== null) {
                lastChild.setSibling(child);
            }
            lastChild = child;
        }
        // Indicates that the contour is in the binary tree.
        list[0]!.fleetingFlag = true;

        // Fill the binary tree with the rest of elements of list.
        for (i = 1; i < list.length; ++i) {
            ++levelOfNode;
            p2DContour = list[i]!;
            if (levelOfNode % 2 === 0) {
                // Is a hole.
                p2DContour.setExteriorContour(lastChild.getData());
            }
            child = new BinaryTreeNode<_Polygon2DContour | null>(p2DContour);
            lastChild.setChild(child);
            child.getData()!.fleetingFlag = true;
            lastChild = child;
        }
    }

    /**
     * Finds the contour node that contains the p2DContour in the binary tree
     * represented by headNode.
     * @param levelOfContainingContourNode is zero based. This array contains one element.
     */
    private static findContainingContourNode(
        p2DContour: _Polygon2DContour,
        headNode: BinaryTreeNode<_Polygon2DContour | null> | null,
        levelOfContainingContourNode: number[],
    ): BinaryTreeNode<_Polygon2DContour | null> | null {
        let isTheContainingContourNode: boolean;
        let child: BinaryTreeNode<_Polygon2DContour | null> | null;
        levelOfContainingContourNode[0] = 0;

        while (headNode !== null) {
            child = headNode.getChild();
            isTheContainingContourNode = true;
            while (child !== null) {
                if (Polygon2D.isContourInsideContour(p2DContour.vertices, child.getData()!.vertices)) {
                    isTheContainingContourNode = false;
                    headNode = child;
                    ++levelOfContainingContourNode[0]!;
                    break;
                }
                child = child.getSibling();
            }
            if (isTheContainingContourNode) return headNode;
        }
        // Reaches here only if headNode == null.
        return headNode;
    }

    /**
     * Test if the contour is inside the polygon. This function assumes that
     * the contour is fully inside or fully outside.
     */
    private static isContourInsideContour(contour: Vertex2D[], mainPolygon: Vertex2D[]): boolean {
        let i: number;
        let point: Vertex2D;
        let result: number;

        for (i = 0; i < contour.length; ++i) {
            point = contour[i]!;
            result = Polygon2D.isPointInsideContour(point, mainPolygon);
            if (result === 1) return true;
            if (result === -1) return false;
            // If result equals 0 the point is in the contour of the mainPolygon,
            // the search continues.
        }
        // If every point is in the contour of the mainPolygon. Note that in this
        // case the contour can be anywhere(inside, outside, intersecting).
        return false;
    }

    /* Function isPointInPolygon2D is based on the function developed by W. Randolph
     Franklin:
     http://www.ecse.rpi.edu/Homepages/wrf/Research/Short_Notes/pnpoly.html
     visited on sep 08 2014.
     The part that tests if the point is in de boundary is new.
     (License: see the Java original.)
     */

    /**
     * Test if point is inside, outside or on the boundary of the polygon.
     * @return 1 if the point is inside, -1 if the point is outside, 0 if the
     * point is on the boundary.
     */
    public static isPointInsideContour(point: Vertex2D, polygon: Vertex2D[]): number {
        let i: number;
        let isInside = false;
        let temp: number;
        let polPoint: Vertex2D;
        let polPointNext: Vertex2D;

        const polSize = polygon.length;
        if (polSize < 3) {
            return -1;
        }
        for (i = 0; i < polSize; ++i) {
            polPoint = polygon[i]!;
            if (i === polSize - 1) polPointNext = polygon[0]!;
            else polPointNext = polygon[i + 1]!;
            temp = polPoint.y - polPointNext.y;
            if (Math.abs(temp) < Polygon2D.EPSILON) {
                //Horizontal line.
                if (Math.abs(point.y - (polPointNext.y + temp / 2)) < Polygon2D.EPSILON) {
                    if ((polPointNext.x - point.x) * (point.x - polPoint.x) >= 0) {
                        return 0;
                    }
                }
            }
            if (point.y < polPoint.y !== point.y < polPointNext.y) {
                temp =
                    ((polPointNext.x - polPoint.x) * (point.y - polPoint.y)) / (polPointNext.y - polPoint.y) +
                    polPoint.x;
                if (Math.abs(point.x - temp) < Polygon2D.EPSILON) {
                    return 0;
                }
                if (point.x < temp) {
                    isInside = !isInside;
                }
            }
        }
        if (isInside) {
            return 1;
        } else {
            return -1;
        }
    }
}
