import { IllegalArgumentException } from "../../../../../../java/lang/IllegalArgumentException.js";
import type { _PolyhedralBoundedSolidEdge } from "./nodes/_PolyhedralBoundedSolidEdge.js";
import type { _PolyhedralBoundedSolidFace } from "./nodes/_PolyhedralBoundedSolidFace.js";
import type { _PolyhedralBoundedSolidHalfEdge } from "./nodes/_PolyhedralBoundedSolidHalfEdge.js";
import type { _PolyhedralBoundedSolidVertex } from "./nodes/_PolyhedralBoundedSolidVertex.js";
import type { PolyhedralBoundedSolid } from "./PolyhedralBoundedSolid.js";

/**
Topology values for one face-connected shell.
 */
class Shell {
    private readonly faceCount: number;
    private readonly edgeCount: number;
    private readonly vertexCount: number;
    private readonly adjustedEulerCharacteristic: number;
    private readonly closed: boolean;
    private readonly closedOrientableEulerCompatible: boolean;

    /** Java private constructor, reachable only from the enclosing summary. */
    public constructor(
        faceCount: number,
        edgeCount: number,
        vertexCount: number,
        adjustedEulerCharacteristic: number,
        closed: boolean,
    ) {
        this.faceCount = faceCount;
        this.edgeCount = edgeCount;
        this.vertexCount = vertexCount;
        this.adjustedEulerCharacteristic = adjustedEulerCharacteristic;
        this.closed = closed;
        this.closedOrientableEulerCompatible =
            closed && adjustedEulerCharacteristic <= 2 && Shell.floorMod(adjustedEulerCharacteristic, 2) === 0;
    }

    /** Java `Math.floorMod(int, int)`. */
    private static floorMod(x: number, y: number): number {
        return ((x % y) + y) % y;
    }

    public getFaceCount(): number {
        return this.faceCount;
    }

    public getEdgeCount(): number {
        return this.edgeCount;
    }

    public getVertexCount(): number {
        return this.vertexCount;
    }

    public getAdjustedEulerCharacteristic(): number {
        return this.adjustedEulerCharacteristic;
    }

    public isClosed(): boolean {
        return this.closed;
    }

    public isClosedOrientableEulerCompatible(): boolean {
        return this.closedOrientableEulerCompatible;
    }

    public toString(): string {
        return (
            "Shell{faces=" +
            this.faceCount +
            ", edges=" +
            this.edgeCount +
            ", vertices=" +
            this.vertexCount +
            ", adjustedEuler=" +
            this.adjustedEulerCharacteristic +
            ", closed=" +
            this.closed +
            ", closedOrientableEulerCompatible=" +
            this.closedOrientableEulerCompatible +
            "}"
        );
    }
}

class DisjointSet {
    private readonly parent: Int32Array;
    private readonly rank: Int8Array;

    public constructor(size: number) {
        this.parent = new Int32Array(size);
        this.rank = new Int8Array(size);
        let i: number;
        for (i = 0; i < size; i++) {
            this.parent[i] = i;
        }
    }

    public find(value: number): number {
        if (this.parent[value] !== value) {
            this.parent[value] = this.find(this.parent[value]!);
        }
        return this.parent[value]!;
    }

    public union(first: number, second: number): void {
        const firstRoot = this.find(first);
        const secondRoot = this.find(second);
        if (firstRoot === secondRoot) {
            return;
        }
        if (this.rank[firstRoot]! < this.rank[secondRoot]!) {
            this.parent[firstRoot] = secondRoot;
        } else if (this.rank[firstRoot]! > this.rank[secondRoot]!) {
            this.parent[secondRoot] = firstRoot;
        } else {
            this.parent[secondRoot] = firstRoot;
            this.rank[firstRoot]!++;
        }
    }
}

/**
Immutable connected-shell and adjusted-Euler summary for a polyhedral B-Rep.

<p>A face with inner boundaries contributes {@code 2 - boundaryLoopCount}
instead of one to the Euler face term. Therefore the reported characteristic
is {@code V - E + sum(2 - boundaryLoopCount(face))}.</p>
 */
