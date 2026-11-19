package vsdk.toolkit.galerkin.processing;

import java.util.ArrayList;
import vsdk.toolkit.common.memoryManagement.MemoryPool;
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
    private static final int K_SIZE = GalerkinBasis.MAX_BASIS_SIZE * GalerkinBasis.MAX_BASIS_SIZE;
    private static final MemoryPool<float[]> scalarPool = new MemoryPool<>(() -> new float[1]);
    private static final MemoryPool<float[]> matrixPool = new MemoryPool<>(() -> new float[K_SIZE]);
    private static boolean poolsInitialized = false;

    private static void ensurePoolsInitialized() {
        if (!poolsInitialized) {
            scalarPool.init(1024);
            matrixPool.init(2L * 1024L * 1024L);
            poolsInitialized = true;
        }
    }

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

        ensurePoolsInitialized();
        // Assume no light transport (overlapping receiver and source)
        int n = receiverElement.basisSize * sourceElement.basisSize;
        float[] K;
        boolean kFromPool = false;
        if ( n == 1 ) {
            K = scalarPool.allocate(1);
            if (K == null) {
                if (scalarPool.expand(1024)) {
                    K = scalarPool.allocate(1);
                }
            }
            if (K != null) {
                kFromPool = true;
            }
            else {
                K = new float[1];
            }
            K[0] = 0.0f;
        } else {
            K = matrixPool.allocate(K_SIZE);
            if (K == null) {
                if (matrixPool.expand(K_SIZE * 128)) {
                    K = matrixPool.allocate(K_SIZE);
                }
            }
            if (K != null) {
                kFromPool = true;
            }
            else {
                K = new float[K_SIZE];
            }
            if ( n <= 0 ) {
                n = 1;
            }
            for ( int i = 0; i < n; i++ ) {
                K[i] = 0.0f;
            }
        }
        Interaction newLink = new Interaction(
            receiverElement,
            sourceElement,
            K,
            Numeric.HUGE_FLOAT_VALUE, // Huge value error on the form factor
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

        if (kFromPool) {
            if (receiverElement.basisSize * sourceElement.basisSize == 1) {
                scalarPool.free(1);
            }
            else {
                matrixPool.free(K_SIZE);
            }
        }
    }
}
