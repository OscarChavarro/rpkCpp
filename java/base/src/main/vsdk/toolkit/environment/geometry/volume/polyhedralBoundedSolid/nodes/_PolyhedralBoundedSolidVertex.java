//= References:                                                             =
//= [MANT1988] Mantyla Martti. "An Introduction To Solid Modeling",         =
//=     Computer Science Press, 1988.                                       =

package vsdk.toolkit.environment.geometry.volume.polyhedralBoundedSolid.nodes;

import java.io.Serial;

import vsdk.toolkit.common.FundamentalEntity;
import vsdk.toolkit.common.dataStructures.ArrayLinkable;
import vsdk.toolkit.common.linealAlgebra.Vector3Dd;
import vsdk.toolkit.common.color.ColorRgb;
import vsdk.toolkit.environment.geometry.volume.polyhedralBoundedSolid.PolyhedralBoundedSolid;

/**
As noted in [MANT1988].10.2.2, a `_PolyhedralBoundedSolidVertex` contains
a vertex position for the geometric information of the boundary model,
and a reference to one of the half-edges emanating from it.
*/
public class _PolyhedralBoundedSolidVertex extends FundamentalEntity
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

    /// Defined as presented in [MANT1988].10.2.1
    public int id;

    /// Defined as presented in [MANT1988].10.2.1
    public Vector3Dd position;

    /// Defined as presented in [MANT1988].10.2.2
    public _PolyhedralBoundedSolidHalfEdge emanatingHalfEdge;

    public ColorRgb debugColor;

    //=================================================================
    public _PolyhedralBoundedSolidVertex(PolyhedralBoundedSolid parentSolid,
                                         Vector3Dd position, int id)
    {
        init(parentSolid, position, id);
    }

    private void
    init(PolyhedralBoundedSolid parentSolid, Vector3Dd position, int id)
    {
        this.id = id;
        emanatingHalfEdge = null;
        this.position = new Vector3Dd(position);
        parentSolid.getVerticesList().add(this);
        debugColor = new ColorRgb(1, 0, 0);
    }

    @Override
    public String toString()
    {
        String msg;
        msg = "vertex id " + id + ". Position " + position + ". ";
        if ( emanatingHalfEdge == null ) {
            msg = msg + "No associated half-edge.";
        }
        else {
            msg = msg + "H.E. " + emanatingHalfEdge.id;
        }
        return msg;
    }
}
