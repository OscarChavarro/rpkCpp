#ifndef RADIANCE_ELEMENT_TABLE__
#define RADIANCE_ELEMENT_TABLE__

class Element;

/**
Top-level radiance element of every face of a scene, indexed by the dense face
index (Patch::getFaceIndex()). The current radiance method creates one element
per face when it is selected (Radiance::setRadianceMethod) and destroys them
when it is replaced; the slots are null otherwise. The table only binds faces
to elements: the elements' contents are the solver's state. Owned by the
scene; the elements are owned by the radiance method that created them.
*/
class RadianceElementTable {
  private:
    Element **elements;
    const int numberOfFaces;

  public:
    explicit RadianceElementTable(int inNumberOfFaces);
    ~RadianceElementTable();
    RadianceElementTable(const RadianceElementTable &) = delete;
    RadianceElementTable &operator=(const RadianceElementTable &) = delete;

    int getNumberOfFaces() const;
    Element *get(int faceIndex) const;
    void set(int faceIndex, Element *element);
};

inline int
RadianceElementTable::getNumberOfFaces() const {
    return numberOfFaces;
}

inline Element *
RadianceElementTable::get(const int faceIndex) const {
    return elements[faceIndex];
}

inline void
RadianceElementTable::set(const int faceIndex, Element *element) {
    elements[faceIndex] = element;
}

#endif
