#ifndef __SPATIAL_QUERY_RESULT__
#define __SPATIAL_QUERY_RESULT__

#include "vsdk/toolkit/common/VSDK.h"

/**
Answer of a `SpatialIndex` query: the accepted primitive and the ray
parameter of its hit, or no primitive (`NO_PRIMITIVE`). An immutable value.
*/
class SpatialQueryResult {
  public:
    enum { NO_PRIMITIVE = -1 };

  private:
    int primitive;
    double parameter;

  public:
    /** No accepted hit */
    SpatialQueryResult();
    SpatialQueryResult(int inPrimitive, double inParameter);

    BOOLEAN hasHit() const;
    int getPrimitive() const;
    /** Ray parameter of the accepted hit; meaningless without a hit */
    double getParameter() const;
};

inline
SpatialQueryResult::SpatialQueryResult() : primitive(NO_PRIMITIVE), parameter(0.0)
{
}

inline
SpatialQueryResult::SpatialQueryResult(const int inPrimitive, const double inParameter) :
    primitive(inPrimitive), parameter(inParameter)
{
}

inline BOOLEAN
SpatialQueryResult::hasHit() const
{
    return primitive != NO_PRIMITIVE;
}

inline int
SpatialQueryResult::getPrimitive() const
{
    return primitive;
}

inline double
SpatialQueryResult::getParameter() const
{
    return parameter;
}

#endif
