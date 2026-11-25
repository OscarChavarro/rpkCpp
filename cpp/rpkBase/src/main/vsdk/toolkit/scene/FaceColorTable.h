#ifndef __FACE_COLOR_TABLE__
#define __FACE_COLOR_TABLE__

#include "vsdk/toolkit/common/color/ColorRgb.h"

/**
Display color of every face of a scene, indexed by the dense face index
(Patch::getFaceIndex()). Every face starts black; the radiance methods
rewrite a face's color from its radiance after each step, and the vertex
colors (Gouraud display, VRML export) and the flat OpenGL display read it.
Owned by the scene.
*/
class FaceColorTable {
  private:
    ColorRgb *colors;
    const int numberOfFaces;

  public:
    explicit FaceColorTable(int inNumberOfFaces);
    ~FaceColorTable();
    FaceColorTable(const FaceColorTable &) = delete;
    FaceColorTable &operator=(const FaceColorTable &) = delete;

    int getNumberOfFaces() const;
    const ColorRgb &get(int faceIndex) const;
    void set(int faceIndex, const ColorRgb &color);
};

inline int
FaceColorTable::getNumberOfFaces() const {
    return numberOfFaces;
}

inline const ColorRgb &
FaceColorTable::get(const int faceIndex) const {
    return colors[faceIndex];
}

inline void
FaceColorTable::set(const int faceIndex, const ColorRgb &color) {
    colors[faceIndex] = color;
}

#endif
