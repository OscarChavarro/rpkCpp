export interface Comparator<T> {
    compare(left: T, right: T): number;
}
