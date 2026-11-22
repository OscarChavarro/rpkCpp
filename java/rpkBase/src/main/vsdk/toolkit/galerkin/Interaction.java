package vsdk.toolkit.galerkin;

import vsdk.toolkit.common.logging.Logger;

public class Interaction {
    private static int totalInteractions;
    private static int ccInteractions;
    private static int csInteractions;
    private static int scInteractions;
    private static int ssInteractions;

    public GalerkinElement receiverElement;
    public GalerkinElement sourceElement;
    public float[] K; // Coupling coefficient(s), stored top to bottom, left to right
    public float deltaK; // Used for approximation error estimation over the link (always a single coefficient)
    public boolean ownsK;
    public byte numberOfBasisFunctionsOnReceiver;
    public byte numberOfBasisFunctionsOnSource;
    public byte numberOfReceiverCubaturePositions;
    public byte visibility; // 255 for full visibility, 0 for full occlusion

    public Interaction() {
        ownsK = true;
    }

    public Interaction(
        GalerkinElement inReceiverElement,
        GalerkinElement inSourceElement,
        float[] inK,
        float inDeltaK,
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
            Logger.reportMessage(this, Logger.FATAL_ERROR, "interactionCreate", "Not yet implemented for higher order approximations");
        }

        deltaK = inDeltaK;

        countCreated(this);
    }

    /**
    Adds the interaction to the interaction counters
    */
    public static void countCreated(Interaction interaction) {
        totalInteractions++;
        if ( interaction.receiverElement != null && interaction.receiverElement.isCluster() ) {
            if ( interaction.sourceElement != null && interaction.sourceElement.isCluster() ) {
                ccInteractions++;
            }
            else {
                scInteractions++;
            }
        }
        else {
            if ( interaction.sourceElement != null && interaction.sourceElement.isCluster() ) {
                csInteractions++;
            }
            else {
                ssInteractions++;
            }
        }
    }

    /**
    Same as interactionDuplicate, but the copy is not counted: for threads that
    create interactions which a single thread counts later (countCreated)
    */
    public static Interaction uncountedDuplicate(Interaction interaction) {
        Interaction newInteraction = new Interaction();
        newInteraction.ownsK = false; // As the copy constructor of interactionDuplicate leaves it
        newInteraction.receiverElement = interaction.receiverElement;
        newInteraction.sourceElement = interaction.sourceElement;
        newInteraction.numberOfBasisFunctionsOnReceiver = interaction.numberOfBasisFunctionsOnReceiver;
        newInteraction.numberOfBasisFunctionsOnSource = interaction.numberOfBasisFunctionsOnSource;
        newInteraction.numberOfReceiverCubaturePositions = interaction.numberOfReceiverCubaturePositions;
        newInteraction.visibility = interaction.visibility;

        int n = (newInteraction.numberOfBasisFunctionsOnReceiver & 0xFF) * (newInteraction.numberOfBasisFunctionsOnSource & 0xFF);
        if ( n <= 0 ) {
            n = 1;
        }
        newInteraction.K = new float[n];
        if ( interaction.K != null ) {
            System.arraycopy(interaction.K, 0, newInteraction.K, 0, Math.min(interaction.K.length, newInteraction.K.length));
        }

        if ( (newInteraction.numberOfReceiverCubaturePositions & 0xFF) > 1 ) {
            Logger.reportMessage(newInteraction, Logger.FATAL_ERROR, "interactionCreate", "Not yet implemented for higher order approximations");
        }

        newInteraction.deltaK = interaction.deltaK;
        return newInteraction;
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
        if ( interaction.ownsK
             && interaction.K != null
             && ((interaction.numberOfBasisFunctionsOnReceiver & 0xFF) > 1
                 || (interaction.numberOfBasisFunctionsOnSource & 0xFF) > 1) ) {
            interaction.K = null;
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
