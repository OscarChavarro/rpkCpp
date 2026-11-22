export class KDTreeNode {
    public loson: KDTreeNode | null;
    public hison: KDTreeNode | null;

    public mFlags: number;
    public mData: unknown;

    public constructor() {
        this.loson = null;
        this.hison = null;
        this.mFlags = 0;
        this.mData = null;
    }

    public discriminator(): number {
        return this.mFlags & 0xf;
    }

    public setDiscriminator(discriminator: number): void {
        this.mFlags = (this.mFlags & 0xfff0) | discriminator;
    }

    public flags(): number {
        return this.mFlags & 0xfff0;
    }

    public findMinMaxDepth(depth: number, minDepth: number[], maxDepth: number[]): void {
        if (this.loson === null && this.hison === null) {
            const currentMax = maxDepth[0];
            const currentMin = minDepth[0];
            if (currentMax === undefined || currentMin === undefined) {
                return;
            }
            maxDepth[0] = globalThis.Math.max(currentMax, depth);
            minDepth[0] = globalThis.Math.min(currentMin, depth);
        } else {
            if (this.loson !== null) {
                this.loson.findMinMaxDepth(depth + 1, minDepth, maxDepth);
            }
            if (this.hison !== null) {
                this.hison.findMinMaxDepth(depth + 1, minDepth, maxDepth);
            }
        }
    }
}