export class PolyhedralBoundedSolidTopologySummary {
    private readonly faceCount: number;
    private readonly edgeCount: number;
    private readonly vertexCount: number;
    private readonly adjustedEulerCharacteristic: number;
    private readonly everyFaceReachedExactlyOnce: boolean;
    private readonly invalidEdgeAdjacencyCount: number;
    private readonly shells: readonly Shell[];

    private constructor(solid: PolyhedralBoundedSolid) {
        this.faceCount = solid.getPolygonsList().size();
        this.edgeCount = solid.getEdgesList().size();
        this.vertexCount = solid.getVerticesList().size();

        const faceIndexes = new Map<_PolyhedralBoundedSolidFace, number>();
        let i: number;
        for (i = 0; i < this.faceCount; i++) {
            faceIndexes.set(solid.getPolygonsList().get(i)!, i);
        }

        const components = new DisjointSet(this.faceCount);
        let invalidAdjacencies = 0;
        for (i = 0; i < this.edgeCount; i++) {
            const edge = solid.getEdgesList().get(i)!;
            const leftFace = PolyhedralBoundedSolidTopologySummary.faceOf(edge.leftHalf);
            const rightFace = PolyhedralBoundedSolidTopologySummary.faceOf(edge.rightHalf);
            const leftIndex = leftFace === null ? undefined : faceIndexes.get(leftFace);
            const rightIndex = rightFace === null ? undefined : faceIndexes.get(rightFace);
            if (leftIndex === undefined || rightIndex === undefined) {
                invalidAdjacencies++;
                continue;
            }
            components.union(leftIndex, rightIndex);
        }
        this.invalidEdgeAdjacencyCount = invalidAdjacencies;

        // Java uses a TreeMap keyed by the component root: iteration is in
        // ascending root order.
        const componentFaces = new Map<number, _PolyhedralBoundedSolidFace[]>();
        let reachedFaces = 0;
        for (i = 0; i < this.faceCount; i++) {
            const root = components.find(i);
            let faces = componentFaces.get(root);
            if (faces === undefined) {
                faces = [];
                componentFaces.set(root, faces);
            }
            faces.push(solid.getPolygonsList().get(i)!);
            reachedFaces++;
        }
        const orderedRoots = Array.from(componentFaces.keys()).sort((a, b) => a - b);
        let facesInComponents = 0;
        for (const root of orderedRoots) {
            facesInComponents += componentFaces.get(root)!.length;
        }
        this.everyFaceReachedExactlyOnce =
            faceIndexes.size === this.faceCount &&
            reachedFaces === this.faceCount &&
            facesInComponents === this.faceCount;

        const computedShells: Shell[] = [];
        let totalAdjustedFaceTerm = 0;
        for (const root of orderedRoots) {
            const faces = componentFaces.get(root)!;
            const faceSet = new Set<_PolyhedralBoundedSolidFace>();
            for (const face of faces) {
                faceSet.add(face);
            }
            const shellEdges = new Set<_PolyhedralBoundedSolidEdge>();
            const shellVertices = new Set<_PolyhedralBoundedSolidVertex>();
            let adjustedFaceTerm = 0;
            let closed = true;

            for (const face of faces) {
                adjustedFaceTerm += 2 - face.boundariesList.size();
                PolyhedralBoundedSolidTopologySummary.collectFaceVertices(face, shellVertices);
            }
            totalAdjustedFaceTerm += adjustedFaceTerm;

            for (i = 0; i < solid.getEdgesList().size(); i++) {
                const edge = solid.getEdgesList().get(i)!;
                const leftFace = PolyhedralBoundedSolidTopologySummary.faceOf(edge.leftHalf);
                const rightFace = PolyhedralBoundedSolidTopologySummary.faceOf(edge.rightHalf);
                const touchesShell =
                    (leftFace !== null && faceSet.has(leftFace)) || (rightFace !== null && faceSet.has(rightFace));
                if (!touchesShell) {
                    continue;
                }
                shellEdges.add(edge);
                if (leftFace === null || rightFace === null || !faceSet.has(leftFace) || !faceSet.has(rightFace)) {
                    closed = false;
                }
                PolyhedralBoundedSolidTopologySummary.collectVertex(edge.leftHalf, shellVertices);
                PolyhedralBoundedSolidTopologySummary.collectVertex(edge.rightHalf, shellVertices);
            }

            const chi = shellVertices.size - shellEdges.size + adjustedFaceTerm;
            computedShells.push(new Shell(faces.length, shellEdges.size, shellVertices.size, chi, closed));
        }
        this.shells = Object.freeze(computedShells);
        this.adjustedEulerCharacteristic = this.vertexCount - this.edgeCount + totalAdjustedFaceTerm;
    }

