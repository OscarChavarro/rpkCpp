//= References:                                                             =
//= [MANT1988] Mantyla Martti. "An Introduction To Solid Modeling",         =
//=     Computer Science Press, 1988.                                       =

import { StringBuilder } from "../../../../../../java/lang/StringBuilder.js";
import { AtomicLong } from "../../../../../../java/util/concurrent/atomic/AtomicLong.js";
import { VSDK } from "../../../../common/VSDK.js";
import { Logger } from "../../../../common/logging/Logger.js";
import type { PolyhedralBoundedSolid } from "./PolyhedralBoundedSolid.js";
import { PolyhedralBoundedSolidGeometricValidator } from "./PolyhedralBoundedSolidGeometricValidator.js";
import { PolyhedralBoundedSolidNumericPolicy } from "./PolyhedralBoundedSolidNumericPolicy.js";
import { PolyhedralBoundedSolidTopologySummary } from "./PolyhedralBoundedSolidTopologySummary.js";
import { _GeometricCollapsedLaminaStrategy } from "./_GeometricCollapsedLaminaStrategy.js";
import { _GeometricPlanarityStrategy } from "./_GeometricPlanarityStrategy.js";
import { _GeometricStrictFaceIntersectionsStrategy } from "./_GeometricStrictFaceIntersectionsStrategy.js";
import { _GeometricStrictLoopsStrategy } from "./_GeometricStrictLoopsStrategy.js";
import type { _PolyhedralBoundedSolidValidationStrategy } from "./_PolyhedralBoundedSolidValidationStrategy.js";
import { _TopologicalIntegrityStrategy } from "./_TopologicalIntegrityStrategy.js";

/**
Orchestrates validation passes that preserve the half-edge representation of
[MANT1988].10 and the geometric consistency conditions used by chapters
[MANT1988].13 and [MANT1988].15.
*/
export class PolyhedralBoundedSolidValidationEngine {
    private static readonly strictValidationInvocationCount = new AtomicLong();

    private constructor() {}

