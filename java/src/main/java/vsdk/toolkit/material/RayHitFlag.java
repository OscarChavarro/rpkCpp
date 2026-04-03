package vsdk.toolkit.material;

public final class RayHitFlag {
    public static final int GEOMETRY = 0x01;
    public static final int PATCH = 0x02;
    public static final int POINT = 0x04;
    public static final int GEOMETRIC_NORMAL = 0x08;
    public static final int MATERIAL = 0x10;
    public static final int DISTANCE = 0x20;

    public static final int UV = 0x100;
    public static final int TEXTURE_COORDINATE = 0x200;
    public static final int SHADING_FRAME = 0x400;

    public static final int NORMAL = 0x800;

    public static final int FRONT = 0x10000;
    public static final int BACK = 0x20000;
    public static final int ANY = 0x40000;

    private RayHitFlag() {
    }
}
