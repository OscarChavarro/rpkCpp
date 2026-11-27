//= References:                                                             =
//= [MANT1988] Mantyla Martti. "An Introduction To Solid Modeling",         =
//=     Computer Science Press, 1988.                                       =

package vsdk.toolkit.environment.geometry.volume.polyhedralBoundedSolid;

import java.util.ArrayList;
import java.util.concurrent.atomic.AtomicLong;

import vsdk.toolkit.common.VSDK;
import vsdk.toolkit.common.logging.Logger;

/**
Orchestrates validation passes that preserve the half-edge representation of
[MANT1988].10 and the geometric consistency conditions used by chapters
[MANT1988].13 and [MANT1988].15.
*/
public class PolyhedralBoundedSolidValidationEngine
{
    private static final AtomicLong strictValidationInvocationCount =
        new AtomicLong();

    private PolyhedralBoundedSolidValidationEngine()
    {
    }

    /**
    Runs a lightweight validation pass aimed at intermediate models that still
    need to respect the face/loop/half-edge structure of [MANT1988].10.2.1 and
    the planar-face assumptions of [MANT1988].13.1.
    */
    public static boolean validateIntermediate(PolyhedralBoundedSolid solid)
    {
        StringBuilder msg = new StringBuilder();
        boolean ok = true;
        PolyhedralBoundedSolidNumericPolicy.ToleranceContext numericContext =
            PolyhedralBoundedSolidNumericPolicy.forSolid(solid);

        ArrayList<_PolyhedralBoundedSolidValidationStrategy> strategies =
            new ArrayList<_PolyhedralBoundedSolidValidationStrategy>();
        strategies.add(new _GeometricPlanarityStrategy());
        strategies.add(new _TopologicalIntegrityStrategy());

        int i;
        for ( i = 0; i < strategies.size(); i++ ) {
            if ( !strategies.get(i).validate(solid, numericContext, msg) ) {
                ok = false;
                break;
            }
        }
        solid.setValidationState(ok);
        if ( !ok ) {
            Logger.reportMessage(solid, VSDK.WARNING, "validateIntermediate",
                "Solid validation test failed!:\n" + msg.toString());
        }
        return ok;
    }

    /**
    Validates both operands of a boolean operation before the pipeline
    starts: validateIntermediate on each solid, no coincident vertices, and
    unique face and vertex ids. Returns true only when both solids pass all
    checks; the reasons go to `msg`.

    <p>It only reads. Until plan-stage11 Phase C it welded the coincident
    vertices it found and validated again, so validating could change the
    operands. The set operator now prepares its own copies of the operands
    before calling this (see
    {@link PolyhedralBoundedSolidTopologyEditing#weldCoincidentVertices}).</p>
    */
    public static boolean validateBooleanInputs(
        PolyhedralBoundedSolid solidA,
        PolyhedralBoundedSolid solidB,
        StringBuilder msg)
    {
        boolean ok = true;

        if ( !validateIntermediate(solidA) ) {
            msg.append("solidA failed validateIntermediate\n");
            ok = false;
        }
        if ( !validateIntermediate(solidB) ) {
            msg.append("solidB failed validateIntermediate\n");
            ok = false;
        }
        if ( !ok ) {
            return false;
        }
        ok &= validateBooleanInput("solidA", solidA, msg);
        ok &= validateBooleanInput("solidB", solidB, msg);
        return ok;
    }

    private static boolean validateBooleanInput(String name,
        PolyhedralBoundedSolid solid, StringBuilder msg)
    {
        PolyhedralBoundedSolidNumericPolicy.ToleranceContext context =
            PolyhedralBoundedSolidNumericPolicy.forSolid(solid);
        StringBuilder sub = new StringBuilder();
        boolean ok = true;

        if ( !PolyhedralBoundedSolidGeometricValidator
                .validateNoCoincidentVertices(solid, context, sub) ) {
            msg.append(name).append(" has coincident vertices:\n")
               .append(sub);
            ok = false;
        }
        sub.setLength(0);
        if ( !PolyhedralBoundedSolidGeometricValidator
                .validateUniqueFaceAndVertexIds(solid, sub) ) {
            msg.append(name).append(" has ID violations:\n").append(sub);
            ok = false;
        }
        return ok;
    }

    /**
    Runs a stricter validation pass that additionally enforces the non-self-
    intersection expectations stated for valid boundary models in
    [MANT1988].15.2.
    */
    public static boolean validateStrict(PolyhedralBoundedSolid solid)
    {
        StringBuilder msg = new StringBuilder();
        return validateStrict(solid, msg);
    }

