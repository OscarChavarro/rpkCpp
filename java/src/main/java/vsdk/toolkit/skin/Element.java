package vsdk.toolkit.skin;

import java.util.ArrayList;
import vsdk.toolkit.common.ColorRgb;
import vsdk.toolkit.common.Error;
import vsdk.toolkit.common.RenderOptions;
import vsdk.toolkit.common.linealAlgebra.Matrix2x2;

public class Element {
    public interface ElementTraversalCallback {
        void apply(Element element);
    }

    public interface ElementRenderTraversalCallback {
        void apply(Element element, RenderOptions renderOptions);
    }

    public int id; // Unique ID number for the element
    public ColorRgb Ed; // Diffuse emittance radiance
    public ColorRgb Rd; // Reflectance
    public ColorRgb[] radiance; // Total radiance on the element as computed so far
    public ColorRgb[] receivedRadiance; // Radiance received during iteration
    public ColorRgb[] unShotRadiance; // For progressive refinement radiosity
    public float area; // Area of all surfaces contained in the element
    public int className;
    public int flags;

    public Element parent; // Parent element in a hierarchy, or null pointer if there is no parent
    public Element[] regularSubElements; // For surface elements with regular quadtree subdivision
    // A null pointer if there are no regular sub-elements (child element in hierarchy), or a 4-sized
    // array containing with sub-elements. Note both triangles and quads are subdivided in 4.
    public ArrayList<Element> irregularSubElements; // Hierarchy of clusters
    // Relates surface element (u, v) coordinates to patch (u, v) coordinates,
    // if non-null, transforms (u, v) coordinates on a sub-element to the (u, v) coordinates
    // of the same point on the parent surface element. It is null if the element is a
    // toplevel element for a patch or a cluster element. If non-null it is a sub-element on a patch
    public Matrix2x2 transformToParent;

    public Element() {
        id = 0;
        Ed = new ColorRgb();
        Rd = new ColorRgb();
        radiance = null;
        receivedRadiance = null;
        unShotRadiance = null;
        area = 0.0f;
        className = 0;
        flags = 0x00;

        parent = null;
        regularSubElements = null;
        irregularSubElements = null;
        transformToParent = null;

        Ed.clear();
        Rd.clear();
    }

    public boolean isCluster() {
        return (flags & ElementFlags.IS_CLUSTER_MASK) != 0;
    }

    /**
    Computes the transform relating a surface element to the toplevel element
    in the patch hierarchy by concatenating the up-transforms of the element
    and all parent elements. If the element is a toplevel element,
    (Matrix4x4 *)null is
    returned and nothing is filled in in xf (no transform is necessary
    to transform positions on the element to the corresponding point on the toplevel
    element). In the other case, the composed transform is filled in in xf and
    xf (pointer to the transform) is returned.
    */
    public Matrix2x2 topTransform(Matrix2x2 transform) {
        // Top level element: no transform necessary to transform to top
        if (transformToParent == null) {
            return null;
        }

        Element window = this;
        copyMatrix(window.transformToParent, transform);

        do {
            window = window.parent;
            if (window != null && window.transformToParent != null) {
                window.transformToParent.matrix2DPreConcatTransform(transform, transform);
            }
        } while (window != null && window.transformToParent != null);

        return transform;
    }

    /**
    Call traversalCallbackFunction for each leaf element of element.
    */
    public void traverseAllLeafElements(ElementTraversalCallback traversalCallbackFunction) {
        for (int i = 0; irregularSubElements != null && i < irregularSubElements.size(); i++) {
            irregularSubElements.get(i).traverseAllLeafElements(traversalCallbackFunction);
        }

        if (regularSubElements != null) {
            for (int i = 0; i < 4; i++) {
                regularSubElements[i].traverseAllLeafElements(traversalCallbackFunction);
            }
        }

        if (irregularSubElements == null && regularSubElements == null) {
            traversalCallbackFunction.apply(this);
        }
    }

    public void traverseClusterLeafElements(ElementTraversalCallback traversalCallbackFunction) {
        if (isCluster()) {
            for (int i = 0; irregularSubElements != null && i < irregularSubElements.size(); i++) {
                if (irregularSubElements.get(i) != null) {
                    irregularSubElements.get(i).traverseClusterLeafElements(traversalCallbackFunction);
                }
            }
        }
        else if (regularSubElements != null) {
            for (int i = 0; i < 4; i++) {
                if (regularSubElements[i] != null) {
                    regularSubElements[i].traverseClusterLeafElements(traversalCallbackFunction);
                }
            }
        }
        else {
            traversalCallbackFunction.apply(this);
        }
    }

    public void traverseQuadTreeLeafs(ElementRenderTraversalCallback traversalCallbackFunction, RenderOptions renderOptions) {
        if (regularSubElements == null) {
            // Trivial case
            traversalCallbackFunction.apply(this, renderOptions);
        }
        else {
            // Recursive case
            for (int i = 0; i < 4; i++) {
                regularSubElements[i].traverseQuadTreeLeafs(traversalCallbackFunction, renderOptions);
            }
        }
    }

    /**
    Returns true if elem is a leaf element.
    */
    public boolean isLeaf() {
        return regularSubElements == null && (irregularSubElements == null || irregularSubElements.size() == 0);
    }

    public Element childContainingElement(Element descendant) {
        while (descendant != null && descendant.parent != this) {
            descendant = descendant.parent;
        }
        if (descendant == null) {
            Error.fatal(-1, "Element::childContainingElement", "descendant is not a descendant of parent");
        }
        return descendant;
    }

    /**
    Returns true if there are children elements and false if top is null or a leaf element.
    */
    public boolean traverseAllChildren(ElementTraversalCallback traversalCallbackFunction) {
        if (isCluster()) {
            for (int i = 0; irregularSubElements != null && i < irregularSubElements.size(); i++) {
                if (irregularSubElements != null) {
                    traversalCallbackFunction.apply(irregularSubElements.get(i));
                }
            }
            return true;
        }
        else if (regularSubElements != null) {
            for (int i = 0; i < 4; i++) {
                if (regularSubElements[i] != null) {
                    traversalCallbackFunction.apply(regularSubElements[i]);
                }
            }
            return true;
        }
        else {
            // Leaf element
            return false;
        }
    }

    private static void copyMatrix(Matrix2x2 source, Matrix2x2 target) {
        target.m[0][0] = source.m[0][0];
        target.m[0][1] = source.m[0][1];
        target.m[1][0] = source.m[1][0];
        target.m[1][1] = source.m[1][1];
        target.t[0] = source.t[0];
        target.t[1] = source.t[1];
    }
}
