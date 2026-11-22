import { CircularListLink } from "./CircularListLink.js";

export class CircularListNode<T> extends CircularListLink {
    public data: T;

    public constructor(inData: T) {
        super();
        this.data = inData;
    }
}