    /**
    Runs strict validation and appends actionable failure detail to
    {@code msg}. This overload is intended for callers that must turn strict
    validation into a postcondition exception instead of relying on log
    output.
    */
    public static boolean validateStrict(
        PolyhedralBoundedSolid solid,
        StringBuilder msg)
    {
        strictValidationInvocationCount.incrementAndGet();
        boolean ok = true;
        PolyhedralBoundedSolidNumericPolicy.ToleranceContext numericContext =
            PolyhedralBoundedSolidNumericPolicy.forSolid(solid);

        ArrayList<_PolyhedralBoundedSolidValidationStrategy> strategies =
            new ArrayList<_PolyhedralBoundedSolidValidationStrategy>();
        strategies.add(new _GeometricPlanarityStrategy());
        strategies.add(new _TopologicalIntegrityStrategy());
        strategies.add(new _GeometricStrictLoopsStrategy());
        strategies.add(new _GeometricCollapsedLaminaStrategy());
        strategies.add(new _GeometricStrictFaceIntersectionsStrategy());

        int i;
        for ( i = 0; i < strategies.size(); i++ ) {
            if ( !strategies.get(i).validate(solid, numericContext, msg) ) {
                ok = false;
                break;
            }
        }
        if ( ok ) {
            PolyhedralBoundedSolidTopologySummary topology =
                PolyhedralBoundedSolidTopologySummary.from(solid);
            if ( topology.hasUniversalContradiction() ) {
                msg.append("  - Global topology contradiction: ")
                    .append(topology).append('\n');
                ok = false;
            }
        }

        solid.setValidationState(ok);
        if ( !ok ) {
            Logger.reportMessage(solid, VSDK.WARNING, "validateStrict",
                "Solid validation test failed!:\n" + msg.toString());
        }
        return ok;
    }

    /**
    Diagnostic counter used to verify that default boolean calls do not enter
    the strict face-pair scan or allocate its topology summary.
    */
    public static long getStrictValidationInvocationCount()
    {
        return strictValidationInvocationCount.get();
    }

    /**
    Resets the strict-validation diagnostic counter.
    */
    public static void resetStrictValidationInvocationCount()
    {
        strictValidationInvocationCount.set(0L);
    }

    /**
    Decides whether two solids represent the same geometric set within
    tolerance — i.e., every vertex in one has a coincident counterpart
    in the other and their topological cardinality matches.

    <p>Used by {@code setOp} to detect the degenerate case
    {@code A op A_clone}, which Mäntylä 1988 does not specify
    explicitly. Without this preflight, the classifier marks every
    face of both solids as "inside the other", and UNION/INTERSECTION
    collapse to ∅ — violating the algebraic identities
    {@code A∪A = A}, {@code A∩A = A}.</p>

    <p>The check is intentionally conservative: it requires same
    cardinality of vertices/edges/faces AND a pairwise coincidence on
    vertex positions within {@code tolerance}. False positives are
    impossible at this granularity (cardinality plus exact-position
    match), and false negatives are acceptable (the pipeline simply
    runs the regular path, which may or may not produce the expected
    output).</p>

    @param a first operand
    @param b second operand
    @param tolerance vertex coincidence epsilon (typically the bigEpsilon
        of either operand's {@code ToleranceContext})
    @return {@code true} if the two solids are interchangeable as boolean
        operands
     */
    public static boolean areGeometricallyIdentical(
        PolyhedralBoundedSolid a,
        PolyhedralBoundedSolid b,
        double tolerance)
    {
        if ( a == null || b == null ) {
            return false;
        }
        if ( a.getVerticesList().size() != b.getVerticesList().size() ) {
            return false;
        }
        if ( a.getEdgesList().size() != b.getEdgesList().size() ) {
            return false;
        }
        if ( a.getPolygonsList().size() != b.getPolygonsList().size() ) {
            return false;
        }
        double[] ma = a.getMinMax();
        double[] mb = b.getMinMax();
        for ( int i = 0; i < 6; i++ ) {
            if ( Math.abs(ma[i] - mb[i]) > tolerance ) {
                return false;
            }
        }
        // Pairwise vertex coincidence: every vertex in A has at least
        // one matching counterpart in B (and by cardinality this implies
        // a bijection). O(n²) on vertex count.
        int n = a.getVerticesList().size();
        boolean[] matched = new boolean[n];
        for ( int i = 0; i < n; i++ ) {
            vsdk.toolkit.common.linealAlgebra.Vector3Dd pa =
                a.getVerticesList().get(i).position;
            boolean found = false;
            for ( int j = 0; j < n; j++ ) {
                if ( matched[j] ) continue;
                vsdk.toolkit.common.linealAlgebra.Vector3Dd pb =
                    b.getVerticesList().get(j).position;
                if ( Math.abs(pa.x() - pb.x()) <= tolerance
                     && Math.abs(pa.y() - pb.y()) <= tolerance
                     && Math.abs(pa.z() - pb.z()) <= tolerance ) {
                    matched[j] = true;
                    found = true;
                    break;
                }
            }
            if ( !found ) return false;
        }
        return true;
    }
}
