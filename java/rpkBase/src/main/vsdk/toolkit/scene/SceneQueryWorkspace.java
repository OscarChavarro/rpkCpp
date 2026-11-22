package vsdk.toolkit.scene;

import vsdk.toolkit.environment.geometry.spatialIndex.SpatialIndexWorkspace;

/**
Per-query scratch of a SceneQuery: the workspace of its index (or of the
legacy grid), and the ray capture recorder its callers report to (ray
capture runs only, otherwise null).

Workspace: mutable, owned by one caller and passed explicitly on every
SceneQuery.intersect call.

Java adaptation: it also keeps the face test that the C++ port builds on the
stack for every query.
*/
public final class SceneQueryWorkspace {
    private final VoxelGridWorkspace legacyWorkspace; // Legacy grid only
    private final SpatialIndexWorkspace indexWorkspace; // The other methods
    private SceneFaceRayTest faceRayTest;
    private final VisibilityQueryRecorder recorder; // Not owned

    public SceneQueryWorkspace(SceneQuery query) {
        this(query, null);
    }

    public SceneQueryWorkspace(SceneQuery query, VisibilityQueryRecorder inRecorder) {
        if (query.getLegacyGrid() != null) {
            legacyWorkspace = new VoxelGridWorkspace(
                query.getLegacyGrid().getNumberOfItems(), query.getLegacyGrid().getNumberOfLevels());
            indexWorkspace = null;
        }
        else {
            legacyWorkspace = null;
            indexWorkspace = new SpatialIndexWorkspace(query.getIndex().getWorkspaceSize());
        }
        faceRayTest = null;
        recorder = inRecorder;
    }

    public VoxelGridWorkspace getLegacyWorkspace() {
        return legacyWorkspace;
    }

    public SpatialIndexWorkspace getIndexWorkspace() {
        return indexWorkspace;
    }

    public VisibilityQueryRecorder getRecorder() {
        return recorder;
    }

    SceneFaceRayTest getFaceRayTest(Scene scene) {
        if (faceRayTest == null) {
            faceRayTest = new SceneFaceRayTest(scene, null, 0, null, null);
        }
        return faceRayTest;
    }
}
