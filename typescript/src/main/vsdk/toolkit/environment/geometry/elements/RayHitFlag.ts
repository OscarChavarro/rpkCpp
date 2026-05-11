export class RayHitFlag {
  public static readonly GEOMETRY = 0x01;
  public static readonly PATCH = 0x02;
  public static readonly POINT = 0x04;
  public static readonly GEOMETRIC_NORMAL = 0x08;
  public static readonly MATERIAL = 0x10;
  public static readonly DISTANCE = 0x20;

  public static readonly UV = 0x100;
  public static readonly TEXTURE_COORDINATE = 0x200;
  public static readonly SHADING_FRAME = 0x400;

  public static readonly NORMAL = 0x800;

  public static readonly FRONT = 0x10000;
  public static readonly BACK = 0x20000;
  public static readonly ANY = 0x40000;

  private constructor() {
  }
}
