import { LookUpBehaviors } from "../../common/dataStructures/LookUpBehaviors";
import { LookUpTable } from "../../common/dataStructures/LookUpTable";
import { Vector3Dd } from "../../common/linealAlgebra/Vector3Dd";
import { VertexContext } from "./VertexContext";

export class VertexRegistryContext {
  public vertexLookUpTable: LookUpTable<VertexContext> | null;
  public defaultVertexContext: VertexContext;
  public unNamedVertexContext: VertexContext;
  public currentVertex: VertexContext | null;

  private static readonly ZERO_VECTOR = new Vector3Dd(0.0, 0.0, 0.0);

  public constructor() {
    this.vertexLookUpTable = new LookUpTable<VertexContext>(LookUpBehaviors.OWNING);
    this.defaultVertexContext = new VertexContext(VertexRegistryContext.ZERO_VECTOR, VertexRegistryContext.ZERO_VECTOR, 0, 1, null);
    this.unNamedVertexContext = new VertexContext();
    this.unNamedVertexContext.copy(this.defaultVertexContext);
    this.currentVertex = this.unNamedVertexContext;
  }

  public destroy(): void {
    if (this.vertexLookUpTable !== null) {
      this.vertexLookUpTable.lookUpDone();
      this.vertexLookUpTable = null;
    }
    this.currentVertex = null;
  }

  public reset(): void {
    this.unNamedVertexContext.copy(this.defaultVertexContext);
    this.currentVertex = this.unNamedVertexContext;
    (this.vertexLookUpTable as LookUpTable<VertexContext>).lookUpDone();
  }
}
