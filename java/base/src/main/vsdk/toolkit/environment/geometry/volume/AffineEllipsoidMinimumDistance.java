package vsdk.toolkit.environment.geometry.volume;

import vsdk.toolkit.common.linealAlgebra.Matrix4x4d;
import vsdk.toolkit.common.linealAlgebra.Vector3Dd;
import vsdk.toolkit.common.numerics.FloatingInterval;
import vsdk.toolkit.environment.geometry.element.GeometryContainment;
import vsdk.toolkit.environment.geometry.element.GeometryDistanceStatus;

/**
Certified enclosure of the minimum world-space surface distance. A valid
result is at most maxWidth wide; otherwise the status is Indeterminate and
all outputs remain unchanged. The inverse is only a numerical seed hint.
Output arguments are one-element holders.
*/
public class AffineEllipsoidMinimumDistance {
    public static GeometryDistanceStatus query(Vector3Dd point,
        double radius, Matrix4x4d forward, Matrix4x4d inverseHint, double maxWidth,
        GeometryContainment[] side, double[] lower, double[] upper)
    {
        if (!Double.isFinite(maxWidth) || !(maxWidth>0)) return GeometryDistanceStatus.InvalidInput;
        for (int i=0;i<4;++i) for (int j=0;j<4;++j)
            if (!Double.isFinite(inverseHint.get(i,j))) return GeometryDistanceStatus.InvalidInput;
        double[] estimate={0},error={0};
        Vector3Dd[] witness={new Vector3Dd()};
        final GeometryDistanceStatus candidate=AffineEllipsoidDistance.queryWithWitness(
            point,radius,forward,estimate,error,witness);
        if (candidate==GeometryDistanceStatus.InvalidInput) return candidate;
        Vector3Dd seed=candidate==GeometryDistanceStatus.Valid ? inverseHint.multiply(witness[0]) : inverseHint.multiply(point);
        if (!Double.isFinite(seed.x()) || !Double.isFinite(seed.y()) ||
            !Double.isFinite(seed.z())) return GeometryDistanceStatus.Indeterminate;
        if (seed.x()==0 && seed.y()==0 && seed.z()==0) seed=new Vector3Dd(1,0,0);
        GeometryContainment[] candidateSide={GeometryContainment.Indeterminate};
        double[] lo={0},hi={0};
        final GeometryDistanceStatus status=AffineEllipsoidDistanceBounds.query(
            point,radius,forward,seed,candidateSide,lo,hi);
        if (status!=GeometryDistanceStatus.Valid) return status;
        final double dual=AffineEllipsoidDualBound.lowerDistance(point,radius,forward,seed);
        if (dual>lo[0]) lo[0]=dual;
        if (lo[0]>hi[0]) return GeometryDistanceStatus.Indeterminate;
        final FloatingInterval width=FloatingInterval.exact(hi[0]).subtract(FloatingInterval.exact(lo[0]));
        if (!width.isValid() || width.getUpper()>maxWidth) return GeometryDistanceStatus.Indeterminate;
        side[0]=candidateSide[0];
        lower[0]=lo[0];
        upper[0]=hi[0];
        return GeometryDistanceStatus.Valid;
    }
}
