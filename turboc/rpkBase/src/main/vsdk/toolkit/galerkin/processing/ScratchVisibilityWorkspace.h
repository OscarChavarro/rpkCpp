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
Instantiated once, in ScratchVisibilityWorkspace.cpp. C++98 has no extern
template (the cpp port declares one here): no other translation unit includes
the member definitions of PolygonVisibilityBuffer.txx, so none instantiates
them.
*/
typedef PolygonVisibilityBuffer<const GalerkinElement *> ScratchVisibilityBuffer;

/**
Scratch of the Z-visibility clustering strategy: the software renderer the
elements of a cluster are rendered into, seen from a sample point, with the
virtual screen of the last rendering and the samples each element covers in
it, as a list of the visible elements in the order they first appear.

Workspace: mutable, owned by the solver and passed explicitly to
ScratchVisibilityStrategy (in the cpp port, parallel Jacobi workers own one
each, so that they render at the same time; this serial port has only the
solver's). The last cluster and eye point are kept to reuse a rendering; they
must match exactly, so that the result never depends on which rendering a
workspace did before. The lists keep their storage from one counting to the
next.
*/
class ScratchVisibilityWorkspace {
  private:
    ScratchVisibilityBuffer buffer;
    const GalerkinElement *lastCluster;
    Vector3Dd lastEye;
    RpkAxisAlignedBoundingBox bounds;

    // Visible elements of the last counting, and for each element id the
    // counting that last saw it (countingStamp) and its index in the list
    ArrayList<ElementSamples> visibleElements;
    ArrayList<int> countingStamps;
    ArrayList<int> visibleIndices;
    int countingStamp;

    // Not copyable (deleted in the cpp port)
    ScratchVisibilityWorkspace(const ScratchVisibilityWorkspace &);
    ScratchVisibilityWorkspace &operator=(const ScratchVisibilityWorkspace &);

  public:
    explicit ScratchVisibilityWorkspace(int frameBufferSize);

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
    const ArrayList<ElementSamples> &getVisibleElements() const { return visibleElements; }
};

#endif
