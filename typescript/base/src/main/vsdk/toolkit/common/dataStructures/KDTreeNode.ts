export class KDTreeNode<T> {
    public loson: KDTreeNode<T> | null = null;
    public hison: KDTreeNode<T> | null = null;
    public flags: number;
    public constructor(public readonly data: T, flags: number) { this.flags = flags & 0xfff0; }
    public discriminator(): number { return this.flags & 0xf; }
    public setDiscriminator(axis: number): void { this.flags = (this.flags & 0xfff0) | axis; }
}
