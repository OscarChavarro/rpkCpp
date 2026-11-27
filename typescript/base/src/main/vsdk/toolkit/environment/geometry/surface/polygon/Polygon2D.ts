import { Surface } from "../Surface.js";
import { _Polygon2DContour } from "./_Polygon2DContour.js";
import { BinaryTreeNode } from "../../../../common/dataStructures/BinaryTreeNode.js";
import { Ray } from "../../element/Ray.js";
import { RayHit } from "../../element/RayHit.js";
import type { Vertex2D } from "../../element/Vertex2D.js";
export class Polygon2D extends Surface<Ray, RayHit> {
    public loops: _Polygon2DContour[] = [];
    private currentLoop: _Polygon2DContour;
    private headNode: BinaryTreeNode<_Polygon2DContour> | null = null;
    public constructor() {
        super();
        this.currentLoop = new _Polygon2DContour();
        this.loops.push(this.currentLoop);
    }
    public addVertex(x: number, y: number, r?: number, g?: number, b?: number) {
        this.currentLoop.addVertex(x, y, r, g, b);
    }
    public pushVertex(x: number, y: number) {
        this.currentLoop.pushVertex(x, y);
    }
    public nextLoop() {
        this.currentLoop = new _Polygon2DContour();
        this.loops.push(this.currentLoop);
    }
    public eraseLastLoop() {
        if (this.loops.length <= 1) return;
        const last = this.loops[this.loops.length - 1]!;
        this.loops = this.loops.filter((x) => x !== last && x.getExteriorContour() !== last);
        if (this.loops.length === 0) this.nextLoop();
        else this.currentLoop = this.loops[this.loops.length - 1]!;
    }
    public invert() {
        for (const loop of this.loops) loop.vertices.reverse();
    }
    public getMinMax() {
        const m = new Float64Array([Infinity, Infinity, 0, -Infinity, -Infinity, 0]);
        for (const loop of this.loops)
            for (const v of loop.vertices.slice(0, -1)) {
                m[0] = Math.min(m[0]!, v.x);
                m[1] = Math.min(m[1]!, v.y);
                m[3] = Math.max(m[3]!, v.x);
                m[4] = Math.max(m[4]!, v.y);
            }
        return m;
    }
    public doIntersectionFirstHit(_ray: Ray, _out: RayHit) {
        return false;
    }
    public override doExtraInformation(_ray: Ray, _t: number, _out: RayHit): void {}
    public getHeadNode() {
        return this.headNode;
    }
    public setHeadNode(x: BinaryTreeNode<_Polygon2DContour> | null) {
        this.headNode = x;
    }

    // Moved from the former PolygonProcessor, as the C++ port did
    // (Polygon2D::classifyContourHoles / isPointInsideContour). The bodies are
    // this port's original algorithm (n-ary contour tree, EPSILON boundary band).
    private static readonly EPSILON = 1e-18;

    public classifyContourHoles(): void {
        const polygon = this;
        for (const contour of polygon.loops) contour.fleetingFlag = false;
        const head = new BinaryTreeNode<_Polygon2DContour>(null as unknown as _Polygon2DContour);
        polygon.setHeadNode(head);
        for (let i = 0; i < polygon.loops.length; i++) {
            const contour = polygon.loops[i]!;
            const contained = [contour];
            for (let j = i + 1; j < polygon.loops.length; j++) {
                const candidate = polygon.loops[j]!;
                if (!candidate.fleetingFlag && Polygon2D.isContourInsideContour(contour.vertices, candidate.vertices))
                    contained.push(candidate);
            }
            contained.sort((a, b) => b.compareTo(a));
            Polygon2D.insertListInBinaryTree(contained, head);
        }
    }

    /** Returns true when the contour lies inside the supplied polygon contour. */
    private static isContourInsideContour(contour: readonly Vertex2D[], mainPolygon: readonly Vertex2D[]): boolean {
        for (const point of contour) {
            const result = Polygon2D.isPointInsideContour(point, mainPolygon);
            if (result === 1) return true;
            if (result === -1) return false;
        }
        return false;
    }

    /** Returns 1 inside, -1 outside, and 0 on the boundary. */
    public static isPointInsideContour(point: Vertex2D, polygon: readonly Vertex2D[]): number {
        if (polygon.length < 3) return -1;
        let isInside = false;
        for (let i = 0; i < polygon.length; i++) {
            const current = polygon[i]!;
            const next = polygon[(i + 1) % polygon.length]!;
            let temp = current.y - next.y;
            if (Math.abs(temp) < Polygon2D.EPSILON && Math.abs(point.y - (next.y + temp / 2)) < Polygon2D.EPSILON) {
                if ((next.x - point.x) * (point.x - current.x) >= 0) return 0;
            }
            if (point.y < current.y !== point.y < next.y) {
                temp = ((next.x - current.x) * (point.y - current.y)) / (next.y - current.y) + current.x;
                if (Math.abs(point.x - temp) < Polygon2D.EPSILON) return 0;
                if (point.x < temp) isInside = !isInside;
            }
        }
        return isInside ? 1 : -1;
    }

    private static insertListInBinaryTree(list: _Polygon2DContour[], head: BinaryTreeNode<_Polygon2DContour>): void {
        if (list.length === 0) return;
        const [containingNode, containingLevel] = Polygon2D.findContainingContourNode(list[0]!, head);
        Polygon2D.insertListInBinaryTreeNode(list, containingNode, containingLevel);
    }

    private static insertListInBinaryTreeNode(
        list: _Polygon2DContour[],
        containingNode: BinaryTreeNode<_Polygon2DContour>,
        containingLevel: number,
    ): void {
        let level = containingLevel + 1;
        if (level % 2 === 0) list[0]!.setExteriorContour(containingNode.getData());
        let lastChild = containingNode.getChild();
        if (lastChild === null) {
            lastChild = new BinaryTreeNode(list[0]!);
            containingNode.setChild(lastChild);
        } else {
            while (lastChild.getSibling() !== null) lastChild = lastChild.getSibling()!;
            const child = new BinaryTreeNode(list[0]!);
            lastChild.setSibling(child);
            lastChild = child;
        }
        list[0]!.fleetingFlag = true;
        for (let i = 1; i < list.length; i++) {
            level++;
            const contour = list[i]!;
            if (level % 2 === 0) contour.setExteriorContour(lastChild.getData());
            const child = new BinaryTreeNode(contour);
            lastChild.setChild(child);
            contour.fleetingFlag = true;
            lastChild = child;
        }
    }

    private static findContainingContourNode(
        contour: _Polygon2DContour,
        head: BinaryTreeNode<_Polygon2DContour>,
    ): [BinaryTreeNode<_Polygon2DContour>, number] {
        let current = head;
        let level = 0;
        while (true) {
            let child = current.getChild();
            let found = false;
            while (child !== null) {
                if (Polygon2D.isContourInsideContour(contour.vertices, child.getData().vertices)) {
                    current = child;
                    level++;
                    found = true;
                    break;
                }
                child = child.getSibling();
            }
            if (!found) return [current, level];
        }
    }
}
