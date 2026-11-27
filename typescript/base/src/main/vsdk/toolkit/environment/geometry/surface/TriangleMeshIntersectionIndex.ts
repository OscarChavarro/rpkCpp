// Legacy last-hit index; queries carrying their own index must not depend on it.
// C++ stores it in an AtomicLong; JavaScript workers do not share this object.
export class TriangleMeshIntersectionIndex {
    private value: number;
    public constructor();
    /** C++ copy constructor. */
    public constructor(other: TriangleMeshIntersectionIndex);
    public constructor(other?: TriangleMeshIntersectionIndex) {
        this.value = other === undefined ? -1 : other.get();
    }
    /** C++ copy assignment. */
    public assign(other: TriangleMeshIntersectionIndex): TriangleMeshIntersectionIndex {
        if (this !== other) this.value = other.get();
        return this;
    }
    public get(): number {
        return this.value;
    }
    public set(index: number): void {
        // Every traced ray stores its result here: skipping the write when
        // the value is already there (most often -1, a miss) mirrors C++,
        // where it keeps the shared cache line from bouncing between cores.
        if (this.value !== index) this.value = index;
    }
}
