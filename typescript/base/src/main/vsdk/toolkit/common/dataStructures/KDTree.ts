import { Logger } from "../logging/Logger.js";
import type { Vector3Df } from "../linealAlgebra/Vector3Df.js";
import { BalancedKDTreeNode } from "./BalancedKDTreeNode.js";
import { KDTreeNode } from "./KDTreeNode.js";
import { KDQuery } from "./KDQuery.js";
import type { KDTreeAdapter } from "./KDTreeAdapter.js";
import type { KDTreeVisitor } from "./KDTreeVisitor.js";

const f = Math.fround;

/** Three-dimensional tree. Radius and distances are squared distances. */
export class KDTree<T> {
    public static readonly KD_MAX_RADIUS = 1e10;
    private root: KDTreeNode<T> | null = null;
    private balanced: BalancedKDTreeNode<T>[] | null = null;
    private numberOfNodes = 0;
    private unbalancedCount = 0;
    private balancedCount = 0;
    private firstLeaf = 0;

    public constructor(private readonly adapter: KDTreeAdapter<T>, private readonly copyData = true) {}

    public addPoint(value: T, flags: number): void {
        const stored = this.copyData ? this.adapter.copy(value) : value;
        const node = new KDTreeNode(stored, flags);
        const point = this.adapter.getKDPosition(stored);
        let parent: KDTreeNode<T> | null = null;
        let current = this.root;
        while (current !== null) {
            parent = current;
            const axis = current.discriminator();
            current = point.component(axis) <= this.adapter.getKDPosition(current.data).component(axis)
                ? current.loson : current.hison;
        }
        if (parent === null) {
            this.root = node;
        } else {
            const parentPoint = this.adapter.getKDPosition(parent.data);
            if (parent.loson === null && parent.hison === null) {
                const dx = Math.abs(f(point.x() - parentPoint.x()));
                const dy = Math.abs(f(point.y() - parentPoint.y()));
                const dz = Math.abs(f(point.z() - parentPoint.z()));
                const axis = dx > dy ? (dx > dz ? 0 : 2) : (dy > dz ? 1 : 2);
                parent.setDiscriminator(axis);
            }
            if (point.component(parent.discriminator()) <= parentPoint.component(parent.discriminator())) parent.loson = node;
            else parent.hison = node;
        }
        ++this.numberOfNodes;
        ++this.unbalancedCount;
    }

