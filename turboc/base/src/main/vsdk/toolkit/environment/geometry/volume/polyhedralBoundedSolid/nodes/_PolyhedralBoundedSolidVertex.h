#ifndef __PLYHDRL_BNDD_SLD_VRTX__
#define __PLYHDRL_BNDD_SLD_VRTX__

#include "java/lang/String.h"
#include "vsdk/toolkit/common/dataStructures/ArrayLinkable.h"

#include "vsdk/toolkit/common/color/ColorRgb.h"
#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"
class _PolyhedralBoundedSolidHalfEdge;

class _PolyhedralBoundedSolidVertex : public ArrayLinkable {
public:
    int id;
    Vector3Dd position;
    _PolyhedralBoundedSolidHalfEdge* emanatingHalfEdge;
    ColorRgb debugColor;

    _PolyhedralBoundedSolidVertex(const Vector3Dd& position, int id)
        : id(id), position(position), emanatingHalfEdge(0), debugColor(1, 0, 0) {}

    /**
    @return a description of the node, as Java `toString`
    */
    String toString() const;
};

#endif
