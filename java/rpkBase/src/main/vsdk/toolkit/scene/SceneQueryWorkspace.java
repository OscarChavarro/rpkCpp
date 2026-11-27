package vsdk.toolkit.scene;

import vsdk.toolkit.environment.geometry.spatialIndex.SpatialIndexWorkspace;

/**
Per-query scratch of a SceneQuery: the workspace of its index and an optional
ray counter.

Workspace: mutable, owned by one caller and passed explicitly on every
SceneQuery.intersect call.

Java adaptation: it also keeps the face test that the C++ port builds on the
stack for every query.
*/
public final class SceneQueryWorkspace {
    private final SpatialIndexWorkspace indexWorkspace; // Owned
    private SceneFaceRayTest faceRayTest;
    private final boolean countRays;
    private long rayCount;

    public SceneQueryWorkspace(SceneQuery query) {
        this(query, false);
    }

    public SceneQueryWorkspace(SceneQuery query, boolean countRays) {
        indexWorkspace = new SpatialIndexWorkspace(query.getIndex().getWorkspaceSize());
        faceRayTest = null;
        this.countRays = countRays;
    }

    public SpatialIndexWorkspace getIndexWorkspace() {
        return indexWorkspace;
    }

    public boolean recordRay() {
        if ( countRays ) {
            rayCount++;
        }
        return countRays;
    }

    public long getRayCount() {
        return rayCount;
    }

    SceneFaceRayTest getFaceRayTest(Scene scene) {
        if (faceRayTest == null) {
            faceRayTest = new SceneFaceRayTest(scene, null, 0, null, null);
        }
        return faceRayTest;
    }
}
