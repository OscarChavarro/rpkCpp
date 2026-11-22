/**
Generic class for samplers. Samplers operate on
path nodes and have to possible actions :
  - sample : fill in a new path node and evaluate bsdf's and pdf's
             where necessary.
  - connect : connect to sub-paths and evaluate the necessary
              bsdf's and pdf's.
*/

package vsdk.toolkit.raycasting.raytracing;

import vsdk.toolkit.common.VSDK;
import vsdk.toolkit.common.linealAlgebra.Ray;
import vsdk.toolkit.common.linealAlgebra.Vector3Df;
import vsdk.toolkit.material.BsdfComponent;
import vsdk.toolkit.environment.geometry.elements.RayHitFlag;
import vsdk.toolkit.raycasting.common.PathRayType;
import vsdk.toolkit.raycasting.common.RayTools;
import vsdk.toolkit.raycasting.common.SimpleRaytracingPathNode;
import vsdk.toolkit.scene.Background;
import vsdk.toolkit.scene.Camera;
import vsdk.toolkit.scene.SceneQuery;
import vsdk.toolkit.scene.SceneQueryWorkspace;
import vsdk.toolkit.environment.geometry.elements.RayHit;

public abstract class Sampler {
    public static final byte BSDF_ALL_COMPONENTS =
        (byte)(BsdfComponent.BRDF_DIFFUSE_COMPONENT
            | BsdfComponent.BRDF_GLOSSY_COMPONENT
            | BsdfComponent.BRDF_SPECULAR_COMPONENT
            | BsdfComponent.BTDF_DIFFUSE_COMPONENT
            | BsdfComponent.BTDF_GLOSSY_COMPONENT
            | BsdfComponent.BTDF_SPECULAR_COMPONENT);

    /**
    sample transfer generates a new point on a surface by ray tracing
    given a certain direction and the pdf for that direction
    The medium the ray is traveling through must be known and
    is given by newNode->m_inBsdf.
    The function fills in newNode: hit, normal, directions, pdf (area)
             geometry factor and depth  (rayType is un-touched)
    It returns false if no point was found when tracing a ray
    or if a shading normal anomaly occurs
    */
    protected boolean sampleTransfer(
        SceneQuery sceneQuery,
        SceneQueryWorkspace sceneSceneQueryWorkspace,
        Background sceneBackground,
        SimpleRaytracingPathNode thisNode,
        SimpleRaytracingPathNode newNode,
        Vector3Df dir,
        double pdfDir)
    {
        Ray ray = new Ray();
        ray.position = thisNode.m_hit.getPoint();
        ray.direction = dir;

        // Fill in depth
        newNode.m_depth = thisNode.m_depth + 1;
        newNode.m_rayType = PathRayType.STOPS;
        RayHit hit = RayTools.findRayIntersection(
            sceneQuery,
            sceneSceneQueryWorkspace,
            ray,
            thisNode.m_hit.getPatch(),
            newNode.m_inBsdf,
            newNode.m_hit);

        if ( hit == null ) {
            if ( sceneBackground != null ) {
                // Fill in path node for background
                newNode.m_hit.init(sceneBackground.bkgPatch, null, dir, null);
                newNode.m_inDirT = dir;
                newNode.m_inDirF = new Vector3Df(dir.x(), dir.y(), dir.z());
                newNode.m_pdfFromPrev = pdfDir;
                newNode.m_G = Math.abs(thisNode.m_hit.getNormal().dotProduct(newNode.m_inDirT));
                newNode.m_inBsdf = thisNode.m_outBsdf;
                newNode.m_useBsdf = null;
                newNode.m_outBsdf = null;
                newNode.m_rayType = PathRayType.ENVIRONMENT;
                int newFlags = newNode.m_hit.getFlags() | RayHitFlag.FRONT;
                newNode.m_hit.setFlags(newFlags);

                return true;
            } else {
                // if no background is present
                return false;
            }
        }

        if ( (hit.getFlags() & RayHitFlag.BACK) != 0 ) {
            // Back hit, invert normal (only happens when newNode->m_inBsdf != nullptr
            Vector3Df normal = new Vector3Df();
            normal = newNode.m_hit.getNormal().multiply(-1.0f);
            newNode.m_hit.setNormal(normal);
        }

        newNode.m_inDirT = ray.direction;
        newNode.m_inDirF = newNode.m_inDirT.multiply(-1.0f);

        // Check for shading normal vs. geometric normal errors
        if ( newNode.m_hit.getNormal().dotProduct(newNode.m_inDirF) < 0 ) {
            // Shading normal anomaly
            return false;
        }

        // Compute geometry factor cos(a)*cos(b)/r^2
        Vector3Df tmpVec = new Vector3Df();

        double cosA = Math.abs(thisNode.m_hit.getNormal().dotProduct(newNode.m_inDirT));
        double cosB = Math.abs(newNode.m_hit.getNormal().dotProduct(newNode.m_inDirT));
        tmpVec = newNode.m_hit.getPoint().subtract(thisNode.m_hit.getPoint());
        double dist2 = tmpVec.lengthSquared();

        if ( dist2 < VSDK.EPSILON ) {
            // Next node is useless, gives rise to numeric errors (Inf)
            return false;
        }

        newNode.m_G = cosA * cosB / dist2; // Integrate over area !

        // Fill in probability
        newNode.m_pdfFromPrev = pdfDir * cosB / dist2;

        return true; // Transfer succeeded
    }

    // Sample : newNode gets filled, others may change
    //   Return true if the node was filled in, false if path Ends
    //   When path ends (absorption) the type of thisNode is adjusted to 'Ends'
    public abstract boolean
    sample(
        Camera camera,
        SceneQuery sceneQuery,
        SceneQueryWorkspace sceneSceneQueryWorkspace,
        Background sceneBackground,
        SimpleRaytracingPathNode prevNode,
        SimpleRaytracingPathNode thisNode,
        SimpleRaytracingPathNode newNode,
        double x1,
        double x2,
        boolean doRR,
        byte flags);

    public boolean
    sample(
        Camera camera,
        SceneQuery sceneQuery,
        SceneQueryWorkspace sceneSceneQueryWorkspace,
        Background sceneBackground,
        SimpleRaytracingPathNode prevNode,
        SimpleRaytracingPathNode thisNode,
        SimpleRaytracingPathNode newNode,
        double x1,
        double x2)
    {
        return sample(camera, sceneQuery, sceneSceneQueryWorkspace, sceneBackground, prevNode, thisNode, newNode, x1, x2, false, BSDF_ALL_COMPONENTS);
    }

    public abstract double
    evalPDF(
        Camera camera,
        SimpleRaytracingPathNode thisNode,
        SimpleRaytracingPathNode newNode,
        byte flags,
        double[] probabilityDensityFunction,
        double[] probabilityDensityFunctionRR);

    public double
    evalPDF(
        Camera camera,
        SimpleRaytracingPathNode thisNode,
        SimpleRaytracingPathNode newNode)
    {
        return evalPDF(camera, thisNode, newNode, BSDF_ALL_COMPONENTS, null, null);
    }
}
