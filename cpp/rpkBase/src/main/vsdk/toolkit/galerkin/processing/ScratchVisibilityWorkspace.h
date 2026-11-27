#ifndef __SCRATCH_VISIBILITY_WORKSPACE__
#define __SCRATCH_VISIBILITY_WORKSPACE__

#include "java/util/ArrayList.h"
#include "vsdk/toolkit/common/linealAlgebra/Vector3Dd.h"
#include "vsdk/toolkit/skin/RpkAxisAlignedBoundingBox.h"
#include "vsdk/toolkit/render/raster/polygonScanConverter/PolygonVisibilityBuffer.h"
#include "vsdk/toolkit/galerkin/processing/ElementSamples.h"

class GalerkinElement;

/**
Frame buffer of the scratch renderer: each pixel holds the surface element
seen there, borrowed for the current solve. A null sample is background.
*/
// Instantiated once, in ScratchVisibilityWorkspace.cpp
extern template class PolygonVisibilityBuffer<const GalerkinElement *>;
using ScratchVisibilityBuffer = PolygonVisibilityBuffer<const GalerkinElement *>;

/**
Scratch of the Z-visibility clustering strategy: the software renderer the
elements of a cluster are rendered into, seen from a sample point, with the
virtual screen of the last rendering and the samples each element covers in
it, as a list of the visible elements in the order they first appear.

Workspace: mutable, owned by the solver (the serial one) or by a parallel
Jacobi worker, and passed explicitly to ScratchVisibilityStrategy, so that
workers render at the same time. The last cluster and eye point are kept to
reuse a rendering; they must match exactly, so that the result never depends
on which rendering a workspace did before. The lists keep their storage from
one counting to the next.
*/
class ScratchVisibilityWorkspace final {
  private:
    ScratchVisibilityBuffer buffer;
    const GalerkinElement *lastCluster;
    Vector3Dd lastEye;
    RpkAxisAlignedBoundingBox bounds;

    // Visible elements of the last counting, and for each element id the
    // counting that last saw it (countingStamp) and its index in the list
    java::ArrayList<ElementSamples> visibleElements;
    java::ArrayList<int> countingStamps;
    java::ArrayList<int> visibleIndices;
    int countingStamp;

  public:
    explicit ScratchVisibilityWorkspace(int frameBufferSize);
    ScratchVisibilityWorkspace(const ScratchVisibilityWorkspace &) = delete;
    ScratchVisibilityWorkspace &operator=(const ScratchVisibilityWorkspace &) = delete;

    ScratchVisibilityBuffer &getBuffer() { return buffer; }
    const ScratchVisibilityBuffer &getBuffer() const { return buffer; }

    /** True when the buffer holds the rendering of this cluster seen from exactly this eye */
    bool
    holdsRendering(const GalerkinElement *cluster, const Vector3Dd &eye) const {
        return cluster == lastCluster && eye.x() == lastEye.x() && eye.y() == lastEye.y() && eye.z() == lastEye.z();
    }

    void
    setRendering(const GalerkinElement *cluster, const Vector3Dd &eye) {
        lastCluster = cluster;
        lastEye = eye;
    }

    /** Virtual screen of the last rendering, in the coordinates of its eye */
    RpkAxisAlignedBoundingBox &getBounds() { return bounds; }

    /** Starts a new counting: no element is visible */
    void startCounting();

    /** Adds samples to the element of the given id */
    void addSamples(GalerkinElement *element, int elementId, int samples);

    /** The elements of the last counting, each with its samples */
    const java::ArrayList<ElementSamples> &getVisibleElements() const { return visibleElements; }
};

#endif
