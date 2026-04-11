import { KDTreeNode } from "./KDTreeNode";

export class BalancedKDTreeNode {
  public mData: unknown;
  public mFlags: number;

  public constructor() {
    this.mData = null;
    this.mFlags = 0;
  }

  public copy(kdNode: KDTreeNode): void {
    this.mData = kdNode.mData;
    this.mFlags = kdNode.flags();
  }

  public discriminator(): number {
    return this.mFlags & 0xF;
  }

  public setDiscriminator(discr: number): void {
    this.mFlags = (this.mFlags & 0xFFF0) | discr;
  }
}