    /**
    Runs a lightweight validation pass aimed at intermediate models that still
    need to respect the face/loop/half-edge structure of [MANT1988].10.2.1 and
    the planar-face assumptions of [MANT1988].13.1.
    */
    public static validateIntermediate(solid: PolyhedralBoundedSolid): boolean {
        const msg = new StringBuilder();
        let ok = true;
        const numericContext = PolyhedralBoundedSolidNumericPolicy.forSolid(solid);

        const strategies: _PolyhedralBoundedSolidValidationStrategy[] = [];
        strategies.push(new _GeometricPlanarityStrategy());
        strategies.push(new _TopologicalIntegrityStrategy());

        let i: number;
        for (i = 0; i < strategies.length; i++) {
            if (!strategies[i]!.validate(solid, numericContext, msg)) {
                ok = false;
                break;
            }
        }
        solid.setValidationState(ok);
        if (!ok) {
            Logger.reportMessage(
                solid,
                VSDK.WARNING,
                "validateIntermediate",
                "Solid validation test failed!:\n" + msg.toString(),
            );
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
    public static validateBooleanInputs(
        solidA: PolyhedralBoundedSolid,
        solidB: PolyhedralBoundedSolid,
        msg: StringBuilder,
    ): boolean {
        let ok = true;

        if (!PolyhedralBoundedSolidValidationEngine.validateIntermediate(solidA)) {
            msg.append("solidA failed validateIntermediate\n");
            ok = false;
        }
        if (!PolyhedralBoundedSolidValidationEngine.validateIntermediate(solidB)) {
            msg.append("solidB failed validateIntermediate\n");
            ok = false;
        }
        if (!ok) {
            return false;
        }
        ok = PolyhedralBoundedSolidValidationEngine.validateBooleanInput("solidA", solidA, msg) && ok;
        ok = PolyhedralBoundedSolidValidationEngine.validateBooleanInput("solidB", solidB, msg) && ok;
        return ok;
    }

    private static validateBooleanInput(name: string, solid: PolyhedralBoundedSolid, msg: StringBuilder): boolean {
        const context = PolyhedralBoundedSolidNumericPolicy.forSolid(solid);
        const sub = new StringBuilder();
        let ok = true;

        if (!PolyhedralBoundedSolidGeometricValidator.validateNoCoincidentVertices(solid, context, sub)) {
            msg.append(name).append(" has coincident vertices:\n").append(sub.toString());
            ok = false;
        }
        sub.clear();
        if (!PolyhedralBoundedSolidGeometricValidator.validateUniqueFaceAndVertexIds(solid, sub)) {
            msg.append(name).append(" has ID violations:\n").append(sub.toString());
            ok = false;
        }
        return ok;
    }

    /**
    Runs a stricter validation pass that additionally enforces the non-self-
    intersection expectations stated for valid boundary models in
    [MANT1988].15.2.

    Java overload with a message: runs strict validation and appends
    actionable failure detail to {@code msg}. This overload is intended for
    callers that must turn strict validation into a postcondition exception
    instead of relying on log output.
    */
    public static validateStrict(solid: PolyhedralBoundedSolid, msg?: StringBuilder): boolean {
        if (msg === undefined) {
            const newMsg = new StringBuilder();
            return PolyhedralBoundedSolidValidationEngine.validateStrict(solid, newMsg);
        }
        PolyhedralBoundedSolidValidationEngine.strictValidationInvocationCount.incrementAndGet();
        let ok = true;
        const numericContext = PolyhedralBoundedSolidNumericPolicy.forSolid(solid);

        const strategies: _PolyhedralBoundedSolidValidationStrategy[] = [];
        strategies.push(new _GeometricPlanarityStrategy());
        strategies.push(new _TopologicalIntegrityStrategy());
        strategies.push(new _GeometricStrictLoopsStrategy());
        strategies.push(new _GeometricCollapsedLaminaStrategy());
        strategies.push(new _GeometricStrictFaceIntersectionsStrategy());

        let i: number;
        for (i = 0; i < strategies.length; i++) {
            if (!strategies[i]!.validate(solid, numericContext, msg)) {
                ok = false;
                break;
            }
        }
        if (ok) {
            const topology = PolyhedralBoundedSolidTopologySummary.from(solid);
            if (topology.hasUniversalContradiction()) {
                msg.append("  - Global topology contradiction: ").append(topology.toString()).appendChar("\n");
                ok = false;
            }
        }

        solid.setValidationState(ok);
        if (!ok) {
            Logger.reportMessage(
                solid,
                VSDK.WARNING,
                "validateStrict",
                "Solid validation test failed!:\n" + msg.toString(),
            );
        }
        return ok;
    }

    /**
    Diagnostic counter used to verify that default boolean calls do not enter
    the strict face-pair scan or allocate its topology summary.
    */
    public static getStrictValidationInvocationCount(): bigint {
        return PolyhedralBoundedSolidValidationEngine.strictValidationInvocationCount.get();
    }

    /**
    Resets the strict-validation diagnostic counter.
    */
    public static resetStrictValidationInvocationCount(): void {
        PolyhedralBoundedSolidValidationEngine.strictValidationInvocationCount.set(0n);
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
    public static areGeometricallyIdentical(
        a: PolyhedralBoundedSolid | null,
        b: PolyhedralBoundedSolid | null,
        tolerance: number,
    ): boolean {
        if (a === null || b === null) {
            return false;
        }
        if (a.getVerticesList().size() !== b.getVerticesList().size()) {
            return false;
        }
        if (a.getEdgesList().size() !== b.getEdgesList().size()) {
            return false;
        }
        if (a.getPolygonsList().size() !== b.getPolygonsList().size()) {
            return false;
        }
        const ma = a.getMinMax();
        const mb = b.getMinMax();
        for (let i = 0; i < 6; i++) {
            if (Math.abs(ma[i]! - mb[i]!) > tolerance) {
                return false;
            }
        }
        // Pairwise vertex coincidence: every vertex in A has at least
        // one matching counterpart in B (and by cardinality this implies
        // a bijection). O(n²) on vertex count.
        const n = a.getVerticesList().size();
        const matched: boolean[] = new Array<boolean>(n).fill(false);
        for (let i = 0; i < n; i++) {
            const pa = a.getVerticesList().get(i)!.position;
            let found = false;
            for (let j = 0; j < n; j++) {
                if (matched[j]) continue;
                const pb = b.getVerticesList().get(j)!.position;
                if (
                    Math.abs(pa.x() - pb.x()) <= tolerance &&
                    Math.abs(pa.y() - pb.y()) <= tolerance &&
                    Math.abs(pa.z() - pb.z()) <= tolerance
                ) {
                    matched[j] = true;
                    found = true;
                    break;
                }
            }
            if (!found) return false;
        }
        return true;
    }
}
