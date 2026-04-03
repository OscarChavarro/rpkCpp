package vsdk.toolkit.io.context;

import vsdk.toolkit.common.dataStructures.LookUpBehaviors;
import vsdk.toolkit.common.dataStructures.LookUpTable;
import vsdk.toolkit.common.linealAlgebra.Vector3Dd;

public class VertexRegistryContext {
    public LookUpTable<VertexContext> vertexLookUpTable;
    public VertexContext defaultVertexContext;
    public VertexContext unNamedVertexContext;
    public VertexContext currentVertex;

    private static final Vector3Dd ZERO_VECTOR = new Vector3Dd(0.0, 0.0, 0.0);

    public VertexRegistryContext() {
        vertexLookUpTable = new LookUpTable<>(LookUpBehaviors.OWNING);
        defaultVertexContext = new VertexContext(ZERO_VECTOR, ZERO_VECTOR, 0, 1, null);
        unNamedVertexContext = new VertexContext();
        unNamedVertexContext.copy(defaultVertexContext);
        currentVertex = unNamedVertexContext;
    }

    public void destroy() {
        if (vertexLookUpTable != null) {
            vertexLookUpTable.lookUpDone();
            vertexLookUpTable = null;
        }
        currentVertex = null;
    }

    public void reset() {
        unNamedVertexContext.copy(defaultVertexContext);
        currentVertex = unNamedVertexContext;
        vertexLookUpTable.lookUpDone();
    }
}
