package vsdk.toolkit.environment.geometry.volume;

import vsdk.toolkit.common.linealAlgebra.Matrix4x4d;
import vsdk.toolkit.common.linealAlgebra.Vector3Dd;
import vsdk.toolkit.common.numerics.FloatingInterval;
import vsdk.toolkit.environment.geometry.element.GeometryContainment;
import vsdk.toolkit.environment.geometry.element.GeometryDistanceStatus;

/**
A certified enclosure for distance to the surface of an affine sphere.
The lower bound uses a supporting plane or an interior inverse-norm bound;
the upper bound uses a surface point in a nonzero local direction. Side may
be Indeterminate. Outputs change only on Valid.
Output arguments are one-element holders.
*/
public class AffineEllipsoidDistanceBounds {
    public static GeometryDistanceStatus query(Vector3Dd point,
        double radius, Matrix4x4d forward, Vector3Dd localDirection,
        GeometryContainment[] side, double[] lower, double[] upper)
    {
        return query(point, radius, forward, localDirection, side, lower, upper, true);
    }

    public static GeometryDistanceStatus query(Vector3Dd point,
        double radius, Matrix4x4d forward, Vector3Dd localDirection,
        GeometryContainment[] side, double[] lower, double[] upper, boolean refineInterior)
    {
        if (!Double.isFinite(radius) || !(radius>0) ||
            !Double.isFinite(point.x()) || !Double.isFinite(point.y()) ||
            !Double.isFinite(point.z()) || !Double.isFinite(localDirection.x()) ||
            !Double.isFinite(localDirection.y()) || !Double.isFinite(localDirection.z()))
            return GeometryDistanceStatus.InvalidInput;
        for (int i=0;i<4;++i) for (int j=0;j<4;++j)
            if (!Double.isFinite(forward.get(i,j))) return GeometryDistanceStatus.InvalidInput;
        if (forward.get(3,0)!=0 || forward.get(3,1)!=0 || forward.get(3,2)!=0 || forward.get(3,3)!=1 ||
            (localDirection.x()==0 && localDirection.y()==0 && localDirection.z()==0))
            return GeometryDistanceStatus.InvalidInput;
        if (!FloatingInterval.isSupportedEnvironment()) return GeometryDistanceStatus.Unsupported;
        for (int i=0;i<3;++i) {
            if ((forward.get(i,0)==0 && forward.get(i,1)==0 && forward.get(i,2)==0) ||
                (forward.get(0,i)==0 && forward.get(1,i)==0 && forward.get(2,i)==0))
                return GeometryDistanceStatus.InvalidInput;
        }
        final FloatingInterval a=FloatingInterval.exact(forward.get(0,0));
        final FloatingInterval b=FloatingInterval.exact(forward.get(0,1));
        final FloatingInterval c=FloatingInterval.exact(forward.get(0,2));
        final FloatingInterval d0=FloatingInterval.exact(forward.get(1,0));
        final FloatingInterval e=FloatingInterval.exact(forward.get(1,1));
        final FloatingInterval f=FloatingInterval.exact(forward.get(1,2));
        final FloatingInterval g=FloatingInterval.exact(forward.get(2,0));
        final FloatingInterval h=FloatingInterval.exact(forward.get(2,1));
        final FloatingInterval i22=FloatingInterval.exact(forward.get(2,2)); // cpp name: i
        final FloatingInterval determinant=a.multiply(e.multiply(i22).subtract(f.multiply(h)))
            .subtract(b.multiply(d0.multiply(i22).subtract(f.multiply(g))))
            .add(c.multiply(d0.multiply(h).subtract(e.multiply(g))));
        if (!determinant.isValid() || (determinant.getLower()<=0 && determinant.getUpper()>=0))
            return GeometryDistanceStatus.Indeterminate;
        final double[] p={point.x(),point.y(),point.z()};
        final double[] d={localDirection.x(),localDirection.y(),localDirection.z()};
        FloatingInterval lengthSquared=FloatingInterval.exact(0);
        for (int i=0;i<3;++i) lengthSquared=lengthSquared.add(FloatingInterval.exact(d[i]).square());
        final FloatingInterval length=lengthSquared.squareRoot();
        if (!length.isValid() || !(length.getLower()>0)) return GeometryDistanceStatus.Indeterminate;
        FloatingInterval[] unit=new FloatingInterval[3];
        for (int i=0;i<3;++i) unit[i]=FloatingInterval.exact(d[i]).divide(length)
            .multiply(FloatingInterval.exact(radius));
        FloatingInterval distanceSquared=FloatingInterval.exact(0);
        for (int i=0;i<3;++i) {
            FloatingInterval coordinate=FloatingInterval.exact(forward.get(i,3));
            for (int j=0;j<3;++j) coordinate=coordinate.add(FloatingInterval.exact(forward.get(i,j)).multiply(unit[j]));
            final FloatingInterval delta=coordinate.subtract(FloatingInterval.exact(p[i]));
            distanceSquared=distanceSquared.add(delta.square());
        }
        final FloatingInterval distance=distanceSquared.squareRoot();
        if (!distance.isValid()) return GeometryDistanceStatus.Indeterminate;
        // Any finite nonzero vector n defines a supporting plane. Its distance
        // from the query is a lower bound whenever its signed gap is positive.
        double[][] normals={{0,0,0},{0,0,0}};
        boolean[] usable={false,false};
        for (int i=0;i<3;++i) {
            normals[0][i]=p[i]-forward.get(i,3);
            if (!Double.isFinite(normals[0][i])) return GeometryDistanceStatus.Indeterminate;
            if (normals[0][i]!=0) usable[0]=true;
        }
        double lowerBound=0;
        GeometryContainment certifiedSide=GeometryContainment.Indeterminate;
        // Enclose A^-1 from cofactors of the stored forward matrix itself; the
        // independently stored approximate inverse is not treated as exact.
        FloatingInterval[][] inverse=new FloatingInterval[3][3];
        inverse[0][0]=e.multiply(i22).subtract(f.multiply(h)).divide(determinant);
        inverse[0][1]=c.multiply(h).subtract(b.multiply(i22)).divide(determinant);
        inverse[0][2]=b.multiply(f).subtract(c.multiply(e)).divide(determinant);
        inverse[1][0]=f.multiply(g).subtract(d0.multiply(i22)).divide(determinant);
        inverse[1][1]=a.multiply(i22).subtract(c.multiply(g)).divide(determinant);
        inverse[1][2]=c.multiply(d0).subtract(a.multiply(f)).divide(determinant);
        inverse[2][0]=d0.multiply(h).subtract(e.multiply(g)).divide(determinant);
        inverse[2][1]=b.multiply(g).subtract(a.multiply(h)).divide(determinant);
        inverse[2][2]=a.multiply(e).subtract(b.multiply(d0)).divide(determinant);
        boolean inverseCandidateValid=true;
        for (int row=0;row<3;++row) for (int col=0;col<3;++col) {
            if (!inverse[row][col].isValid()) { inverseCandidateValid=false; continue; }
            // This rounded midpoint is only a choice of plane direction: any
            // finite direction is valid for the interval-verified support test.
            final double midpoint=0.5*inverse[row][col].getLower()+0.5*inverse[row][col].getUpper();
            normals[1][col]+=midpoint*d[row];
        }
        for (int j=0;j<3;++j) {
            if (!Double.isFinite(normals[1][j])) inverseCandidateValid=false;
            if (normals[1][j]!=0) usable[1]=true;
        }
        usable[1]=usable[1] && inverseCandidateValid;
        FloatingInterval localSquared=FloatingInterval.exact(0);
        FloatingInterval inverseSquared=FloatingInterval.exact(0);
        FloatingInterval[] local=new FloatingInterval[3];
        for (int row=0;row<3;++row) {
            FloatingInterval component=FloatingInterval.exact(0);
            for (int col=0;col<3;++col) {
                component=component.add(inverse[row][col].multiply(FloatingInterval.exact(p[col])
                    .subtract(FloatingInterval.exact(forward.get(col,3)))));
                inverseSquared=inverseSquared.add(inverse[row][col].square());
            }
            local[row]=component;
            localSquared=localSquared.add(component.square());
        }
        final FloatingInterval localLength=localSquared.squareRoot();
        final FloatingInterval inverseNorm=inverseSquared.squareRoot();
        if (localLength.isValid() && inverseNorm.isValid() && inverseNorm.getLower()>0) {
            if (localLength.getUpper()<radius) {
                certifiedSide=GeometryContainment.Inside;
                final FloatingInterval margin=FloatingInterval.exact(radius).subtract(localLength).divide(inverseNorm);
                if (margin.isValid() && margin.getLower()>0) lowerBound=margin.getLower();
                // For Q=A^-T A^-1 and g=A^-T x, the exact quadratic obeys
                // q(p+d)<=q(p)+2||g||||d||+||Q||_2||d||^2. Enclose g and Q,
                // bound the symmetric spectral norm by its largest absolute row
                // sum, and solve the scalar quadratic with outward rounding.
                FloatingInterval gradientSquared=FloatingInterval.exact(0);
                FloatingInterval[] gradientComponents=new FloatingInterval[3];
                FloatingInterval[][] quadratic=new FloatingInterval[3][3];
                double maxRow=0;
                boolean quadraticValid=true;
                for (int row=0;row<3;++row) {
                    FloatingInterval gradient=FloatingInterval.exact(0);
                    FloatingInterval rowSum=FloatingInterval.exact(0);
                    for (int k=0;k<3;++k) gradient=gradient.add(inverse[k][row].multiply(local[k]));
                    gradientComponents[row]=gradient;
                    gradientSquared=gradientSquared.add(gradient.square());
                    for (int col=0;col<3;++col) {
                        FloatingInterval element=FloatingInterval.exact(0);
                        for (int k=0;k<3;++k) element=element.add(inverse[k][row].multiply(inverse[k][col]));
                        quadratic[row][col]=element;
                        if (!element.isValid()) { quadraticValid=false; break; }
                        final double maxAbs=Math.max(Math.abs(element.getLower()),
                            Math.abs(element.getUpper()));
                        rowSum=rowSum.add(FloatingInterval.exact(maxAbs));
                    }
                    if (!rowSum.isValid()) quadraticValid=false;
                    else if (rowSum.getUpper()>maxRow) maxRow=rowSum.getUpper();
                }
                final FloatingInterval gradientLength=gradientSquared.squareRoot();
                final FloatingInterval gapSquared=FloatingInterval.exact(radius).square().subtract(localSquared);
                if (quadraticValid && gradientLength.isValid() && gapSquared.isValid() && gapSquared.getLower()>0) {
                    final FloatingInterval gap=FloatingInterval.exact(gapSquared.getLower());
                    final FloatingInterval grad=FloatingInterval.exact(gradientLength.getUpper());
                    final FloatingInterval curvature=FloatingInterval.exact(maxRow);
                    final FloatingInterval denominator=grad.square().add(curvature.multiply(gap)).squareRoot().add(grad);
                    final FloatingInterval root=gap.divide(denominator);
                    if (root.isValid() && root.getLower()>lowerBound) lowerBound=root.getLower();
                    if (refineInterior) {
                        final double dual=QuadraticInteriorBound.improve(radius,localSquared,
                            gradientComponents,quadratic,maxRow);
                        if (dual>lowerBound) lowerBound=dual;
                    }
                }
            } else if (localLength.getLower()>radius) certifiedSide=GeometryContainment.Outside;
        }
        for (int candidateIndex=0;candidateIndex<2;++candidateIndex) {
            if (!usable[candidateIndex]) continue;
            final double[] n=normals[candidateIndex];
            FloatingInterval projection=FloatingInterval.exact(0);
            FloatingInterval normalSquared=FloatingInterval.exact(0);
            FloatingInterval supportSquared=FloatingInterval.exact(0);
            for (int i=0;i<3;++i) {
                final FloatingInterval ni=FloatingInterval.exact(n[i]);
                normalSquared=normalSquared.add(ni.square());
                projection=projection.add(ni.multiply(FloatingInterval.exact(p[i])
                    .subtract(FloatingInterval.exact(forward.get(i,3)))));
                FloatingInterval component=FloatingInterval.exact(0);
                for (int j=0;j<3;++j) component=component.add(FloatingInterval.exact(n[j])
                    .multiply(FloatingInterval.exact(forward.get(j,i))));
                supportSquared=supportSquared.add(component.square());
            }
            final FloatingInterval support=supportSquared.squareRoot().multiply(FloatingInterval.exact(radius));
            final FloatingInterval gap=projection.subtract(support);
            final FloatingInterval normalLength=normalSquared.squareRoot();
            if (!gap.isValid() || !normalLength.isValid() || !(normalLength.getLower()>0))
                return GeometryDistanceStatus.Indeterminate;
            if (gap.getLower()>0) {
                final FloatingInterval gapDistance=gap.divide(normalLength);
                if (!gapDistance.isValid()) return GeometryDistanceStatus.Indeterminate;
                if (certifiedSide==GeometryContainment.Inside) return GeometryDistanceStatus.Indeterminate;
                certifiedSide=GeometryContainment.Outside;
                if (gapDistance.getLower()>lowerBound) lowerBound=gapDistance.getLower();
            }
        }
        final double upperBound=distance.getUpper();
        if (!Double.isFinite(upperBound) || lowerBound>upperBound) return GeometryDistanceStatus.Indeterminate;
        side[0]=certifiedSide;
        lower[0]=lowerBound;
        upper[0]=upperBound;
        return GeometryDistanceStatus.Valid;
}
}
