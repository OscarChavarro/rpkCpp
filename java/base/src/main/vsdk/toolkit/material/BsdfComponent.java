package vsdk.toolkit.material;

public final class BsdfComponent {
    public static final int BRDF_DIFFUSE_COMPONENT = 0x01;
    public static final int BRDF_GLOSSY_COMPONENT = 0x02;
    public static final int BRDF_SPECULAR_COMPONENT = 0x04;
    public static final int BTDF_DIFFUSE_COMPONENT = 0x08;
    public static final int BTDF_GLOSSY_COMPONENT = 0x10;
    public static final int BTDF_SPECULAR_COMPONENT = 0x20;

    private BsdfComponent() {
    }
}

final class BsdfComponentInfo {
    static final int BSDF_DIFFUSE_COMPONENT = BsdfComponent.BTDF_DIFFUSE_COMPONENT | BsdfComponent.BRDF_DIFFUSE_COMPONENT;
    static final int BSDF_GLOSSY_COMPONENT = BsdfComponent.BTDF_GLOSSY_COMPONENT | BsdfComponent.BRDF_GLOSSY_COMPONENT;
    static final int BSDF_SPECULAR_COMPONENT = BsdfComponent.BTDF_SPECULAR_COMPONENT | BsdfComponent.BRDF_SPECULAR_COMPONENT;
    static final int BSDF_COMPONENTS = 6;
    static final int BSDF_ALL_COMPONENTS = BsdfComponent.BRDF_DIFFUSE_COMPONENT
        | BsdfComponent.BRDF_GLOSSY_COMPONENT
        | BsdfComponent.BRDF_SPECULAR_COMPONENT
        | BsdfComponent.BTDF_DIFFUSE_COMPONENT
        | BsdfComponent.BTDF_GLOSSY_COMPONENT
        | BsdfComponent.BTDF_SPECULAR_COMPONENT;

    private BsdfComponentInfo() {
    }
}
