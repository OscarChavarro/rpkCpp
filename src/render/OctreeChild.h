#ifndef __OPENGL_OCTREE_CHILD__
#define __OPENGL_OCTREE_CHILD__

class Geometry;

class OctreeChild {
  public:
    Geometry *geometry;
    float distance;
};

#endif
