package vsdk.toolkit.galerkin.processing;

import vsdk.toolkit.scene.DirectPotentialTable;
import vsdk.toolkit.material.RendererConfiguration;
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
    protected static double pushPullPotential(GalerkinElement element, double down, DirectPotentialTable directPotentials) {
        if ( element == null ) {
            return 0.0;
        }
        down += element.area > 0.0 ? (element.receivedPotential / element.area) : 0.0;
        element.receivedPotential = 0.0;
        double up = 0.0;

        if ( element.regularSubElements == null && element.irregularSubElements == null ) {
            up = down + (element.patch != null ? directPotentials.get(element.patch.getFaceIndex()) : 0.0);
        }

        if ( element.regularSubElements != null ) {
            for ( int i = 0; i < 4; i++ ) {
                if ( element.regularSubElements[i] instanceof GalerkinElement ) {
                    up += 0.25 * pushPullPotential((GalerkinElement)element.regularSubElements[i], down, directPotentials);
                }
            }
        }

        if ( element.irregularSubElements != null ) {
            for ( int j = 0; j < element.irregularSubElements.size(); j++ ) {
                if ( !(element.irregularSubElements.get(j) instanceof GalerkinElement) ) {
                    continue;
                }
                GalerkinElement subElement = (GalerkinElement)element.irregularSubElements.get(j);
                double localDown = element.isCluster() ? down : 0.0;
                up += element.area > 0.0
                    ? (subElement.area / element.area) * pushPullPotential(subElement, localDown, directPotentials)
                    : 0.0;
            }
        }

        element.potential = up;
        return up;
    }

    public GatheringStrategy() {
    }

    public abstract boolean doGatheringIteration(Scene scene, GalerkinState galerkinState, RendererConfiguration renderOptions);
}
