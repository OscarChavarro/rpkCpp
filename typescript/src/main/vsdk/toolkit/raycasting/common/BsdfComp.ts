import { ColorRgb } from "../../common/ColorRgb";
import { BsdfComponent } from "../../material/BsdfComponent";
import { BsdfComponentFlag } from "../../material/BsdfComponentFlag";

export class BsdfComp {
  private static readonly BSDF_COMPONENTS = 6;
  private static readonly BSDF_ALL_COMPONENTS =
    BsdfComponent.BRDF_DIFFUSE_COMPONENT
    | BsdfComponent.BRDF_GLOSSY_COMPONENT
    | BsdfComponent.BRDF_SPECULAR_COMPONENT
    | BsdfComponent.BTDF_DIFFUSE_COMPONENT
    | BsdfComponent.BTDF_GLOSSY_COMPONENT
    | BsdfComponent.BTDF_SPECULAR_COMPONENT;

  public comp: ColorRgb[];

  public constructor() {
    this.comp = new Array<ColorRgb>(BsdfComp.BSDF_COMPONENTS);
    for (let i = 0; i < BsdfComp.BSDF_COMPONENTS; i++) {
      this.comp[i] = new ColorRgb();
    }
  }

  public get(index: number): ColorRgb {
    return this.comp[index];
  }

  public asArray(): ColorRgb[] {
    return this.comp;
  }

  public Clear(flags?: number): void {
    const useFlags = flags ?? BsdfComp.BSDF_ALL_COMPONENTS;
    for (let i = 0; i < BsdfComp.BSDF_COMPONENTS; i++) {
      if ((useFlags & BsdfComponentFlag.bsdfIndexToComp(i)) !== 0) {
        this.comp[i].clear();
      }
    }
  }

  public Fill(col: ColorRgb, flags?: number): void {
    const useFlags = flags ?? BsdfComp.BSDF_ALL_COMPONENTS;
    for (let i = 0; i < BsdfComp.BSDF_COMPONENTS; i++) {
      if ((useFlags & BsdfComponentFlag.bsdfIndexToComp(i)) !== 0) {
        this.comp[i].set(col.r, col.g, col.b);
      }
    }
  }

  public Sum(flags?: number): ColorRgb {
    const useFlags = flags ?? BsdfComp.BSDF_ALL_COMPONENTS;
    const result = new ColorRgb();
    result.clear();

    for (let i = 0; i < BsdfComp.BSDF_COMPONENTS; i++) {
      if ((useFlags & BsdfComponentFlag.bsdfIndexToComp(i)) !== 0) {
        result.add(result, this.comp[i]);
      }
    }

    return result;
  }
}
