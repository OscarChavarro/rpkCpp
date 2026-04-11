import { EntityTypeContext } from "../context/EntityTypeContext";
import { ParseErrorContext } from "../context/ParseErrorContext";
import { ParseRuntimeContext } from "../context/ParseRuntimeContext";
import { ReaderContext } from "../context/ReaderContext";
import { MgfEntityControl } from "./MgfEntityControl";

export class MgfFaceWithHolesEntityExpander {
  private constructor() {
  }

  /**
  Replace face + holes with single contour
  */
  public static handleEntity(argumentCount: number, argumentValues: string[], context: ParseRuntimeContext): number {
    const newArgumentValues = new Array<string>(ReaderContext.MGF_MAXIMUM_ARGUMENT_COUNT);
    let lastPerimeterIndex = 0;

    newArgumentValues[0] = context.entityNames[EntityTypeContext.FACE];
    let i = 1;
    for (; i < argumentCount; i++) {
      if (argumentValues[i].charAt(0) === "-") {
        if (i < 4) {
          return ParseErrorContext.MGF_ERROR_WRONG_NUMBER_OF_ARGUMENTS;
        }
        if (i >= argumentCount - 1) {
          break;
        }
        if (lastPerimeterIndex === 0) {
          lastPerimeterIndex = i - 1;
        }
        let j = i + 1;
        for (; j < argumentCount - 1 && argumentValues[j + 1].charAt(0) !== "-"; j++) {
        }
        if (j - i < 3) {
          return ParseErrorContext.MGF_ERROR_WRONG_NUMBER_OF_ARGUMENTS;
        }
        // Connect hole loop
        newArgumentValues[i] = argumentValues[j];
      }
      else {
        // Hole or perimeter vertex
        newArgumentValues[i] = argumentValues[i];
      }
    }
    if (lastPerimeterIndex !== 0) {
      // Finish seam to outside
      newArgumentValues[i++] = argumentValues[lastPerimeterIndex];
    }
    return MgfEntityControl.mgfHandle(EntityTypeContext.FACE, i, newArgumentValues, context);
  }
}
