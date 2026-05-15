import { ColorRgb } from "../../common/color/ColorRgb";
import { CircularList } from "../../common/dataStructures/CircularList";
import { CircularListIterator } from "../../common/dataStructures/CircularListIterator";
import { BiPath } from "./BiPath";
import { Spar } from "./Spar";
import { SparConfig } from "./SparConfig";

export class SparList extends CircularList<Spar> {
  public handlePath(config: SparConfig, path: BiPath, fRad: ColorRgb, fBpt: ColorRgb): void {
    const iter = new CircularListIterator<Spar>(this);

    fBpt.clear();
    fRad.clear();

    let spar: Spar | null;
    while ((spar = iter.nextOnSequence()) !== null) {
      const col = spar.handlePath(config, path);

      if (spar === config.leSpar) {
        fBpt.add(col, fBpt);
      }
      else {
        fRad.add(col, fRad);
      }
    }
  }
}
