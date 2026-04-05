package vsdk.toolkit.galerkin.processing;

import java.util.ArrayList;
import vsdk.toolkit.common.linealAlgebra.Numeric;
import vsdk.toolkit.galerkin.GalerkinBasis;
import vsdk.toolkit.galerkin.GalerkinElement;
import vsdk.toolkit.galerkin.GalerkinIterationMethod;
import vsdk.toolkit.galerkin.GalerkinRole;
import vsdk.toolkit.galerkin.GalerkinState;
import vsdk.toolkit.galerkin.Interaction;

/**
Creates an initial link between the given element and the top cluster
*/
public class LinkingClusteredStrategy {
    public static void createInitialLinks(
        GalerkinElement element,
        GalerkinRole role,
        GalerkinState galerkinState)
    {
        GalerkinElement receiverElement;
        GalerkinElement sourceElement;

        switch ( role ) {
            case RECEIVER:
                receiverElement = element;
                sourceElement = galerkinState.topCluster;
                break;
            case SOURCE:
                sourceElement = element;
                receiverElement = galerkinState.topCluster;
                break;
            default:
                return;
        }

        if ( receiverElement == null || sourceElement == null ) {
            return;
        }

        // Assume no light transport (overlapping receiver and source)
        int n = receiverElement.basisSize * sourceElement.basisSize;
        if ( n <= 0 ) {
            n = 1;
        }
        float[] K = new float[n];
        for ( int i = 0; i < n; i++ ) {
            K[i] = 0.0f;
        }

        float[] deltaK = new float[1];
        deltaK[0] = Numeric.HUGE_FLOAT_VALUE; // Huge value error on the form factor

        Interaction newLink = new Interaction(
            receiverElement,
            sourceElement,
            K,
            deltaK,
            (byte)receiverElement.basisSize,
            (byte)sourceElement.basisSize,
            (byte)1,
            (byte)128
        );

        // Store interactions with the source patch for the progressive radiosity method
        // and with the receiving patch for gathering methods
        if ( galerkinState.galerkinIterationMethod == GalerkinIterationMethod.SOUTH_WELL ) {
            if ( sourceElement.interactions == null ) {
                sourceElement.interactions = new ArrayList<>();
            }
            sourceElement.interactions.add(newLink);
        } else {
            if ( receiverElement.interactions == null ) {
                receiverElement.interactions = new ArrayList<>();
            }
            receiverElement.interactions.add(newLink);
        }
    }
}
