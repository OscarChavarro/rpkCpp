package vsdk.toolkit.render.jogl.visualDebugTools;

import vsdk.toolkit.galerkin.GalerkinRadianceMethod;
import java.awt.event.MouseEvent;
import java.util.ArrayList;
import vsdk.toolkit.common.linealAlgebra.Matrix4x4d;
import vsdk.toolkit.common.linealAlgebra.Numeric;
import vsdk.toolkit.environment.geometry.element.Ray;
import vsdk.toolkit.common.linealAlgebra.Vector3Dd;
import vsdk.toolkit.environment.geometry.elements.RayHitFlag;
import vsdk.toolkit.render.jogl.RenderOpenGL;
import vsdk.toolkit.scene.Camera;
import vsdk.toolkit.scene.Scene;
import vsdk.toolkit.skin.RpkAxisAlignedBoundingBox;
import vsdk.toolkit.environment.geometry.elements.ElementTypes;
import vsdk.toolkit.skin.RpkGeometry;
import vsdk.toolkit.environment.geometry.elements.Patch;
import vsdk.toolkit.environment.geometry.elements.RayExclusionSet;
import vsdk.toolkit.environment.geometry.elements.RpkRayHit;

public final class GlutDebugToolsMouseControl {
    private static final int DRAG_START_THRESHOLD_PIXELS = 3;
    private static final double DRAG_ROTATION_DEGREES_PER_PIXEL = 0.25;
    private static final double DEGREES_TO_RADIANS = (double)Math.PI / 180.0;

    private static boolean leftButtonDown = false;
    private static boolean dragging = false;
    private static boolean pressWithShift = false;
    private static int pressX = 0;
    private static int pressY = 0;
    private static int lastX = 0;
    private static int lastY = 0;

    private GlutDebugToolsMouseControl() {
    }

    private static boolean applyPatchSelection(int pickedPatchIndex, int[] selectedPatch) {
        if ( selectedPatch == null || selectedPatch.length == 0 ) {
            return false;
        }

        if ( pickedPatchIndex < 0 ) {
            if ( selectedPatch[0] == -1 ) {
                return false;
            }
            selectedPatch[0] = -1;
            return true;
        }

        if ( selectedPatch[0] == pickedPatchIndex ) {
            selectedPatch[0] = -1;
            return true;
        }

        selectedPatch[0] = pickedPatchIndex;
        return true;
    }

    private static void syncCameraToViewport(GlutDebugToolsModel model) {
        if ( model.scene == null || model.scene.camera == null ) {
            return;
        }
        if ( model.width <= 0 || model.height <= 0 ) {
            return;
        }

        Camera camera = model.scene.camera;
        if ( camera.xSize == model.width
             && camera.ySize == model.height
             && camera.pixelWidth > Numeric.EPSILON
             && camera.pixelHeight > Numeric.EPSILON ) {
            return;
        }

        camera.set(
            camera.eyePosition,
            camera.lookPosition,
            camera.upDirection,
            camera.fieldOfVision,
            model.width,
            model.height,
            camera.background);
    }

    private static int clampCoord(int value, int maxExclusive) {
        if ( maxExclusive <= 0 ) {
            return 0;
        }
        if ( value < 0 ) {
            return 0;
        }
        if ( value >= maxExclusive ) {
            return maxExclusive - 1;
        }
        return value;
    }

    private static void clampSelectedHierarchyLevel(GlutDebugToolsModel model) {
        if ( model.debugState == null ) {
            return;
        }

        int maxHierarchyLevel = GlutDebugPatchHierarchy.maxLevelForSelectedPatch(
            model.scene,
            model.debugState.primarySelectedPatch);

        if ( model.selectedHierarchyLevel < 0 ) {
            model.selectedHierarchyLevel = 0;
        }
        if ( model.selectedHierarchyLevel > maxHierarchyLevel ) {
            model.selectedHierarchyLevel = maxHierarchyLevel;
        }
    }

