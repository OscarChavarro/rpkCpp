package vsdk.toolkit.material;

public final class XxdfComponentFlag {
    public static final int DIFFUSE_COMPONENT = 1;
    public static final int GLOSSY_COMPONENT = 2;
    public static final int SPECULAR_COMPONENT = 4;

    private XxdfComponentFlag() {
    }
}

final class XxdfComponentFlagInfo {
    static final int XXDF_COMPONENTS = 3;
    static final int NO_COMPONENTS = 0;
    static final int ALL_COMPONENTS = XxdfComponentFlag.DIFFUSE_COMPONENT
        | XxdfComponentFlag.GLOSSY_COMPONENT
        | XxdfComponentFlag.SPECULAR_COMPONENT;

    private XxdfComponentFlagInfo() {
    }
}
