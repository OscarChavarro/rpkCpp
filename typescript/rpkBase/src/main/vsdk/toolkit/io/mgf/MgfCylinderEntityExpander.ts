import { EntityTypeContext } from "../context/EntityTypeContext";
import { ParseErrorContext } from "../context/ParseErrorContext";
import { ParseRuntimeContext } from "../context/ParseRuntimeContext";
import { MgfEntityControl } from "./MgfEntityControl";

export class MgfCylinderEntityExpander {
  private constructor() {
  }

  /**
  Replace a cylinder with equivalent cone
  */
  public static handleEntity(argumentCount: number, argumentValues: string[], context: ParseRuntimeContext): number {
    const newArgumentValues = new Array<string>(6);
    newArgumentValues[0] = context.entityNames[EntityTypeContext.CONE]!;

    if (argumentCount !== 4) {
      return ParseErrorContext.MGF_ERROR_WRONG_NUMBER_OF_ARGUMENTS;
    }
    newArgumentValues[1] = argumentValues[1]!;
    newArgumentValues[2] = argumentValues[2]!;
    newArgumentValues[3] = argumentValues[3]!;
    newArgumentValues[4] = argumentValues[2]!;
    return MgfEntityControl.mgfHandle(EntityTypeContext.CONE, 5, newArgumentValues as string[], context);
  }
}
