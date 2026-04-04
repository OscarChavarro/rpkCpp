package vsdk.toolkit.galerkin;

import vsdk.toolkit.common.Error;

public class Interaction {
    private static int totalInteractions;
    private static int ccInteractions;
    private static int csInteractions;
    private static int scInteractions;
    private static int ssInteractions;

    public GalerkinElement receiverElement;
    public GalerkinElement sourceElement;
    public float[] K; // Coupling coefficient(s), stored top to bottom, left to right
    public float[] deltaK; // Used for approximation error estimation over the link
    public byte numberOfBasisFunctionsOnReceiver;
    public byte numberOfBasisFunctionsOnSource;
    public byte numberOfReceiverCubaturePositions;
    public byte visibility; // 255 for full visibility, 0 for full occlusion

    public Interaction() {
    }

    public Interaction(
        GalerkinElement inReceiverElement,
        GalerkinElement inSourceElement,
        float[] inK,
        float[] inDeltaK,
        byte inNumberOfBasisFunctionsOnReceiver,
        byte inNumberOfBasisFunctionsOnSource,
        byte inNumberOfReceiverCubaturePositions,
        byte inVisibility
    ) {
        receiverElement = inReceiverElement;
        sourceElement = inSourceElement;
        numberOfBasisFunctionsOnReceiver = inNumberOfBasisFunctionsOnReceiver;
        numberOfBasisFunctionsOnSource = inNumberOfBasisFunctionsOnSource;
        numberOfReceiverCubaturePositions = inNumberOfReceiverCubaturePositions;
        visibility = inVisibility;

        int n = (numberOfBasisFunctionsOnReceiver & 0xFF) * (numberOfBasisFunctionsOnSource & 0xFF);
        if ( n <= 0 ) {
            n = 1;
        }
        K = new float[n];
        if ( inK != null ) {
            System.arraycopy(inK, 0, K, 0, Math.min(inK.length, K.length));
        }

        if ( (numberOfReceiverCubaturePositions & 0xFF) > 1 ) {
            Error.fatal(2, "interactionCreate", "Not yet implemented for higher order approximations");
        }

        deltaK = new float[1];
        if ( inDeltaK != null && inDeltaK.length > 0 ) {
            deltaK[0] = inDeltaK[0];
        }

        totalInteractions++;
        if ( inReceiverElement != null && inReceiverElement.isCluster() ) {
            if ( inSourceElement != null && inSourceElement.isCluster() ) {
                ccInteractions++;
            }
            else {
                scInteractions++;
            }
        }
        else {
            if ( inSourceElement != null && inSourceElement.isCluster() ) {
                csInteractions++;
            }
            else {
                ssInteractions++;
            }
        }
    }

    public static int getNumberOfInteractions() {
        return totalInteractions;
    }

    public static int getNumberOfClusterToClusterInteractions() {
        return ccInteractions;
    }

    public static int getNumberOfClusterToSurfaceInteractions() {
        return csInteractions;
    }

    public static int getNumberOfSurfaceToClusterInteractions() {
        return scInteractions;
    }

    public static int getNumberOfSurfaceToSurfaceInteractions() {
        return ssInteractions;
    }

    public static void interactionDestroy(Interaction interaction) {
        if ( interaction == null ) {
            return;
        }

        totalInteractions--;
        if ( interaction.receiverElement != null && interaction.receiverElement.isCluster() ) {
            if ( interaction.sourceElement != null && interaction.sourceElement.isCluster() ) {
                ccInteractions--;
            }
            else {
                scInteractions--;
            }
        }
        else {
            if ( interaction.sourceElement != null && interaction.sourceElement.isCluster() ) {
                csInteractions--;
            }
            else {
                ssInteractions--;
            }
        }
    }

    public static Interaction interactionDuplicate(Interaction interaction) {
        if ( interaction == null ) {
            return null;
        }
        return new Interaction(
            interaction.receiverElement,
            interaction.sourceElement,
            interaction.K,
            interaction.deltaK,
            interaction.numberOfBasisFunctionsOnReceiver,
            interaction.numberOfBasisFunctionsOnSource,
            interaction.numberOfReceiverCubaturePositions,
            interaction.visibility
        );
    }
}
