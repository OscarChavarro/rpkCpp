package vsdk.toolkit.processing;

public enum Containment {
    OUTSIDE(-1),
    LIMIT(0),
    INSIDE(1);

    private final int value;

    Containment(int value) {
        this.value = value;
    }

    public int value() {
        return value;
    }
}
