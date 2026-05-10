/**
Class definition of path nodes. These node are building blocks of paths.
and contain necessary information for raytracing-like algorithms
*/

package vsdk.toolkit.raycasting.common;

import java.io.PrintStream;

import vsdk.toolkit.common.logging.Logger;
import vsdk.toolkit.common.linealAlgebra.Vector3D;
import vsdk.toolkit.common.color.ColorRgb;
import vsdk.toolkit.io.wrapper.Vector3DPrinter;
import vsdk.toolkit.material.Material;
import vsdk.toolkit.material.PhongBidirectionalScatteringDistributionFunction;
import vsdk.toolkit.skin.RayHitFlag;
import vsdk.toolkit.skin.RayHit;

// Type definitions used in CPathNode

// -- TODO clean up, additional functions that are now duplicated
// -- in the samplers, accessor methods, splitting in a
// -- path node and spear rate connections

public class SimpleRaytracingPathNode {
    /**
    Heuristic for multiple Importance sampling / weighting
    */
    public static double multipleImportanceSampling(double a) {
        return a * a;
    }

    public RayHit m_hit;
    public Vector3D m_inDirT; // towards the patch
    public Vector3D m_inDirF; // from the patch
    public Vector3D m_normal;

    public double m_G; // Geometric factor x(i-1) -> x(i)
    public double m_pdfFromPrev;
    public double m_pdfFromNext;
    public double m_rrPdfFromNext; // Path length in other direction not
    //  known beforehand => separate components needed.
    public double accumulatedRussianRouletteFactors;

    public ColorRgb m_bsdfEval;
    public BsdfComp m_bsdfComp;

    public byte m_usedComponents; // Components used for scattering in this
    // node. For full bsdf sampling, these are all components, independent
    // of which component is actually used (M.I.S.), but other types
    // of sampling are possible.
    public byte m_accUsedComponents; // Accumulated used components. This allows
    // one to check immediately if any previous node in the path had a certain
    // scattering component used. accUsed = prev->used | prev->accUsed

    public PhongBidirectionalScatteringDistributionFunction m_useBsdf; // bsdf used for scattering
    public PhongBidirectionalScatteringDistributionFunction m_inBsdf; // Medium of incoming ray
    public PhongBidirectionalScatteringDistributionFunction m_outBsdf; //Medium of a possible transmitted ray (other side of normal)
    public PathRayType m_rayType;
    public int m_depth; // First node in a path has depth 0

    private SimpleRaytracingPathNode m_next;
    private SimpleRaytracingPathNode m_previous;

    public SimpleRaytracingPathNode() {
        m_hit = new RayHit();
        m_inDirT = new Vector3D();
        m_inDirF = new Vector3D();
        m_normal = new Vector3D();

        m_G = 0.0;
        m_pdfFromPrev = 0.0;
        m_pdfFromNext = 0.0;
        m_rrPdfFromNext = 0.0;
        accumulatedRussianRouletteFactors = 0.0;

        m_bsdfEval = new ColorRgb();
        m_bsdfComp = new BsdfComp();

        m_usedComponents = 0;
        m_accUsedComponents = 0;
        m_useBsdf = null;
        m_inBsdf = null;
        m_outBsdf = null;
        m_rayType = PathRayType.STARTS;
        m_depth = 0;

        m_next = null;
        m_previous = null;
    }

    // Navigation in a path
    public SimpleRaytracingPathNode next() {
        return m_next;
    }

    public SimpleRaytracingPathNode previous() {
        return m_previous;
    }

    public void setNext(SimpleRaytracingPathNode node) {
        m_next = node;
    }

    public void setPrevious(SimpleRaytracingPathNode node) {
        m_previous = node;
    }

    public void attach(SimpleRaytracingPathNode node) {
        m_next = node;
        node.setPrevious(this);
    }

    public void ensureNext() {
        if (m_next == null) {
            attach(new SimpleRaytracingPathNode());
        }
    }

