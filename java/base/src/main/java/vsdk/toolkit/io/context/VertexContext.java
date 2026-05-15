package vsdk.toolkit.io.context;

import vsdk.toolkit.common.linealAlgebra.Vector3Dd;
import vsdk.toolkit.environment.geometry.elements.Vertex;

public class VertexContext {
    public Vector3Dd p; // Point
    public Vector3Dd n; // Normal
    public long xid; // Transform id of transform last time the vertex was modified (or created)
    public int clock; // Incremented each change -- resettable
    public Vertex vertex;

    public VertexContext() {
        p = new Vector3Dd();
        n = new Vector3Dd();
        xid = 0;
        clock = 0;
        vertex = null;
    }

    public VertexContext(Vector3Dd inP, Vector3Dd inN, long inXid, int inClock, Vertex inVertex) {
        p = new Vector3Dd();
        n = new Vector3Dd();
        xid = 0;
        clock = 0;
        vertex = null;
        p.copy(inP);
        n.copy(inN);
        xid = inXid;
        clock = inClock;
        vertex = inVertex;
    }

    public void copy(VertexContext source) {
        if (source == null) {
            return;
        }
        p.copy(source.p);
        n.copy(source.n);
        xid = source.xid;
        clock = source.clock;
        vertex = source.vertex;
    }
}
