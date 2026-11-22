//= References:                                                             =
//= [WALD2007] Wald, Ingo. "On fast Construction of SAH-based Bounding      =
//=     Volume Hierarchies". Proceedings of the 2007 IEEE Symposium on      =
//=     Interactive Ray Tracing, 2007, pp. 33-40.                           =

package vsdk.toolkit.environment.geometry.boundingVolumeHierarchy;

/**
Axis aligned box of the build. The default box is the "negative box"
[+inf, -inf] of [WALD2007].3.4: enclosing it in another box leaves that box
unchanged, so bins and partitions grow without emptiness checks.
*/
class _BinnedSahBox {
    final double[] minimum = new double[3];
    final double[] maximum = new double[3];

    _BinnedSahBox()
    {
        for ( int a = 0; a < 3; a++ ) {
            minimum[a] = 1e30;
            maximum[a] = -1e30;
        }
    }

    boolean isEmpty()
    {
        return minimum[0] > maximum[0] || minimum[1] > maximum[1] || minimum[2] > maximum[2];
    }

    void enclose(_BinnedSahBox other)
    {
        for ( int a = 0; a < 3; a++ ) {
            minimum[a] = other.minimum[a] < minimum[a] ? other.minimum[a] : minimum[a];
            maximum[a] = other.maximum[a] > maximum[a] ? other.maximum[a] : maximum[a];
        }
    }

    void enclosePoint(double[] point)
    {
        for ( int a = 0; a < 3; a++ ) {
            minimum[a] = point[a] < minimum[a] ? point[a] : minimum[a];
            maximum[a] = point[a] > maximum[a] ? point[a] : maximum[a];
        }
    }

    double centroid(int axis)
    {
        return 0.5 * (minimum[axis] + maximum[axis]);
    }

    double extent(int axis)
    {
        return maximum[axis] - minimum[axis];
    }

    /** Half the surface area: the SAH cost of [WALD2007].3.1 drops the
    constant factors and only compares ratios */
    double halfArea()
    {
        if ( isEmpty() ) {
            return 0.0;
        }
        final double dx = maximum[0] - minimum[0];
        final double dy = maximum[1] - minimum[1];
        final double dz = maximum[2] - minimum[2];
        return dx * dy + dy * dz + dz * dx;
    }
}
