//= References:                                                             =
//= [MANT1988] Mantyla Martti. "An Introduction To Solid Modeling",         =
//=     Computer Science Press, 1988.                                       =

package vsdk.toolkit.environment.geometry.volume.polyhedralBoundedSolid.nodes;
import java.io.Serial;

import vsdk.toolkit.common.FundamentalEntity;
import vsdk.toolkit.common.dataStructures.ArrayLinkable;
import vsdk.toolkit.common.color.ColorRgb;
import vsdk.toolkit.environment.geometry.volume.polyhedralBoundedSolid.PolyhedralBoundedSolid;

/**
As noted in [MANT1988].10.2.2, a `_PolyhedralBoundedSolidEdge` makes a
face-to-face relationship representing the identification of the line
segments between faces.

Note that in the sake of simplify and efficiency current programming 
implementation this class exhibit public access attributes. It is important
to note that those attributes will only be accessed directly from related 
classes in the same package
(vsdk.toolkit.environment.geometry.volume.polyhedralBoundedSolid.nodes) and
from methods in the `PolyhedralBoundedSolid` class, and that they should
not be used from outer classes.
*/
public class _PolyhedralBoundedSolidEdge extends FundamentalEntity
    implements ArrayLinkable {
    @Serial private static final long serialVersionUID = 20061118L;

    /// Place of this element in the list that holds it, kept up to date by
    /// the list (see `ArrayLinkable`).
    private Object listOwner;
    private int listIndex = -1;

    @Override
    public Object getListOwner()
    {
        return listOwner;
    }

    @Override
    public int getListIndex()
    {
        return listIndex;
    }

    @Override
    public void setListSlot(Object owner, int index)
    {
        listOwner = owner;
        listIndex = index;
    }

    /// Reference to `right` half edge, as defined in [MANT1988].10.2.2.
    /// Note that half edge in this side is considered positively oriented.
    public _PolyhedralBoundedSolidHalfEdge rightHalf;

    /// Reference to `right` half edge, as defined in [MANT1988].10.2.2.
    /// Note that half edge in this side is considered negatively oriented.
    public _PolyhedralBoundedSolidHalfEdge leftHalf;

    //
    public int id;
    public ColorRgb debugColor;
    private static int currentId = 1;

    //=================================================================
    public _PolyhedralBoundedSolidEdge(PolyhedralBoundedSolid parentSolid)
    {
        init(parentSolid);
    }

    private void init(PolyhedralBoundedSolid parentSolid)
    {
        parentSolid.getEdgesList().add(this);
        rightHalf = null;
        leftHalf = null;

        id = currentId;
        currentId++;

        debugColor = new ColorRgb(1, 1, 1);
    }

    public int getEndingVertexId()
    {
        if ( leftHalf == null ) {
            return -1;
        }
        return leftHalf.startingVertex.id;
    }

    public int getStartingVertexId()
    {
        if ( rightHalf == null ) {
            return -1;
        }
        return rightHalf.startingVertex.id;
    }

    @Override
    public String toString()
    {
        String msg;
        msg = "Edge id " + id + ". Half1: ";
        if ( leftHalf == null ) {
            msg = msg + "null. ";
        }
        else {
            msg = msg + "vertex " + leftHalf.startingVertex.id;
        }

        msg = msg + " / Half2: ";

        if ( rightHalf == null ) {
            msg = msg + "null. ";
        }
        else {
            msg = msg + "vertex " + rightHalf.startingVertex.id;
        }

        return msg;
    }
}
