package vsdk.toolkit.scene;

import vsdk.toolkit.environment.geometry.elements.Element;

/**
Top-level radiance element of every face of a scene, indexed by the dense face
index (Patch::getFaceIndex()). The current radiance method creates one element
per face when it is selected (Radiance::setRadianceMethod) and destroys them
when it is replaced; the slots are null otherwise. The table only binds faces
to elements: the elements' contents are the solver's state. Owned by the
scene; the elements are owned by the radiance method that created them.
*/
public class RadianceElementTable {
    private final Element[] elements;
    private final int numberOfFaces;

    public RadianceElementTable(int inNumberOfFaces) {
        numberOfFaces = inNumberOfFaces;
        elements = new Element[numberOfFaces > 0 ? numberOfFaces : 1];
    }

    public int getNumberOfFaces() {
        return numberOfFaces;
    }

    public Element get(int faceIndex) {
        return elements[faceIndex];
    }

    public void set(int faceIndex, Element element) {
        elements[faceIndex] = element;
    }
}
