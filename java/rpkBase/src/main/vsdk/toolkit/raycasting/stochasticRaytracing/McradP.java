package vsdk.toolkit.raycasting.stochasticRaytracing;

import vsdk.toolkit.common.color.ColorRgb;
import vsdk.toolkit.environment.geometry.elements.Patch;

public final class McradP {
    private McradP() {
    }

    public static int numberOfVertices(StochasticRadiosityElement elem) {
        return elem.patch.getNumberOfVertices();
    }

    public static StochasticRadiosityElement topLevelStochasticRadiosityElement(Patch patch) {
        return (StochasticRadiosityElement)
            ElementHierarchyState.activeState().getRadianceElements().get(patch.getFaceIndex());
    }

    public static ColorRgb[] getTopLevelPatchRad(Patch patch) {
        return topLevelStochasticRadiosityElement(patch).radiance;
    }

    public static ColorRgb[] getTopLevelPatchUnShotRad(Patch patch) {
        return topLevelStochasticRadiosityElement(patch).unShotRadiance;
    }

    public static ColorRgb[] getTopLevelPatchReceivedRad(Patch patch) {
        return topLevelStochasticRadiosityElement(patch).receivedRadiance;
    }

    public static StochasticRadiosityBasis getTopLevelPatchBasis(Patch patch) {
        return topLevelStochasticRadiosityElement(patch).basis;
    }
}
