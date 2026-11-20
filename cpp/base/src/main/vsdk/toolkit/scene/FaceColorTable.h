#ifndef FACE_COLOR_TABLE__
#define FACE_COLOR_TABLE__

#include "vsdk/toolkit/common/color/ColorRgbMutable.h"

/**
Display color of every face of a scene, indexed by the dense face index
(Patch::getFaceIndex()). Every face starts black; the radiance methods
rewrite a face's color from its radiance after each step, and the vertex
colors (Gouraud display, VRML export) and the flat OpenGL display read it.
Owned by the scene.
*/
class FaceColorTable {
  private:
    ColorRgbMutable *colors;
    const int numberOfFaces;

  public:
    explicit FaceColorTable(int inNumberOfFaces);
    ~FaceColorTable();
    FaceColorTable(const FaceColorTable &) = delete;
    FaceColorTable &operator=(const FaceColorTable &) = delete;

    int getNumberOfFaces() const;
    const ColorRgbMutable &get(int faceIndex) const;
    void set(int faceIndex, const ColorRgbMutable &color);
};

inline int
FaceColorTable::getNumberOfFaces() const {
    return numberOfFaces;
}

inline const ColorRgbMutable &
FaceColorTable::get(const int faceIndex) const {
    return colors[faceIndex];
}

inline void
FaceColorTable::set(const int faceIndex, const ColorRgbMutable &color) {
    colors[faceIndex] = color;
}

#endif
