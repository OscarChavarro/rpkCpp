package vsdk.toolkit.skin;

import vsdk.toolkit.environment.geometry.elements.*;

import java.util.ArrayList;
import vsdk.toolkit.common.linealAlgebra.Ray;
import vsdk.toolkit.common.statistics.Statistics;

public class Compound extends Geometry {
    public ArrayList<Geometry> children;

    /**
    Creates a Compound from a list of geometries.

    Actually, it just counts the number of compounds in the scene and
    returns the geometry list.
    */
    public Compound(ArrayList<Geometry> geometryList) {
        super(GeometryClassId.COMPOUND);
        Statistics.instance().reader.numberOfCompounds++;
        children = geometryList;
        Geometry.listBounds(children, boundingBox);
        boundingBox.enlargeTinyBit();
        bounded = true;
    }

    @Override
    public void destroy() {
        Statistics.instance().reader.numberOfCompounds--;
        if (children != null) {
            children = null;
        }
        super.destroy();
    }

    /**
    DiscretizationIntersect returns null if the ray does not hit the discretization
    of the object. If the ray hits the object, a hit record is returned containing
    information about the intersection point. See geometry.h for more explanation.
    */
    @Override
    public RayHit discretizationIntersect(
        Ray ray,
        float minimumDistance,
        float[] maximumDistance,
        int hitFlags,
        RayHit hitStore) {
        if (!discretizationIntersectPreTest(ray, minimumDistance, maximumDistance)) {
            return null;
        }

        return Geometry.listDiscretizationIntersect(children, ray, minimumDistance, maximumDistance, hitFlags, hitStore);
    }
}