    private static Vector3Dd sceneRotationPivot(Scene scene) {
        if ( scene == null ) {
            return new Vector3Dd(0.0, 0.0, 0.0);
        }

        if ( scene.clusteredRootGeometry != null && scene.clusteredRootGeometry.bounded ) {
            return scene.clusteredRootGeometry.boundingBox.center();
        }

        if ( scene.geometryList != null && !scene.geometryList.isEmpty() ) {
            RpkAxisAlignedBoundingBox sceneBounds = new RpkAxisAlignedBoundingBox();
            RpkGeometry.listBounds(scene.geometryList, sceneBounds);
            return sceneBounds.center();
        }

        return new Vector3Dd(0.0, 0.0, 0.0);
    }

    private static void viewportAxesInWorld(Scene scene, Vector3Dd[] axisU, Vector3Dd[] axisV) {
        if ( axisU == null || axisV == null ) {
            return;
        }

        axisU[0] = new Vector3Dd(1.0, 0.0, 0.0);
        axisV[0] = new Vector3Dd(0.0, 1.0, 0.0);

        if ( scene == null || scene.camera == null ) {
            return;
        }

        Camera camera = scene.camera;

        Vector3Dd cameraU = new Vector3Dd();
        cameraU = camera.X;
        Vector3Dd cameraV = new Vector3Dd();
        cameraV = camera.Y;

        if ( cameraU.lengthSquared() < Numeric.EPSILON || cameraV.lengthSquared() < Numeric.EPSILON ) {
            Vector3Dd viewDirection = new Vector3Dd();
            viewDirection = camera.lookPosition.subtract(camera.eyePosition);
            if ( viewDirection.lengthSquared() < Numeric.EPSILON ) {
                return;
            }
            viewDirection = viewDirection.normalized(Numeric.EPSILON);

            Vector3Dd upDirection = new Vector3Dd();
            upDirection = camera.upDirection;
            if ( upDirection.lengthSquared() < Numeric.EPSILON ) {
                upDirection = new Vector3Dd(0.0, 0.0, 1.0);
            }
            else {
                upDirection = upDirection.normalized(Numeric.EPSILON);
            }

            cameraU = viewDirection.crossProduct(upDirection);
            if ( cameraU.lengthSquared() < Numeric.EPSILON ) {
                upDirection = new Vector3Dd(0.0, 1.0, 0.0);
                cameraU = viewDirection.crossProduct(upDirection);
            }
            if ( cameraU.lengthSquared() < Numeric.EPSILON ) {
                return;
            }

            cameraU = cameraU.normalized(Numeric.EPSILON);
            cameraV = viewDirection.crossProduct(cameraU);
        }

        if ( cameraU.lengthSquared() < Numeric.EPSILON || cameraV.lengthSquared() < Numeric.EPSILON ) {
            return;
        }

        cameraU = cameraU.normalized(Numeric.EPSILON);
        cameraV = cameraV.normalized(Numeric.EPSILON);
        axisU[0] = cameraU;
        axisV[0] = cameraV;
    }

    /** Returns the rotated vector (the vector itself when nothing is to be done) */
    private static Vector3Dd rotateVectorAroundAxis(Vector3Dd vector, Vector3Dd axis, double angleDegrees) {
        if ( vector == null || axis == null || axis.lengthSquared() < Numeric.EPSILON || angleDegrees == 0.0 ) {
            return vector;
        }

        final Matrix4x4d rotation = new Matrix4x4d().axisRotation(angleDegrees * DEGREES_TO_RADIANS, axis);
        return rotation.withoutTranslation().multiply(vector);
    }

    // Java adaptation: the C++ port changes the ray through a pointer; Java rays
    // are immutable, so the changed ray is returned
    private static Ray applyInverseDebugRotationToRay(GlutDebugToolsModel model, Ray ray) {
        if ( ray == null || model.debugState == null ) {
            return ray;
        }

        double angleAroundU = model.debugState.angleAroundViewportU;
        double angleAroundV = model.debugState.angleAroundViewportV;
        if ( angleAroundU == 0.0 && angleAroundV == 0.0 ) {
            return ray;
        }

        Vector3Dd pivot = sceneRotationPivot(model.scene);
        Vector3Dd[] axisU = {new Vector3Dd()};
        Vector3Dd[] axisV = {new Vector3Dd()};
        viewportAxesInWorld(model.scene, axisU, axisV);

        Vector3Dd translatedOrigin = ray.getOrigin().subtract(pivot);
        translatedOrigin = rotateVectorAroundAxis(translatedOrigin, axisU[0], -angleAroundU);
        translatedOrigin = rotateVectorAroundAxis(translatedOrigin, axisV[0], -angleAroundV);
        ray = ray.withOrigin(pivot.add(translatedOrigin));

        ray = ray.withDirection(rotateVectorAroundAxis(ray.getDirection(), axisU[0], -angleAroundU));
        ray = ray.withDirection(rotateVectorAroundAxis(ray.getDirection(), axisV[0], -angleAroundV));
        ray = ray.withDirection(ray.getDirection().normalized(Numeric.EPSILON));
        return ray;
    }