    public void print(PrintStream out) {
        if (out == null) {
            return;
        }

        out.printf("Path node at depth %d\n", m_depth);
        out.printf("Pos : ");
        Vector3DPrinter.print(m_hit.getPoint(), out);
        out.printf("\n");
        out.printf("Norm: ");
        Vector3DPrinter.print(m_normal, out);
        out.printf("\n");
        if (m_previous != null) {
            out.printf("InF: ");
            Vector3DPrinter.print(m_inDirF, out);
            out.printf("\n");
            out.printf("Cos in  %f\n", m_normal.dotProduct(m_inDirF));
            out.printf("GCos in %f\n", m_hit.getPatch().normal.dotProduct(m_inDirF));
        }
        if (m_next != null) {
            out.printf("OutF: ");
            Vector3DPrinter.print(m_next.m_inDirT, out);
            out.printf("\n");
            out.printf("Cos out %f\n", m_normal.dotProduct(m_next.m_inDirT));
            out.printf("GCos out %f\n", m_hit.getPatch().normal.dotProduct(m_next.m_inDirT));
        }
    }

    /**
GetPreviousBsdf

Searches back in the path to find the bsdf on the outside
of the current path/material. The last node should be
a back hit -> Leaving ray-type. If not, the current bsdf
is returned and an error is reported
*/

    /**
Helper routine, searches the corresponding 'Enters' node for this node
*/
    protected SimpleRaytracingPathNode GetMatchingNode() {
        final PhongBidirectionalScatteringDistributionFunction thisBsdf = m_useBsdf;
        int backHits = 1;
        SimpleRaytracingPathNode tmpNode = previous();
        SimpleRaytracingPathNode matchedNode = null;

        while (tmpNode != null && backHits > 0) {
            switch (tmpNode.m_rayType) {
                case ENTERS:
                    if (tmpNode.m_hit.getPatch().material.getBsdf() == thisBsdf) {
                        backHits--; // Entering point in this material
                    }
                    break;
                case LEAVES:
                    if (tmpNode.m_inBsdf == thisBsdf) {
                        backHits++; // Leaves the same material more than one time
                    }
                    break;
                case REFLECTS:
                    break;
                default:
                    Logger.error("CPathNode::GetMatchingNode", "Wrong ray type in path");
            }

            matchedNode = tmpNode;
            tmpNode = tmpNode.previous();
        }

        if (backHits == 0) {
            return matchedNode;
        }
        else {
            return null;  // No matching node
        }
    }

    public PhongBidirectionalScatteringDistributionFunction getPreviousBsdf() {
        if ((m_hit.getFlags() & RayHitFlag.BACK) == 0) {
            Logger.error("CPathNode::getPreviousBsdf", "Last node not a back hit");
            return m_inBsdf;  // Should not happen
        }

        if (m_hit.getPatch().material.getBsdf() != m_inBsdf) {
            Logger.warning("CPathNode::GetPreviousBtdf", "Last back hit has wrong bsdf");
        }

        // Find the corresponding ray that enters the material
        final SimpleRaytracingPathNode matchedNode = GetMatchingNode();

        if (matchedNode == null) {
            Logger.warning("CPathNode::GetPreviousBtdf", "No corresponding entering ray");
            return m_inBsdf;  // Should not happen
        }

        return matchedNode.m_inBsdf;
    }

    public void assignBsdfAndNormal() {
        final Material thisMaterial;

        if (m_hit.getPatch() == null) {
            // Invalid node
            return;
        }

        thisMaterial = m_hit.getPatch().material;

        m_normal.copy(m_hit.getNormal()); // Possible double format

        // Assign bsdf's
        m_useBsdf = thisMaterial.getBsdf();

        if ((m_hit.getFlags() & RayHitFlag.FRONT) != 0) {
            m_outBsdf = m_useBsdf; // In filled in when creating this node
        }
        else {
            // BACK HIT
            m_outBsdf = getPreviousBsdf();
        }
    }

    public boolean ends() {
        return (m_rayType == PathRayType.STOPS) || (m_rayType == PathRayType.ENVIRONMENT);
    }
}