    public static from(solid: PolyhedralBoundedSolid | null): PolyhedralBoundedSolidTopologySummary {
        if (solid === null) {
            throw new IllegalArgumentException("solid must not be null");
        }
        return new PolyhedralBoundedSolidTopologySummary(solid);
    }

    public getFaceCount(): number {
        return this.faceCount;
    }

    public getEdgeCount(): number {
        return this.edgeCount;
    }

    public getVertexCount(): number {
        return this.vertexCount;
    }

    public getShellCount(): number {
        return this.shells.length;
    }

    public getAdjustedEulerCharacteristic(): number {
        return this.adjustedEulerCharacteristic;
    }

    public isEveryFaceReachedExactlyOnce(): boolean {
        return this.everyFaceReachedExactlyOnce;
    }

    public getInvalidEdgeAdjacencyCount(): number {
        return this.invalidEdgeAdjacencyCount;
    }

    public getShells(): readonly Shell[] {
        return this.shells;
    }

    public hasUniversalContradiction(): boolean {
        if (!this.everyFaceReachedExactlyOnce || this.invalidEdgeAdjacencyCount > 0) {
            return true;
        }
        for (const shell of this.shells) {
            if (!shell.isClosedOrientableEulerCompatible()) {
                return true;
            }
        }
        return false;
    }

    public toString(): string {
        return (
            "TopologySummary{faces=" +
            this.faceCount +
            ", edges=" +
            this.edgeCount +
            ", vertices=" +
            this.vertexCount +
            ", shells=" +
            this.shells.length +
            ", adjustedEuler=" +
            this.adjustedEulerCharacteristic +
            ", everyFaceReachedExactlyOnce=" +
            this.everyFaceReachedExactlyOnce +
            ", invalidEdgeAdjacencies=" +
            this.invalidEdgeAdjacencyCount +
            ", perShell=[" +
            this.shells.map((shell) => shell.toString()).join(", ") +
            "]}"
        );
    }

    private static faceOf(halfEdge: _PolyhedralBoundedSolidHalfEdge | null): _PolyhedralBoundedSolidFace | null {
        if (halfEdge === null || halfEdge.parentLoop === null) {
            return null;
        }
        return halfEdge.parentLoop.parentFace;
    }

    private static collectVertex(
        halfEdge: _PolyhedralBoundedSolidHalfEdge | null,
        vertices: Set<_PolyhedralBoundedSolidVertex>,
    ): void {
        if (halfEdge !== null && halfEdge.startingVertex !== null) {
            vertices.add(halfEdge.startingVertex);
        }
    }

    private static collectFaceVertices(
        face: _PolyhedralBoundedSolidFace,
        vertices: Set<_PolyhedralBoundedSolidVertex>,
    ): void {
        let i: number;
        let j: number;
        for (i = 0; i < face.boundariesList.size(); i++) {
            for (j = 0; j < face.boundariesList.get(i)!.halfEdgesList.size(); j++) {
                PolyhedralBoundedSolidTopologySummary.collectVertex(
                    face.boundariesList.get(i)!.halfEdgesList.get(j),
                    vertices,
                );
            }
        }
    }
}

type _Shell = Shell;

export namespace PolyhedralBoundedSolidTopologySummary {
    export type Shell = _Shell;
}

export { Shell as PolyhedralBoundedSolidTopologySummaryShell };