    // Java adaptation: returns the ray the C++ port writes through its pointer
    private static Ray buildPickRay(GlutDebugToolsModel model, int x, int y) {
        Ray ray = new Ray();
        ray = ray.withOrigin(new Vector3Dd(0.0, 0.0, 0.0));
        ray = ray.withDirection(new Vector3Dd(0.0, 0.0, 1.0));

        if ( model.scene == null || model.scene.camera == null || model.width <= 0 || model.height <= 0 ) {
            return ray;
        }

        syncCameraToViewport(model);

        Camera camera = model.scene.camera;
        RenderOpenGL.renderGetNearFar(camera, model.scene.geometryList);

        double nearDistance = camera.near;
        if ( nearDistance < Numeric.EPSILON ) {
            if ( camera.viewDistance > Numeric.EPSILON ) {
                nearDistance = camera.viewDistance / 100.0;
            }
            else {
                nearDistance = 0.1;
            }
        }

        int pixelX = clampCoord(x, model.width);
        int pixelY = clampCoord(y, model.height);

        double normalizedU =
            (2.0 * (((double)pixelX + 0.5) / (double)model.width)) - 1.0;
        double normalizedV =
            (2.0 * (((double)pixelY + 0.5) / (double)model.height)) - 1.0;

        double nearPlaneU = normalizedU * camera.pixelWidthTangent * nearDistance;
        double nearPlaneV = normalizedV * camera.pixelHeightTangent * nearDistance;

        double uAtUnitDepth = nearPlaneU / nearDistance;
        double vAtUnitDepth = nearPlaneV / nearDistance;

        ray = ray.withOrigin(new Vector3Dd(camera.eyePosition.x(), camera.eyePosition.y(), camera.eyePosition.z()));
        ray = ray.withDirection(camera.Z.add(camera.X.multiply(uAtUnitDepth)).add(camera.Y.multiply(vAtUnitDepth)));
        ray = ray.withDirection(ray.getDirection().normalized(Numeric.EPSILON));

        return applyInverseDebugRotationToRay(model, ray);
    }

    private static boolean pickPatchAtMousePosition(GlutDebugToolsModel model, int x, int y, int[] patchIndex) {
        if ( patchIndex == null || patchIndex.length == 0 ) {
            return false;
        }
        patchIndex[0] = -1;

        if ( model.scene == null || model.scene.patchList == null ) {
            return false;
        }

        Ray ray = buildPickRay(model, x, y);

        ArrayList<PatchHitCandidate> hitCandidates = new ArrayList<>();

        for ( int i = 0; i < model.scene.patchList.size(); i++ ) {
            Patch patch = model.scene.patchList.get(i);
            if ( patch == null || GalerkinRadianceMethod.galerkinState.boundElement(patch) == null ) {
                continue;
            }
            if ( GalerkinRadianceMethod.galerkinState.boundElement(patch).className != ElementTypes.ELEMENT_GALERKIN ) {
                continue;
            }

            double[] maxDistance = new double[] {Double.MAX_VALUE};
            RpkRayHit hit = new RpkRayHit();
            final RayExclusionSet noExclusions = new RayExclusionSet();
            RpkRayHit intersection = patch.intersect(
                ray,
                Numeric.EPSILON,
                maxDistance,
                RayHitFlag.FRONT | RayHitFlag.BACK,
                hit,
                noExclusions);
            if ( intersection != null ) {
                boolean frontFacing = (hit.getFlags() & RayHitFlag.FRONT) != 0;
                hitCandidates.add(new PatchHitCandidate(i, maxDistance[0], frontFacing));
            }
        }

        if ( hitCandidates.isEmpty() ) {
            return false;
        }

        double nearestFrontDistance = Double.MAX_VALUE;
        int nearestFrontPatchIndex = -1;
        double nearestBackDistance = Double.MAX_VALUE;
        int nearestBackPatchIndex = -1;

        for ( int i = 0; i < hitCandidates.size(); i++ ) {
            PatchHitCandidate candidate = hitCandidates.get(i);
            if ( candidate.frontFacing ) {
                if ( candidate.distance < nearestFrontDistance ) {
                    nearestFrontDistance = candidate.distance;
                    nearestFrontPatchIndex = candidate.patchIndex;
                }
            }
            else if ( candidate.distance < nearestBackDistance ) {
                nearestBackDistance = candidate.distance;
                nearestBackPatchIndex = candidate.patchIndex;
            }
        }

        if ( nearestFrontPatchIndex >= 0 ) {
            patchIndex[0] = nearestFrontPatchIndex;
            return true;
        }

        if ( nearestBackPatchIndex < 0 ) {
            return false;
        }

        patchIndex[0] = nearestBackPatchIndex;
        return true;
    }

