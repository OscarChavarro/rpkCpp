import { FundamentalEntity } from "../FundamentalEntity.js";

export class BinaryTreeNode<T> extends FundamentalEntity {
    private data: T;
    private sibling: BinaryTreeNode<T> | null = null;
    private child: BinaryTreeNode<T> | null = null;
    public constructor(data: T) {
        super();
        this.data = data;
    }
    public getSibling(): BinaryTreeNode<T> | null {
        return this.sibling;
    }
    public setSibling(sibling: BinaryTreeNode<T> | null): void {
        this.sibling = sibling;
    }
    public getChild(): BinaryTreeNode<T> | null {
        return this.child;
    }
    public setChild(child: BinaryTreeNode<T> | null): void {
        this.child = child;
    }
    public getData(): T {
        return this.data;
    }
    public setData(data: T): void {
        this.data = data;
    }
}
