#ifndef __DIRECT_POTENTIAL_TABLE__
#define __DIRECT_POTENTIAL_TABLE__

/**
Direct potential (importance received straight from the eye) of every face of
a scene, indexed by the dense face index (Patch::getFaceIndex()). All values
start at zero; Potential::updateDirectPotential rewrites them whenever the view
changes, and the importance-driven solvers read them. Owned by the scene.
*/
class DirectPotentialTable {
  private:
    float *potentials;
    const int numberOfFaces;

  public:
    explicit DirectPotentialTable(int inNumberOfFaces);
    ~DirectPotentialTable();
    DirectPotentialTable(const DirectPotentialTable &) = delete;
    DirectPotentialTable &operator=(const DirectPotentialTable &) = delete;

    int getNumberOfFaces() const;
    float get(int faceIndex) const;
    void set(int faceIndex, float potential);
};

inline int
DirectPotentialTable::getNumberOfFaces() const {
    return numberOfFaces;
}

inline float
DirectPotentialTable::get(const int faceIndex) const {
    return potentials[faceIndex];
}

inline void
DirectPotentialTable::set(const int faceIndex, const float potential) {
    potentials[faceIndex] = potential;
}

#endif
