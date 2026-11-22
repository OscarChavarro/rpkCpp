export class BsdfComponent {
  public static readonly BRDF_DIFFUSE_COMPONENT = 0x01;
  public static readonly BRDF_GLOSSY_COMPONENT = 0x02;
  public static readonly BRDF_SPECULAR_COMPONENT = 0x04;
  public static readonly BTDF_DIFFUSE_COMPONENT = 0x08;
  public static readonly BTDF_GLOSSY_COMPONENT = 0x10;
  public static readonly BTDF_SPECULAR_COMPONENT = 0x20;

  private constructor() {
  }
}

export class BsdfComponentInfo {
  public static readonly BSDF_DIFFUSE_COMPONENT =
    BsdfComponent.BTDF_DIFFUSE_COMPONENT | BsdfComponent.BRDF_DIFFUSE_COMPONENT;
  public static readonly BSDF_GLOSSY_COMPONENT =
    BsdfComponent.BTDF_GLOSSY_COMPONENT | BsdfComponent.BRDF_GLOSSY_COMPONENT;
  public static readonly BSDF_SPECULAR_COMPONENT =
    BsdfComponent.BTDF_SPECULAR_COMPONENT | BsdfComponent.BRDF_SPECULAR_COMPONENT;
  public static readonly BSDF_COMPONENTS = 6;
  public static readonly BSDF_ALL_COMPONENTS =
    BsdfComponent.BRDF_DIFFUSE_COMPONENT
    | BsdfComponent.BRDF_GLOSSY_COMPONENT
    | BsdfComponent.BRDF_SPECULAR_COMPONENT
    | BsdfComponent.BTDF_DIFFUSE_COMPONENT
    | BsdfComponent.BTDF_GLOSSY_COMPONENT
    | BsdfComponent.BTDF_SPECULAR_COMPONENT;

  private constructor() {
  }
}
