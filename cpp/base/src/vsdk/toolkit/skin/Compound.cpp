#include "vsdk/toolkit/java/util/ArrayList.txx"
#include "vsdk/toolkit/common/statistics/Statistics.h"
#include "vsdk/toolkit/skin/Compound.h"

/**
Creates a Compound from a list of geometries

Actually, it just counts the number of compounds in the scene and
returns the geometry list
*/
Compound::Compound(java::ArrayList<Geometry *> *geometryList): Geometry(GeometryClassId::COMPOUND) {
    Statistics::instance().reader.numberOfCompounds++;
    children = geometryList;
    Geometry::listBounds(children, &boundingBox);
    boundingBox.enlargeTinyBit();
    bounded = true;
}

Compound::~Compound() {
    Statistics::instance().reader.numberOfCompounds--;
    if ( children != nullptr ) {
        delete children;
        children = nullptr;
    }
}

/**
DiscretizationIntersect returns nullptr is the ray doesn't hit the discretization
of the object. If the ray hits the object, a hit record is returned containing
information about the intersection point. See geometry.h for more explanation
*/
RayHit *
Compound::discretizationIntersect(
    Ray *ray,
    float minimumDistance,
    float *maximumDistance,
    int hitFlags,
    RayHit *hitStore) const
{
    if ( !discretizationIntersectPreTest(ray, minimumDistance, maximumDistance) ) {
        return nullptr;
    }

    return Geometry::listDiscretizationIntersect(
            children, ray, minimumDistance, maximumDistance, hitFlags, hitStore);
}
