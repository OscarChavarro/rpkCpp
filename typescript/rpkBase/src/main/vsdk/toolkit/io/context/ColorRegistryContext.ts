import { LookUpBehaviors } from "../../common/dataStructures/LookUpBehaviors";
import { LookUpTable } from "../../common/dataStructures/LookUpTable";
import { ColorContext } from "./ColorContext";

export class ColorRegistryContext {
  public colorLookUpTable: LookUpTable<ColorContext> | null;
  public unNamedColorContext: ColorContext | null;
  public currentColor: ColorContext | null;

  public constructor() {
    this.colorLookUpTable = new LookUpTable<ColorContext>(LookUpBehaviors.OWNING);
    this.unNamedColorContext = new ColorContext();
    this.currentColor = this.unNamedColorContext;
    this.unNamedColorContext.copy(ColorContext.DEFAULT_COLOR_CONTEXT);
  }

  public destroy(): void {
    this.unNamedColorContext = null;
    if (this.colorLookUpTable !== null) {
      this.colorLookUpTable.lookUpDone();
      this.colorLookUpTable = null;
    }
    this.currentColor = null;
  }

  public reset(): void {
    (this.unNamedColorContext as ColorContext).copy(ColorContext.DEFAULT_COLOR_CONTEXT);
    this.currentColor = this.unNamedColorContext;
    (this.colorLookUpTable as LookUpTable<ColorContext>).lookUpDone();
  }
}
