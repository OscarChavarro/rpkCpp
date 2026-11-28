package vsdk.toolkit.galerkin.processing;

import java.util.ArrayList;
import java.util.Arrays;
import vsdk.toolkit.common.linealAlgebra.Vector3Dd;
import vsdk.toolkit.galerkin.GalerkinElement;
import vsdk.toolkit.render.raster.polygonScanConverter.PolygonVisibilityBuffer;
import vsdk.toolkit.skin.RpkAxisAlignedBoundingBox;

/**
Scratch of the Z-visibility clustering strategy: the software renderer the
elements of a cluster are rendered into, seen from a sample point, with the
virtual screen of the last rendering and the samples each element covers in
it, as a list of the visible elements in the order they first appear.

The frame buffer of the scratch renderer holds in each pixel the surface
element seen there, borrowed for the current solve. A null sample is
background. Java adaptation: the C++ port names that buffer
ScratchVisibilityBuffer, an alias of PolygonVisibilityBuffer<const
GalerkinElement *>; here it is PolygonVisibilityBuffer<GalerkinElement>.

Workspace: mutable, owned by the solver (the serial one) or by a parallel
Jacobi worker, and passed explicitly to ScratchVisibilityStrategy, so that
workers render at the same time. The last cluster and eye point are kept to
reuse a rendering; they must match exactly, so that the result never depends
on which rendering a workspace did before. The lists keep their storage from
one counting to the next.
*/
public final class ScratchVisibilityWorkspace {
    private final PolygonVisibilityBuffer<GalerkinElement> buffer;
    private GalerkinElement lastCluster;
    private Vector3Dd lastEye;
    private final RpkAxisAlignedBoundingBox bounds;

    // Visible elements of the last counting, and for each element id the
    // counting that last saw it (countingStamp) and its index in the list.
    // Java adaptation: the C++ ArrayList<int> lists are growable int arrays
    // here (sizes in numberOfCountedIds), so that no index is boxed
    private final ArrayList<ElementSamples> visibleElements;
    private int[] countingStamps;
    private int[] visibleIndices;
    private int numberOfCountedIds;
    private int countingStamp;

    public ScratchVisibilityWorkspace(int frameBufferSize) {
        buffer = new PolygonVisibilityBuffer<>(frameBufferSize, frameBufferSize, null);
        lastCluster = null;
        lastEye = new Vector3Dd();
        bounds = new RpkAxisAlignedBoundingBox();
        visibleElements = new ArrayList<>();
        countingStamps = new int[16];
        visibleIndices = new int[16];
        numberOfCountedIds = 0;
        countingStamp = 0;
    }

    public PolygonVisibilityBuffer<GalerkinElement> getBuffer() {
        return buffer;
    }

    /** True when the buffer holds the rendering of this cluster seen from exactly this eye */
    public boolean holdsRendering(GalerkinElement cluster, Vector3Dd eye) {
        return cluster == lastCluster && eye.x() == lastEye.x() && eye.y() == lastEye.y() && eye.z() == lastEye.z();
    }

    public void setRendering(GalerkinElement cluster, Vector3Dd eye) {
        lastCluster = cluster;
        lastEye = eye;
    }

    /** Virtual screen of the last rendering, in the coordinates of its eye */
    public RpkAxisAlignedBoundingBox getBounds() {
        return bounds;
    }

    /** Starts a new counting: no element is visible */
    public void startCounting() {
        visibleElements.clear();
        countingStamp++;
    }

    /** Adds samples to the element of the given id */
    public void addSamples(GalerkinElement element, int elementId, int samples) {
        while ( numberOfCountedIds <= elementId ) {
            if ( numberOfCountedIds == countingStamps.length ) {
                countingStamps = Arrays.copyOf(countingStamps, 2 * numberOfCountedIds);
                visibleIndices = Arrays.copyOf(visibleIndices, 2 * numberOfCountedIds);
            }
            countingStamps[numberOfCountedIds] = 0;
            visibleIndices[numberOfCountedIds] = 0;
            numberOfCountedIds++;
        }
        if ( countingStamps[elementId] == countingStamp ) {
            final int index = visibleIndices[elementId];
            visibleElements.get(index).addSamples(samples);
        } else {
            countingStamps[elementId] = countingStamp;
            visibleIndices[elementId] = visibleElements.size();
            visibleElements.add(new ElementSamples(element, samples));
        }
    }

    /** The elements of the last counting, each with its samples */
    public ArrayList<ElementSamples> getVisibleElements() {
        return visibleElements;
    }
}