    public query(point: Vector3Df, count: number, results: T[], distances: number[] | null,
                 radius = KDTree.KD_MAX_RADIUS, excluded = 0): number {
        if (count <= 0) return 0;
        if (distances === null && count > 1000) {
            Logger.reportMessage(this, Logger.ERROR, "KDTree::query", "Too many nodes requested");
            return 0;
        }
        const q = new KDQuery(point, count, results, distances ?? new Array<number>(1000).fill(0), radius, radius, excluded);
        if (this.balanced !== null) this.balancedQueryRec(0, q);
        if (this.root !== null) this.queryRec(this.root, q);
        return q.found;
    }
    private distance(value: T, point: Vector3Df): number {
        const position = this.adapter.getKDPosition(value);
        let d = f(position.x() - point.x());
        let result = f(d * d);
        d = f(position.y() - point.y()); result = f(result + f(d * d));
        d = f(position.z() - point.z()); return f(result + f(d * d));
    }
    private consider(data: T, flags: number, q: KDQuery<T>): void {
        const dist = this.distance(data, q.point);
        if (dist >= q.maximumDistance || (flags & q.excludeFlags) !== 0) return;
        if (q.found < q.wanted) {
            let child = q.found;
            q.results[child] = data;
            q.distances[child] = dist;
            while (child > 0) {
                const parent = (child - 1) >> 1;
                if (q.distances[parent]! >= q.distances[child]!) break;
                this.swapResults(q, parent, child);
                child = parent;
            }
            if (++q.found === q.wanted) q.maximumDistance = q.distances[0]!;
        } else {
            q.results[0] = data;
            q.distances[0] = dist;
            let parent = 0;
            let child = 1;
            while (child < q.found) {
                if (q.distances[child]! <= q.distances[parent]!) {
                    if (++child >= q.found || q.distances[child]! <= q.distances[parent]!) break;
                } else if (child + 1 < q.found && q.distances[child + 1]! > q.distances[child]!) {
                    ++child;
                }
                this.swapResults(q, parent, child);
                parent = child;
                child = (parent << 1) + 1;
            }
            q.maximumDistance = q.distances[0]!;
        }
    }
    private swapResults(q: KDQuery<T>, a: number, b: number): void {
        const value = q.results[a]!; q.results[a] = q.results[b]!; q.results[b] = value;
        const dist = q.distances[a]!; q.distances[a] = q.distances[b]!; q.distances[b] = dist;
    }
    private queryRec(node: KDTreeNode<T>, q: KDQuery<T>): void {
        this.consider(node.data, node.flags, q);
        let d = f(this.adapter.getKDPosition(node.data).component(node.discriminator()) - q.point.component(node.discriminator()));
        const near = d >= 0 ? node.loson : node.hison;
        const far = d >= 0 ? node.hison : node.loson;
        if (near !== null) this.queryRec(near, q);
        d = f(d * d);
        if (far !== null && ((q.found < q.wanted && d < q.squaredRadius) || d < q.maximumDistance)) this.queryRec(far, q);
    }
    private balancedQueryRec(index: number, q: KDQuery<T>): void {
        const node = this.balanced![index]!;
        if (index < this.firstLeaf) {
            const axis = node.discriminator();
            let d = f(this.adapter.getKDPosition(node.data).component(axis) - q.point.component(axis));
            let near = (index << 1) + 1;
            let far = near + 1;
            if (d < 0) { const temp = near; near = far; far = temp; }
            if (near < this.balancedCount) this.balancedQueryRec(near, q);
            d = f(d * d);
            if (far < this.balancedCount && ((q.found < q.wanted && d < q.squaredRadius) || d < q.maximumDistance)) this.balancedQueryRec(far, q);
        }
        this.consider(node.data, node.flags(), q);
    }
    public balance(): void {
        if (this.unbalancedCount === 0) return;
        const source: BalancedKDTreeNode<T>[] = [];
        const dest: BalancedKDTreeNode<T>[] = new Array(this.numberOfNodes);
        if (this.balanced !== null) source.push(...this.balanced);
        this.copyNodes(this.root, source);
        this.balanceRec(source, dest, 0, 0, this.numberOfNodes - 1);
        this.balanced = dest;
        this.balancedCount = this.numberOfNodes;
        this.firstLeaf = Math.trunc((this.balancedCount + 1) / 2);
        this.root = null;
        this.unbalancedCount = 0;
    }
    private copyNodes(node: KDTreeNode<T> | null, dest: BalancedKDTreeNode<T>[]): void {
        if (node === null) return;
        dest.push(new BalancedKDTreeNode(node.data, node.flags & 0xfff0));
        this.copyNodes(node.loson, dest);
        this.copyNodes(node.hison, dest);
    }
    private balanceRec(source: BalancedKDTreeNode<T>[], dest: BalancedKDTreeNode<T>[], index: number, low: number, high: number): void {
        if (low === high) { dest[index] = source[low]!; dest[index]!.setDiscriminator(0); return; }
        const axis = this.bestAxis(source, low, high);
        const median = this.select(source, low, high, axis);
        dest[index] = source[median]!;
        dest[index]!.setDiscriminator(axis);
        if (low < median) this.balanceRec(source, dest, (index << 1) + 1, low, median - 1);
        if (high > median) this.balanceRec(source, dest, (index << 1) + 2, median + 1, high);
    }
    private bestAxis(nodes: BalancedKDTreeNode<T>[], low: number, high: number): number {
        const min = [Number.MAX_VALUE, Number.MAX_VALUE, Number.MAX_VALUE];
        const max = [-Number.MAX_VALUE, -Number.MAX_VALUE, -Number.MAX_VALUE];
        for (let i = low; i <= high; ++i) {
            const point = this.adapter.getKDPosition(nodes[i]!.data);
            for (let axis = 0; axis < 3; ++axis) {
                const value = point.component(axis);
                if (value < min[axis]!) min[axis] = value;
                if (value > max[axis]!) max[axis] = value;
            }
        }
        let axis = 0;
        let spread = f(max[0]! - min[0]!);
        if (f(max[1]! - min[1]!) > spread) { axis = 1; spread = f(max[1]! - min[1]!); }
        if (f(max[2]! - min[2]!) > spread) axis = 2;
        return axis;
    }
    private value(nodes: BalancedKDTreeNode<T>[], index: number, axis: number): number {
        return this.adapter.getKDPosition(nodes[index]!.data).component(axis);
    }
    private swap(nodes: BalancedKDTreeNode<T>[], a: number, b: number): void {
        const temp = nodes[a]!; nodes[a] = nodes[b]!; nodes[b] = temp;
    }
    private select(nodes: BalancedKDTreeNode<T>[], low: number, high: number, axis: number): number {
        const n = high - low + 1;
        const power = 1 << Math.trunc(Math.log(n + 0.1) / Math.log(2));
        const last = n - (power - 1);
        const half = power >> 1;
        const median = low + (last < half ? last + half - 1 : half + half - 1);
        while (true) {
            if (high <= low) return median;
            if (high === low + 1) {
                if (this.value(nodes, low, axis) > this.value(nodes, high, axis)) this.swap(nodes, low, high);
                return median;
            }
            const middle = Math.trunc((low + high + 1) / 2);
            if (this.value(nodes, middle, axis) > this.value(nodes, high, axis)) this.swap(nodes, middle, high);
            if (this.value(nodes, low, axis) > this.value(nodes, high, axis)) this.swap(nodes, low, high);
            if (this.value(nodes, middle, axis) > this.value(nodes, low, axis)) this.swap(nodes, middle, low);
            this.swap(nodes, middle, low + 1);
            let ll = low + 1, hh = high;
            while (true) {
                do { ++ll; } while (this.value(nodes, low, axis) > this.value(nodes, ll, axis));
                do { --hh; } while (this.value(nodes, hh, axis) > this.value(nodes, low, axis));
                if (hh < ll) break;
                this.swap(nodes, ll, hh);
            }
            this.swap(nodes, low, hh);
            if (hh <= median) low = ll;
            if (hh >= median) high = hh - 1;
        }
    }
    public getBalancedCount(): number { return this.balancedCount; }
    public getUnbalancedCount(): number { return this.unbalancedCount; }
    public getFirstLeaf(): number { return this.firstLeaf; }
    public getBalancedNode(index: number): BalancedKDTreeNode<T> { return this.balanced![index]!; }
    public iterateNodes(visitor: KDTreeVisitor<T>): void {
        if (this.unbalancedCount > 0) { Logger.reportMessage(this, Logger.ERROR, "KDTree::iterateNodes", "Cannot iterate unbalanced trees"); return; }
        for (let i = 0; i < this.balancedCount; ++i) visitor.visit(this.balanced![i]!.data);
    }
}
