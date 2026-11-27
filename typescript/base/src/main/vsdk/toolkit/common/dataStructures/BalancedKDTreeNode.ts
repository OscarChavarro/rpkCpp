export class BalancedKDTreeNode<T> {
    public constructor(
        public readonly data: T,
        private nodeFlags: number,
    ) {}
    public discriminator(): number {
        return this.nodeFlags & 0xf;
    }
    public flags(): number {
        return this.nodeFlags & 0xfff0;
    }
    public setDiscriminator(axis: number): void {
        this.nodeFlags = (this.nodeFlags & 0xfff0) | axis;
    }
}
