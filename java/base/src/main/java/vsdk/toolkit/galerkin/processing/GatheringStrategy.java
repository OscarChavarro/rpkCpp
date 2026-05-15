package vsdk.toolkit.galerkin.processing;

import vsdk.toolkit.material.RenderOptions;
import vsdk.toolkit.galerkin.GalerkinElement;
import vsdk.toolkit.galerkin.GalerkinState;
import vsdk.toolkit.scene.Scene;

/**
Reference:
[COHE1993] Cohen, M. Wallace, J. "Radiosity and Realistic Image Synthesis",
     Academic Press Professional, 1993.
*/

/**
Numerical integration for Jacobi or Gauss-Seidel Galerkin radiosity.
See [COHE1993] section 5.3.2.
*/
public abstract class GatheringStrategy {
    protected static float pushPullPotential(GalerkinElement element, float down) {
        if ( element == null ) {
            return 0.0f;
        }
        down += element.area > 0.0f ? (element.receivedPotential / element.area) : 0.0f;
        element.receivedPotential = 0.0f;
        float up = 0.0f;

        if ( element.regularSubElements == null && element.irregularSubElements == null ) {
            up = down + (element.patch != null ? element.patch.directPotential : 0.0f);
        }

        if ( element.regularSubElements != null ) {
            for ( int i = 0; i < 4; i++ ) {
                if ( element.regularSubElements[i] instanceof GalerkinElement ) {
                    up += 0.25f * pushPullPotential((GalerkinElement)element.regularSubElements[i], down);
                }
            }
        }

        if ( element.irregularSubElements != null ) {
            for ( int j = 0; j < element.irregularSubElements.size(); j++ ) {
                if ( !(element.irregularSubElements.get(j) instanceof GalerkinElement) ) {
                    continue;
                }
                GalerkinElement subElement = (GalerkinElement)element.irregularSubElements.get(j);
                float localDown = element.isCluster() ? down : 0.0f;
                up += element.area > 0.0f
                    ? (subElement.area / element.area) * pushPullPotential(subElement, localDown)
                    : 0.0f;
            }
        }

        element.potential = up;
        return up;
    }

    public GatheringStrategy() {
    }

    public abstract boolean doGatheringIteration(Scene scene, GalerkinState galerkinState, RenderOptions renderOptions);
}
