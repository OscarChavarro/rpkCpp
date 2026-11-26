#ifndef __SHAFT_PLANE__
#define __SHAFT_PLANE__

class ShaftPlane {
  public:
    double n[3];
    double d;
    int coordinateOffset[3]; // Coordinate offset for nearest corner in box-plane tests
};

#endif
