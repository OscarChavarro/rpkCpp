import { Material } from "../../material/Material";

export class MaterialSelectionContext {
  public currentMaterial: Material | null;
  public currentMaterialName: string | null;
  public materials: Material[] | null;

  public constructor() {
    this.currentMaterial = null;
    this.currentMaterialName = null;
    this.materials = null;
  }
}
