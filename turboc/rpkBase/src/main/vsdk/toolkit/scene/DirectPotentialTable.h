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
    double *potentials;
    const int numberOfFaces;

    // Not copyable (deleted in the cpp port)
    DirectPotentialTable(const DirectPotentialTable &);
    DirectPotentialTable &operator=(const DirectPotentialTable &);

  public:
    explicit DirectPotentialTable(int inNumberOfFaces);
    ~DirectPotentialTable();

    int getNumberOfFaces() const;
    double get(int faceIndex) const;
    void set(int faceIndex, double potential);
};

inline int
DirectPotentialTable::getNumberOfFaces() const {
    return numberOfFaces;
}

inline double
DirectPotentialTable::get(const int faceIndex) const {
    return potentials[faceIndex];
}

inline void
DirectPotentialTable::set(const int faceIndex, const double potential) {
    potentials[faceIndex] = potential;
}

#endif
