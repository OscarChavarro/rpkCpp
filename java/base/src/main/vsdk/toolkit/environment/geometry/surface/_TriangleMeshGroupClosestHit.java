package vsdk.toolkit.environment.geometry.surface;

import java.util.ArrayList;

import vsdk.toolkit.environment.geometry.boundingVolumeHierarchy.BinnedSahLeafVisitor;
import vsdk.toolkit.environment.geometry.element.Ray;
import vsdk.toolkit.environment.geometry.element.RayHit;

/**
Closest hit over the meshes of a group: the leaf test of the top level
hierarchy and, called for every mesh in order, the linear loop. Keeps the
first (lowest index) mesh among equal distances, as the linear loop does.
*/
class _TriangleMeshGroupClosestHit implements BinnedSahLeafVisitor {
    final ArrayList<TriangleMesh> meshes;
    final Ray ray;
    RayHit bestHit;
    boolean hasBestHit;
    int bestMesh;
    int bestTriangle;

    _TriangleMeshGroupClosestHit(ArrayList<TriangleMesh> inMeshes, Ray inRay)
    {
        meshes = inMeshes;
        ray = inRay;
        bestHit = null;
        hasBestHit = false;
        bestMesh = -1;
        bestTriangle = -1;
    }

    void reset()
    {
        hasBestHit = false;
        bestMesh = -1;
        bestTriangle = -1;
    }

    @Override
    public void visit(int mesh, double[] inOutBestT)
    {
        RayHit meshHit = new RayHit();
        int[] triangle = {-1};
        if ( !meshes.get(mesh).doIntersectionFirstHit(ray, meshHit, triangle) || meshHit.getRay() == null ) {
            return;
        }
        final double t = meshHit.getRay().getT();
        if ( t < inOutBestT[0] || (t == inOutBestT[0] && hasBestHit && mesh < bestMesh) ) {
            inOutBestT[0] = t;
            bestHit = new RayHit(meshHit);
            bestMesh = mesh;
            bestTriangle = triangle[0];
            hasBestHit = true;
        }
    }
}
