package vsdk.toolkit.environment.geometry.volume;
import java.io.Serial;

import vsdk.toolkit.common.linealAlgebra.Vector3Dd;

public abstract class Solid extends Volume {
    @SuppressWarnings("FieldNameHidesFieldInSuperclass")
    @Serial private static final long serialVersionUID = 20150218L;
    
    /**
    Given current solid, the method `doCenterOfMass` returns a vector
    containing the center of mass for current solid, assuming that all the
    solid interior is filled with a material of constant density.

    This method should be overwritten and defined for every solid

    @return a new Vector3Dd containing the coordinate of current solid
    center of mass.
    */
    public Vector3Dd doCenterOfMass() {
        return new Vector3Dd(0, 0, 0);
    }
}
