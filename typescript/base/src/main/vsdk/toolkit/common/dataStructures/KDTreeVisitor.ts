export interface KDTreeVisitor<T> {
    visit(value: T): void;
}
