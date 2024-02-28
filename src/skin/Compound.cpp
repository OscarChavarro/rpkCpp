#include "java/util/ArrayList.txx"
#include "material/statistics.h"
#include "skin/Compound.h"
#include "skin/Geometry.h"

/**
Creates a Compound from a list of geometries

Actually, it just counts the number of compounds in the scene and
returns the geometry list
*/
Compound::Compound(java::ArrayList<Geometry *> *geometryList) {
    GLOBAL_statistics_numberOfCompounds++;
    children = geometryList;
}

Compound::~Compound() {
    if ( children != nullptr ) {
        delete children;
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
    RayHit *hitStore)
{
    RayHit * result = geometryListDiscretizationIntersect(
            children, ray, minimumDistance, maximumDistance, hitFlags, hitStore);
    return result;
}
