//= References:                                                             =
//= [WALD2007] Wald, Ingo. "On fast Construction of SAH-based Bounding      =
//=     Volume Hierarchies". Proceedings of the 2007 IEEE Symposium on      =
//=     Interactive Ray Tracing, 2007, pp. 33-40.                           =
//= [SNYD1987] Snyder, John M. and Barr, Alan H. "Ray Tracing Complex       =
//=     Models Containing Surface Tessellations". SIGGRAPH '87, Computer    =
//=     Graphics 21(4), 1987, pp. 119-128.                                  =
//=     PDF: doc/references/[SNYD1987]_*.pdf                                =

package vsdk.toolkit.environment.geometry.boundingVolumeHierarchy;

/**
Methods to find the first hit of a ray against the triangles of a
`TriangleMesh`, selected by `RendererConfiguration.setMeshAccelerationMethod`.
Every method runs on the CPU only (no GPU, no threads), so all of them are
available on every vitral platform, and every method gives exactly the hit
of `BRUTE_FORCE`.

To add a scheme: add a constant here, its names in
`TriangleMeshAccelerationMethodUtil`, a `TriangleAccelerationStructure`
subclass, and its case in `TriangleAccelerationStructure.create`.
*/
public enum TriangleMeshAccelerationMethod {
    /** No acceleration structure: every triangle is tested */
    BRUTE_FORCE,

    /**
    Binary bounding volume hierarchy built top-down with the binned surface
    area heuristic of [WALD2007].3 (`BinnedSahTriangleHierarchy`), the same
    build and traversal of `VulkanBvhBuilder` and
    `computeAccelerationStructure.glsl` in the Vulkan 1.4 raytracer.
    */
    WALD2007_BINNED_SAH,

    /**
    Uniform grid of [SNYD1987] (`GridTriangleHierarchy`): the
    `GridSpatialIndex` port of RenderPark's voxel grid over the triangles,
    for comparing the two index families on one mesh.
    */
    VOXEL_GRID
}
