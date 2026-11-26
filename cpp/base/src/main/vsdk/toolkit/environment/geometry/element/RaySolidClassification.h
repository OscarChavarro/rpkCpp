#ifndef __RAY_SOLID_CLASSIFICATION__
#define __RAY_SOLID_CLASSIFICATION__

#include "java/util/ArrayList.h"
#include "vsdk/toolkit/environment/geometry/element/RaySolidBoundary.h"

// For N ordered boundaries there are N+1 segment membership values. Endpoint
// events are included; a zero-length endpoint segment has the limiting state
// just outside the query interval. Point tangencies do not create solid segments.
// On failure both arrays are empty. Clear retains storage for query/worker reuse.
class RaySolidClassification {
public:
    java::ArrayList<RaySolidBoundary> boundaries;
    java::ArrayList<bool> insideSegments;

    RaySolidClassification();
    ~RaySolidClassification();
    RaySolidClassification(const RaySolidClassification& other);
    RaySolidClassification& operator=(const RaySolidClassification& other);
    void clear();
};

#endif
