export class XxdfComponentFlag {
  public static readonly DIFFUSE_COMPONENT = 1;
  public static readonly GLOSSY_COMPONENT = 2;
  public static readonly SPECULAR_COMPONENT = 4;

  private constructor() {
  }
}

export class XxdfComponentFlagInfo {
  public static readonly XXDF_COMPONENTS = 3;
  public static readonly NO_COMPONENTS = 0;
  public static readonly ALL_COMPONENTS = XxdfComponentFlag.DIFFUSE_COMPONENT
    | XxdfComponentFlag.GLOSSY_COMPONENT
    | XxdfComponentFlag.SPECULAR_COMPONENT;

  private constructor() {
  }
}
