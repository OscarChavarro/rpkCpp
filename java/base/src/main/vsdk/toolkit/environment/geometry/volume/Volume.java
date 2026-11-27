package vsdk.toolkit.environment.geometry.volume;
import java.io.Serial;

import vsdk.toolkit.environment.geometry.Geometry;
import vsdk.toolkit.environment.geometry.volume.polyhedralBoundedSolid.PolyhedralBoundedSolid;

public abstract class Volume extends Geometry {
    @SuppressWarnings("FieldNameHidesFieldInSuperclass")
    @Serial private static final long serialVersionUID = 20260527L;

    /**
    The design of this method could change in future.
    @return a new PolyhedralBoundedSolid representing the surface of current
    Geometry when possible, null if not possible
    */
    public PolyhedralBoundedSolid exportToPolyhedralBoundedSolid()
    {
        return null;
    }
}
