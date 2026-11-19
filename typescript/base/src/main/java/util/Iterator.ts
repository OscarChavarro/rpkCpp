/** Java-style iterator API, including explicit hasNext and remove semantics. */
export interface Iterator<T> {
    hasNext(): boolean;
    next(): T;
    remove(): void;
}
