#ifndef __PLYHDRL_BNDD_SLD_EDG__
#define __PLYHDRL_BNDD_SLD_EDG__

#include "java/lang/String.h"
#include "vsdk/toolkit/common/dataStructures/ArrayLinkable.h"

#include "vsdk/toolkit/common/color/ColorRgb.h"
class _PolyhedralBoundedSolidHalfEdge;

class _PolyhedralBoundedSolidEdge : public ArrayLinkable {
public:
    _PolyhedralBoundedSolidHalfEdge* rightHalf;
    _PolyhedralBoundedSolidHalfEdge* leftHalf;
    int id;
    ColorRgb debugColor;

    _PolyhedralBoundedSolidEdge();
    int getEndingVertexId() const;
    int getStartingVertexId() const;

    /**
    @return a description of the node, as Java `toString`
    */
    String toString() const;
};

#endif
