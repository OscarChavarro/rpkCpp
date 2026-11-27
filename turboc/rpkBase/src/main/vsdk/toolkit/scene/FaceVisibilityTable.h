#ifndef __FACE_VISIBILITY_TABLE__
#define __FACE_VISIBILITY_TABLE__

/**
Whether each face of a scene is directly visible from the camera, indexed by
the dense face index (Patch::getFaceIndex()). Every face starts invisible;
Potential::updateDirectVisibility rewrites the whole table from an ID render
of the current view, and the importance-driven stochastic radiosity reads it
as source importance. Owned by the scene.
*/
class FaceVisibilityTable {
  private:
    bool *visible;
    const int numberOfFaces;

    // Not copyable (deleted in the cpp port)
    FaceVisibilityTable(const FaceVisibilityTable &);
    FaceVisibilityTable &operator=(const FaceVisibilityTable &);

  public:
    explicit FaceVisibilityTable(int inNumberOfFaces);
    ~FaceVisibilityTable();

    int getNumberOfFaces() const;
    bool isVisible(int faceIndex) const;
    void setVisible(int faceIndex, bool isFaceVisible);
};

inline int
FaceVisibilityTable::getNumberOfFaces() const {
    return numberOfFaces;
}

inline bool
FaceVisibilityTable::isVisible(const int faceIndex) const {
    return visible[faceIndex];
}

inline void
FaceVisibilityTable::setVisible(const int faceIndex, const bool isFaceVisible) {
    visible[faceIndex] = isFaceVisible;
}

#endif
