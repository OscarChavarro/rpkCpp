#ifndef __POLYHEDRAL_BOUNDED_SOLID_VALIDATION_ENGINE__
#define __POLYHEDRAL_BOUNDED_SOLID_VALIDATION_ENGINE__

#include "java/lang/IllegalArgumentException.h"
#include "java/util/concurrent/atomic/AtomicLong.h"

#include "java/lang/String.h"
class PolyhedralBoundedSolid;

/**
Orchestrates validation passes that preserve the half-edge representation of
[MANT1988].10 and the geometric consistency conditions used by chapters
[MANT1988].13 and [MANT1988].15.
*/
class PolyhedralBoundedSolidValidationEngine {
public:
    /**
    Runs a lightweight validation pass aimed at intermediate models that still
    need to respect the face/loop/half-edge structure of [MANT1988].10.2.1 and
    the planar-face assumptions of [MANT1988].13.1.
    @param solid solid to validate; its validation state is updated
    @return true when the solid is valid
    */
    static bool validateIntermediate(PolyhedralBoundedSolid* solid);

    /**
    Validates both operands of a boolean operation before the pipeline
    starts: validateIntermediate on each solid, no coincident vertices, and
    unique face and vertex ids. Returns true only when both solids pass all
    checks; the reasons go to `msg`.

    It only reads. Until plan-stage11 Phase C it welded the coincident
    vertices it found and validated again, so validating could change the
    operands. The set operator now prepares its own copies of the operands
    before calling this (see
    `PolyhedralBoundedSolidTopologyEditing::weldCoincidentVertices`).
    @param solidA first operand
    @param solidB second operand
    @param msg receives failure details, can be null
    @return true only when both solids pass all checks
    */
    static bool validateBooleanInputs(PolyhedralBoundedSolid* solidA,
        PolyhedralBoundedSolid* solidB, java::String* msg);

    /**
    Runs a stricter validation pass that additionally enforces the non-self-
    intersection expectations stated for valid boundary models in
    [MANT1988].15.2.
    @param solid solid to validate; its validation state is updated
    @return true when the solid is valid
    */
    static bool validateStrict(PolyhedralBoundedSolid* solid);

    /**
    Runs strict validation and appends actionable failure detail to `msg`.
    @param solid solid to validate; its validation state is updated
    @param msg receives failure details, can be null
    @return true when the solid is valid
    */
    static bool validateStrict(PolyhedralBoundedSolid* solid, java::String* msg);

    /**
    @return number of strict validation invocations, a diagnostic counter
    */
    static long getStrictValidationInvocationCount();

    /**
    Resets the strict-validation diagnostic counter.
    */
    static void resetStrictValidationInvocationCount();

    /**
    Decides whether two solids represent the same geometric set within
    tolerance: same cardinality of vertices, edges and faces, same bounding
    box and a pairwise vertex position coincidence. Used by setOp to detect
    the degenerate case `A op A_clone`.
    @param a first operand
    @param b second operand
    @param tolerance vertex coincidence epsilon
    @return true if the two solids are interchangeable as boolean operands
    */
    static bool areGeometricallyIdentical(PolyhedralBoundedSolid* a,
        PolyhedralBoundedSolid* b, double tolerance);

private:
    static java::AtomicLong strictValidationInvocationCount;

    static bool validateBooleanInput(const java::String& name,
        PolyhedralBoundedSolid* solid, java::String* msg);
    PolyhedralBoundedSolidValidationEngine();
};

#endif
