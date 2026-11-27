package vsdk.toolkit.skin;

import vsdk.toolkit.environment.geometry.elements.RayExclusionSet;
import vsdk.toolkit.environment.geometry.elements.RpkRayHit;

import java.util.ArrayList;
import vsdk.toolkit.environment.geometry.element.Ray;
import vsdk.toolkit.common.statistics.Statistics;

public class Compound extends RpkGeometry {
    public ArrayList<RpkGeometry> children;

    /**
    Creates a Compound from a list of geometries.

    Actually, it just counts the number of compounds in the scene and
    returns the geometry list.
    */
    public Compound(ArrayList<RpkGeometry> geometryList) {
        super(GeometryClassId.COMPOUND);
        Statistics.instance().reader.numberOfCompounds++;
        children = geometryList;
        RpkGeometry.listBounds(children, boundingBox);
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
    public RpkRayHit discretizationIntersect(
        Ray ray,
        double minimumDistance,
        double[] maximumDistance,
        int hitFlags,
        RpkRayHit hitStore,
        RayExclusionSet exclusions) {
        if (!discretizationIntersectPreTest(ray, minimumDistance, maximumDistance, exclusions)) {
            return null;
        }

        return RpkGeometry.listDiscretizationIntersect(children, ray, minimumDistance, maximumDistance, hitFlags, hitStore, exclusions);
    }
}