    public static boolean handleMouseButton(int button, int state, int x, int y, boolean shiftDown, GlutDebugToolsModel model) {
        if ( button != MouseEvent.BUTTON1 ) {
            return false;
        }

        int clampedX = clampCoord(x, model.width);
        int clampedY = clampCoord(y, model.height);

        if ( state == MouseEvent.MOUSE_PRESSED ) {
            pressWithShift = shiftDown;
            leftButtonDown = true;
            dragging = false;
            pressX = clampedX;
            pressY = clampedY;
            lastX = clampedX;
            lastY = clampedY;
            return false;
        }

        if ( state != MouseEvent.MOUSE_RELEASED || !leftButtonDown ) {
            return false;
        }

        leftButtonDown = false;
        boolean shouldSelectPatch = !dragging;
        dragging = false;

        if ( !shouldSelectPatch || model.debugState == null ) {
            return false;
        }

        int[] patchIndex = new int[] {-1};
        if ( !pickPatchAtMousePosition(model, clampedX, clampedY, patchIndex) ) {
            patchIndex[0] = -1;
        }

        int[] targetSelection = new int[] {model.debugState.primarySelectedPatch};
        boolean isPrimarySelection = !pressWithShift;
        if ( pressWithShift ) {
            targetSelection[0] = model.debugState.selectedSelectedPatch;
        }

        if ( !applyPatchSelection(patchIndex[0], targetSelection) ) {
            return false;
        }

        if ( pressWithShift ) {
            model.debugState.selectedSelectedPatch = targetSelection[0];
        }
        else {
            model.debugState.primarySelectedPatch = targetSelection[0];
        }

        if ( isPrimarySelection ) {
            clampSelectedHierarchyLevel(model);
        }

        return true;
    }

    public static boolean handleMouseMotion(int x, int y, GlutDebugToolsModel model) {
        if ( !leftButtonDown ) {
            return false;
        }

        int clampedX = clampCoord(x, model.width);
        int clampedY = clampCoord(y, model.height);

        int deltaX = clampedX - lastX;
        int deltaY = clampedY - lastY;
        lastX = clampedX;
        lastY = clampedY;

        if ( deltaX == 0 && deltaY == 0 ) {
            return false;
        }

        if ( !dragging ) {
            int fromPressX = Math.abs(clampedX - pressX);
            int fromPressY = Math.abs(clampedY - pressY);
            if ( fromPressX >= DRAG_START_THRESHOLD_PIXELS || fromPressY >= DRAG_START_THRESHOLD_PIXELS ) {
                dragging = true;
            }
        }

        if ( !dragging || model.debugState == null ) {
            return false;
        }

        model.debugState.angleAroundViewportV -= (double)deltaX * DRAG_ROTATION_DEGREES_PER_PIXEL;
        model.debugState.angleAroundViewportU += (double)deltaY * DRAG_ROTATION_DEGREES_PER_PIXEL;
        return true;
    }
}
