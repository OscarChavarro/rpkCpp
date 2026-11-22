package vsdk.toolkit.galerkin;

/**
Reference:
[COHE1993] Cohen, M. Wallace, J. "Radiosity and Realistic Image Synthesis",
     Academic Press Professional, 1993.
*/

// See [COHE1993] section 5.3. PARALLEL_JACOBI is Jacobi whose refinement and
// form factors are evaluated by several threads (GalerkinState.numberOfThreads)
// and committed in the serial order, so its result is JACOBI's, bit for bit
public enum GalerkinIterationMethod {
    JACOBI,
    GAUSS_SEIDEL,
    SOUTH_WELL,
    PARALLEL_JACOBI
}
