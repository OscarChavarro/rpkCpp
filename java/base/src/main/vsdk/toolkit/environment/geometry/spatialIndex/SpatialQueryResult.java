package vsdk.toolkit.environment.geometry.spatialIndex;

/**
Answer of a `SpatialIndex` query: the accepted primitive and the ray
parameter of its hit, or no primitive (`NO_PRIMITIVE`). An immutable value.
*/
public final class SpatialQueryResult {
    public static final int NO_PRIMITIVE = -1;

    private final int primitive;
    private final double parameter;

    /** No accepted hit */
    public SpatialQueryResult()
    {
        primitive = NO_PRIMITIVE;
        parameter = 0.0;
    }

    public SpatialQueryResult(int inPrimitive, double inParameter)
    {
        primitive = inPrimitive;
        parameter = inParameter;
    }

    public boolean hasHit()
    {
        return primitive != NO_PRIMITIVE;
    }

    public int getPrimitive()
    {
        return primitive;
    }

    /** Ray parameter of the accepted hit; meaningless without a hit */
    public double getParameter()
    {
        return parameter;
    }
}
