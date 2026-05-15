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
    return this.mFlags & 0xF;
  }

  public setDiscriminator(discriminator: number): void {
    this.mFlags = (this.mFlags & 0xFFF0) | discriminator;
  }

  public flags(): number {
    return this.mFlags & 0xFFF0;
  }

  public findMinMaxDepth(depth: number, minDepth: number[], maxDepth: number[]): void {
    if ((this.loson === null) && (this.hison === null)) {
      maxDepth[0] = globalThis.Math.max(maxDepth[0], depth);
      minDepth[0] = globalThis.Math.min(minDepth[0], depth);
    }
    else {
      if (this.loson !== null) {
        this.loson.findMinMaxDepth(depth + 1, minDepth, maxDepth);
      }
      if (this.hison !== null) {
        this.hison.findMinMaxDepth(depth + 1, minDepth, maxDepth);
      }
    }
  }
}
