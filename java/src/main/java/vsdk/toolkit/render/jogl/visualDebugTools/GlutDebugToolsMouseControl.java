package vsdk.toolkit.render.jogl.visualDebugTools;

import java.awt.event.MouseEvent;
import java.util.ArrayList;
import vsdk.toolkit.common.linealAlgebra.Matrix4x4;
import vsdk.toolkit.common.linealAlgebra.Numeric;
import vsdk.toolkit.common.linealAlgebra.Ray;
import vsdk.toolkit.common.linealAlgebra.Vector3D;
import vsdk.toolkit.skin.RayHitFlag;
import vsdk.toolkit.render.jogl.RenderOpenGL;
import vsdk.toolkit.scene.Camera;
import vsdk.toolkit.scene.Scene;
import vsdk.toolkit.skin.BoundingBox;
import vsdk.toolkit.skin.ElementTypes;
import vsdk.toolkit.skin.Geometry;
import vsdk.toolkit.skin.Patch;
import vsdk.toolkit.skin.RayHit;

public final class GlutDebugToolsMouseControl {
    private static final int DRAG_START_THRESHOLD_PIXELS = 3;
    private static final float DRAG_ROTATION_DEGREES_PER_PIXEL = 0.25f;
    private static final float DEGREES_TO_RADIANS = (float)Math.PI / 180.0f;

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
             && camera.pixelWidth > Numeric.EPSILON_FLOAT
             && camera.pixelHeight > Numeric.EPSILON_FLOAT ) {
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

    private static Vector3D sceneRotationPivot(Scene scene) {
        if ( scene == null ) {
            return new Vector3D(0.0f, 0.0f, 0.0f);
        }

        if ( scene.clusteredRootGeometry != null && scene.clusteredRootGeometry.bounded ) {
            return scene.clusteredRootGeometry.boundingBox.center();
        }

        if ( scene.geometryList != null && !scene.geometryList.isEmpty() ) {
            BoundingBox sceneBounds = new BoundingBox();
            Geometry.listBounds(scene.geometryList, sceneBounds);
            return sceneBounds.center();
        }

        return new Vector3D(0.0f, 0.0f, 0.0f);
    }

    private static void viewportAxesInWorld(Scene scene, Vector3D axisU, Vector3D axisV) {
        if ( axisU == null || axisV == null ) {
            return;
        }

        axisU.set(1.0f, 0.0f, 0.0f);
        axisV.set(0.0f, 1.0f, 0.0f);

        if ( scene == null || scene.camera == null ) {
            return;
        }

        Camera camera = scene.camera;

        Vector3D cameraU = new Vector3D();
        cameraU.copy(camera.X);
        Vector3D cameraV = new Vector3D();
        cameraV.copy(camera.Y);

        if ( cameraU.norm2() < Numeric.EPSILON_FLOAT || cameraV.norm2() < Numeric.EPSILON_FLOAT ) {
            Vector3D viewDirection = new Vector3D();
            viewDirection.subtraction(camera.lookPosition, camera.eyePosition);
            if ( viewDirection.norm2() < Numeric.EPSILON_FLOAT ) {
                return;
            }
            viewDirection.normalize(Numeric.EPSILON_FLOAT);

            Vector3D upDirection = new Vector3D();
            upDirection.copy(camera.upDirection);
            if ( upDirection.norm2() < Numeric.EPSILON_FLOAT ) {
                upDirection.set(0.0f, 0.0f, 1.0f);
            }
            else {
                upDirection.normalize(Numeric.EPSILON_FLOAT);
            }

            cameraU.crossProduct(viewDirection, upDirection);
            if ( cameraU.norm2() < Numeric.EPSILON_FLOAT ) {
                upDirection.set(0.0f, 1.0f, 0.0f);
                cameraU.crossProduct(viewDirection, upDirection);
            }
            if ( cameraU.norm2() < Numeric.EPSILON_FLOAT ) {
                return;
            }

            cameraU.normalize(Numeric.EPSILON_FLOAT);
            cameraV.crossProduct(viewDirection, cameraU);
        }

        if ( cameraU.norm2() < Numeric.EPSILON_FLOAT || cameraV.norm2() < Numeric.EPSILON_FLOAT ) {
            return;
        }

        cameraU.normalize(Numeric.EPSILON_FLOAT);
        cameraV.normalize(Numeric.EPSILON_FLOAT);
        axisU.copy(cameraU);
        axisV.copy(cameraV);
    }

    private static void rotateVectorAroundAxis(Vector3D vector, Vector3D axis, float angleDegrees) {
        if ( vector == null || axis == null || axis.norm2() < Numeric.EPSILON_FLOAT || angleDegrees == 0.0f ) {
            return;
        }

        Matrix4x4 rotation = Matrix4x4.createRotationMatrix(angleDegrees * DEGREES_TO_RADIANS, axis);
        Vector3D rotated = new Vector3D();
        rotation.transformPoint3D(vector, rotated);
        vector.copy(rotated);
    }

    private static void applyInverseDebugRotationToRay(GlutDebugToolsModel model, Ray ray) {
        if ( ray == null || model.debugState == null ) {
            return;
        }

        float angleAroundU = model.debugState.angleAroundViewportU;
        float angleAroundV = model.debugState.angleAroundViewportV;
        if ( angleAroundU == 0.0f && angleAroundV == 0.0f ) {
            return;
        }

        Vector3D pivot = sceneRotationPivot(model.scene);
        Vector3D axisU = new Vector3D();
        Vector3D axisV = new Vector3D();
        viewportAxesInWorld(model.scene, axisU, axisV);

        Vector3D translatedOrigin = new Vector3D();
        translatedOrigin.subtraction(ray.position, pivot);
        rotateVectorAroundAxis(translatedOrigin, axisU, -angleAroundU);
        rotateVectorAroundAxis(translatedOrigin, axisV, -angleAroundV);
        ray.position.addition(pivot, translatedOrigin);

        rotateVectorAroundAxis(ray.direction, axisU, -angleAroundU);
        rotateVectorAroundAxis(ray.direction, axisV, -angleAroundV);
        ray.direction.normalize(Numeric.EPSILON_FLOAT);
    }

    private static void buildPickRay(GlutDebugToolsModel model, int x, int y, Ray ray) {
        if ( ray == null ) {
            return;
        }

        ray.position.set(0.0f, 0.0f, 0.0f);
        ray.direction.set(0.0f, 0.0f, 1.0f);

        if ( model.scene == null || model.scene.camera == null || model.width <= 0 || model.height <= 0 ) {
            return;
        }

        syncCameraToViewport(model);

        Camera camera = model.scene.camera;
        RenderOpenGL.renderGetNearFar(camera, model.scene.geometryList);

        float nearDistance = camera.near;
        if ( nearDistance < Numeric.EPSILON_FLOAT ) {
            if ( camera.viewDistance > Numeric.EPSILON_FLOAT ) {
                nearDistance = camera.viewDistance / 100.0f;
            }
            else {
                nearDistance = 0.1f;
            }
        }

        int pixelX = clampCoord(x, model.width);
        int pixelY = clampCoord(y, model.height);

        float normalizedU =
            (2.0f * (((float)pixelX + 0.5f) / (float)model.width)) - 1.0f;
        float normalizedV =
            (2.0f * (((float)pixelY + 0.5f) / (float)model.height)) - 1.0f;

        float nearPlaneU = normalizedU * camera.pixelWidthTangent * nearDistance;
        float nearPlaneV = normalizedV * camera.pixelHeightTangent * nearDistance;

        float uAtUnitDepth = nearPlaneU / nearDistance;
        float vAtUnitDepth = nearPlaneV / nearDistance;

        ray.position = new Vector3D(camera.eyePosition.x, camera.eyePosition.y, camera.eyePosition.z);
        ray.direction.combine3(camera.Z, uAtUnitDepth, camera.X, vAtUnitDepth, camera.Y);
        ray.direction.normalize(Numeric.EPSILON_FLOAT);

        applyInverseDebugRotationToRay(model, ray);
    }

    private static boolean pickPatchAtMousePosition(GlutDebugToolsModel model, int x, int y, int[] patchIndex) {
        if ( patchIndex == null || patchIndex.length == 0 ) {
            return false;
        }
        patchIndex[0] = -1;

        if ( model.scene == null || model.scene.patchList == null ) {
            return false;
        }

        Ray ray = new Ray();
        buildPickRay(model, x, y, ray);

        ArrayList<PatchHitCandidate> hitCandidates = new ArrayList<>();

        for ( int i = 0; i < model.scene.patchList.size(); i++ ) {
            Patch patch = model.scene.patchList.get(i);
            if ( patch == null || patch.radianceData == null ) {
                continue;
            }
            if ( patch.radianceData.className != ElementTypes.ELEMENT_GALERKIN ) {
                continue;
            }

            float[] maxDistance = new float[] {Numeric.HUGE_FLOAT_VALUE};
            RayHit hit = new RayHit();
            RayHit intersection = patch.intersect(
                ray,
                Numeric.EPSILON_FLOAT,
                maxDistance,
                RayHitFlag.FRONT | RayHitFlag.BACK,
                hit);
            if ( intersection != null ) {
                boolean frontFacing = (hit.getFlags() & RayHitFlag.FRONT) != 0;
                hitCandidates.add(new PatchHitCandidate(i, maxDistance[0], frontFacing));
            }
        }

        if ( hitCandidates.isEmpty() ) {
            return false;
        }

        float nearestFrontDistance = Numeric.HUGE_FLOAT_VALUE;
        int nearestFrontPatchIndex = -1;
        float nearestBackDistance = Numeric.HUGE_FLOAT_VALUE;
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

        model.debugState.angleAroundViewportV -= (float)deltaX * DRAG_ROTATION_DEGREES_PER_PIXEL;
        model.debugState.angleAroundViewportU += (float)deltaY * DRAG_ROTATION_DEGREES_PER_PIXEL;
        return true;
    }
}
