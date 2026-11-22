/**
Per-query scratch of a `SpatialIndex`: the traversal counters and, for
indexes whose cells may list one primitive several times (grids), a mailbox
that records the last query that tested each primitive.

Caller-owned and reusable: one workspace per worker, never shared between
concurrent queries. The counters accumulate over the queries until the
caller reads and clears them; nothing static is written (the counters of all
workers are merged by the caller).
*/
export class SpatialIndexWorkspace {
    private nodeVisits: number;
    private primitiveTests: number;
    /** C++ `unsigned int` array */
    private readonly lastQueryByPrimitive: Uint32Array | null;
    private readonly numberOfPrimitives: number;
    /** C++ `unsigned int`: wraps at 2^32 */
    private queryCount: number;

    /**
    @param inNumberOfPrimitives size of the mailbox: the number of primitives
    of the index, or zero when the index does not use one
    */
    public constructor(inNumberOfPrimitives = 0) {
        this.nodeVisits = 0;
        this.primitiveTests = 0;
        this.numberOfPrimitives = inNumberOfPrimitives > 0 ? inNumberOfPrimitives : 0;
        this.queryCount = 0;
        this.lastQueryByPrimitive = null;
        if (this.numberOfPrimitives > 0) {
            this.lastQueryByPrimitive = new Uint32Array(this.numberOfPrimitives);
            for (let i = 0; i < this.numberOfPrimitives; i++) {
                this.lastQueryByPrimitive[i] = 0;
            }
        }
    }

    public getNodeVisits(): number {
        return this.nodeVisits;
    }

    public getPrimitiveTests(): number {
        return this.primitiveTests;
    }

    public addNodeVisits(count: number): void {
        this.nodeVisits += count;
    }

    public addPrimitiveTests(count: number): void {
        this.primitiveTests += count;
    }

    public clearCounters(): void {
        this.nodeVisits = 0;
        this.primitiveTests = 0;
    }

    public getNumberOfPrimitives(): number {
        return this.numberOfPrimitives;
    }

    /** Starts a query that uses the mailbox */
    public beginMailboxQuery(): void {
        this.queryCount = (this.queryCount + 1) >>> 0;
        if (this.queryCount === 0) {
            // The counter wrapped: clear the mailbox, so that no primitive appears
            // as already reached by this query, and skip the value 0 it holds
            for (let i = 0; i < this.numberOfPrimitives; i++) {
                this.lastQueryByPrimitive![i] = 0;
            }
            this.queryCount = 1;
        }
    }

    /**
    Records that the current query reaches a primitive; returns false when
    the query had already reached it (the primitive is then not tested
    again). Valid after beginMailboxQuery, for primitives below the mailbox
    size.
    */
    public firstVisit(primitive: number): boolean {
        if (this.lastQueryByPrimitive![primitive] === this.queryCount) {
            return false;
        }
        this.lastQueryByPrimitive![primitive] = this.queryCount;
        return true;
    }
}
