package vsdk.toolkit.galerkin.processing;

import java.util.ArrayList;
import vsdk.toolkit.common.logging.Logger;
import vsdk.toolkit.galerkin.GalerkinBasis;
import vsdk.toolkit.galerkin.GalerkinElement;
import vsdk.toolkit.galerkin.GalerkinIterationMethod;
import vsdk.toolkit.galerkin.GalerkinRole;
import vsdk.toolkit.galerkin.GalerkinShaftCullMode;
import vsdk.toolkit.galerkin.GalerkinState;
import vsdk.toolkit.galerkin.Interaction;
import vsdk.toolkit.galerkin.Shaft;
import vsdk.toolkit.scene.Polygon;
import vsdk.toolkit.scene.Scene;
import vsdk.toolkit.skin.BoundingBox;
import vsdk.toolkit.skin.ElementFlags;
import vsdk.toolkit.skin.Geometry;
import vsdk.toolkit.skin.Patch;

public class LinkingSimpleStrategy {
    private static final ArrayList<float[]> coefficientsPool = new ArrayList<>();

    private static float[] borrowKBuffer() {
        int n = coefficientsPool.size();
        if ( n > 0 ) {
            return coefficientsPool.remove(n - 1);
        }
        return new float[GalerkinBasis.MAX_BASIS_SIZE * GalerkinBasis.MAX_BASIS_SIZE];
    }

    private static void returnKBuffer(float[] buffer) {
        if ( buffer != null && buffer.length == GalerkinBasis.MAX_BASIS_SIZE * GalerkinBasis.MAX_BASIS_SIZE ) {
            coefficientsPool.add(buffer);
        }
    }

    private static void createInitialLink(
        Scene scene,
        GalerkinState galerkinState,
        GalerkinRole role,
        ArrayList<Geometry>[] candidateList,
        GalerkinElement topElement,
        BoundingBox topLevelBoundingBox,
        Patch patch)
    {
        if ( scene == null || galerkinState == null || candidateList == null || candidateList.length == 0
             || topElement == null || patch == null || topElement.patch == null ) {
            return;
        }

        if ( !patch.facing(topElement.patch) ) {
            return;
        }

        GalerkinElement receiverElement;
        GalerkinElement sourceElement;
        GalerkinElement topLevelElement = GalerkinElement.fromPatch(patch);

        switch ( role ) {
            case SOURCE:
                receiverElement = topLevelElement;
                sourceElement = topElement;
                break;
            case RECEIVER:
                receiverElement = topElement;
                sourceElement = topLevelElement;
                break;
            default:
                Logger.fatal(2, "createInitialLink", "Impossible element role");
                return;
        }

        ArrayList<Geometry> oldCandidateList = candidateList[0];
        if ( (galerkinState.exactVisibility != 0
              || galerkinState.shaftCullMode == GalerkinShaftCullMode.ALWAYS_DO_SHAFT_CULLING)
             && oldCandidateList != null ) {
            Shaft shaft = new Shaft();

            if ( galerkinState.exactVisibility != 0 ) {
                if ( receiverElement != null && sourceElement != null ) {
                    Polygon receiverPolygon = new Polygon();
                    Polygon sourcePolygon = new Polygon();
                    receiverElement.initPolygon(receiverPolygon);
                    sourceElement.initPolygon(sourcePolygon);
                    shaft.constructFromPolygonToPolygon(receiverPolygon, sourcePolygon);
                }
            }
            else {
                BoundingBox boundingBox = new BoundingBox();
                patch.computeAndGetBoundingBox(boundingBox);
                shaft.constructFromBoundingBoxes(topLevelBoundingBox, boundingBox);
            }

            shaft.setShaftOmit(topElement.patch);
            shaft.setShaftOmit(patch);
            ArrayList<Geometry> arr = new ArrayList<>();
            shaft.doCulling(oldCandidateList, arr, galerkinState.shaftCullStrategy);
            candidateList[0] = arr;

            if ( shaft.isCut() ) {
                Shaft.freeCandidateList(candidateList[0]);
                candidateList[0] = oldCandidateList;
                return;
            }
        }

        Interaction link = new Interaction();
        link.K = borrowKBuffer();
        link.ownsK = false;
        link.receiverElement = receiverElement;
        link.sourceElement = sourceElement;

        if ( receiverElement != null ) {
            link.numberOfBasisFunctionsOnReceiver = receiverElement.basisSize;
        }

        if ( sourceElement != null ) {
            link.numberOfBasisFunctionsOnSource = sourceElement.basisSize;
        }

        boolean isSceneGeometry = candidateList[0] == scene.geometryList;
        boolean isClusteredGeometry = candidateList[0] == scene.clusteredGeometryList;
        ArrayList<Geometry> geometryListReferences = candidateList[0];
        FormFactorStrategy.computeAreaToAreaFormFactorVisibility(
            scene.voxelGrid,
            geometryListReferences,
            isSceneGeometry,
            isClusteredGeometry,
            link,
            galerkinState);

        if ( galerkinState.exactVisibility != 0
             || galerkinState.shaftCullMode == GalerkinShaftCullMode.ALWAYS_DO_SHAFT_CULLING ) {
            if ( oldCandidateList != candidateList[0] ) {
                Shaft.freeCandidateList(candidateList[0]);
            }
            candidateList[0] = oldCandidateList;
        }

        if ( (link.visibility & 0xFF) > 0 ) {
            Interaction newLink = Interaction.interactionDuplicate(link);
            if ( newLink == null ) {
                return;
            }

            // Store interactions with the source patch for the progressive radiosity
            // method and with the receiving patch for gathering methods.
            if ( galerkinState.galerkinIterationMethod == GalerkinIterationMethod.SOUTH_WELL ) {
                if ( sourceElement != null ) {
                    if ( sourceElement.interactions == null ) {
                        sourceElement.interactions = new ArrayList<>();
                    }
                    sourceElement.interactions.add(newLink);
                }
            }
            else if ( receiverElement != null ) {
                if ( receiverElement.interactions == null ) {
                    receiverElement.interactions = new ArrayList<>();
                }
                receiverElement.interactions.add(newLink);
            }
        }

        float[] borrowed = link.K;
        link.K = null;
        returnKBuffer(borrowed);
    }

    private static void geometryLink(
        Scene scene,
        GalerkinState galerkinState,
        GalerkinRole role,
        ArrayList<Geometry>[] candidateList,
        GalerkinElement topElement,
        BoundingBox topLevelBoundingBox,
        Geometry geometry)
    {
        if ( scene == null || galerkinState == null || candidateList == null || candidateList.length == 0
             || topElement == null || topElement.patch == null || geometry == null ) {
            return;
        }

        // Immediately return if the geometry is bounded and behind the plane
        // of the patch for which interactions are created.
        if ( geometry.bounded
             && geometry.getBoundingBox().behindPlane(topElement.patch.normal, topElement.patch.planeConstant) ) {
            return;
        }

        // If the geometry is bounded, do shaft culling, reducing the candidate
        // list which contains possible occluders between patch pairs.
        ArrayList<Geometry> oldCandidateList = candidateList[0];
        if ( geometry.bounded && oldCandidateList != null ) {
            Shaft shaft = new Shaft();
            BoundingBox boundingBox = geometry.getBoundingBox();
            shaft.constructFromBoundingBoxes(topLevelBoundingBox, boundingBox);
            shaft.setShaftOmit(topElement.patch);
            ArrayList<Geometry> arr = new ArrayList<>();
            shaft.doCulling(oldCandidateList, arr, galerkinState.shaftCullStrategy);
            candidateList[0] = arr;
        }

        // If the geometry is an aggregate, test each child. If it is primitive,
        // create an initial link with each patch it consists of.
        if ( geometry.isCompound() ) {
            ArrayList<Geometry> geometryList = Geometry.primitiveListCopy(geometry);
            for ( int i = 0; geometryList != null && i < geometryList.size(); i++ ) {
                geometryLink(
                    scene,
                    galerkinState,
                    role,
                    candidateList,
                    topElement,
                    topLevelBoundingBox,
                    geometryList.get(i));
            }
        }
        else {
            ArrayList<Patch> patchList = Geometry.patchListReference(geometry);
            for ( int i = 0; patchList != null && i < patchList.size(); i++ ) {
                createInitialLink(
                    scene,
                    galerkinState,
                    role,
                    candidateList,
                    topElement,
                    topLevelBoundingBox,
                    patchList.get(i));
            }
        }

        if ( geometry.bounded && oldCandidateList != null ) {
            Shaft.freeCandidateList(candidateList[0]);
        }
        candidateList[0] = oldCandidateList;
    }

    public static void createInitialLinks(
        Scene scene,
        GalerkinState galerkinState,
        GalerkinRole role,
        GalerkinElement topElement)
    {
        if ( scene == null || galerkinState == null || topElement == null ) {
            return;
        }

        if ( (topElement.flags & ElementFlags.IS_CLUSTER_MASK) != 0 ) {
            Logger.fatal(-1, "createInitialLinks", "cannot use this routine for cluster elements");
            return;
        }
        if ( topElement.patch == null ) {
            return;
        }

        BoundingBox topLevelBoundingBox = new BoundingBox();
        topElement.patch.computeAndGetBoundingBox(topLevelBoundingBox);

        @SuppressWarnings("unchecked")
        ArrayList<Geometry>[] candidateList = (ArrayList<Geometry>[])new ArrayList<?>[1];
        candidateList[0] = scene.clusteredGeometryList;

        for ( int i = 0; scene.geometryList != null && i < scene.geometryList.size(); i++ ) {
            geometryLink(
                scene,
                galerkinState,
                role,
                candidateList,
                topElement,
                topLevelBoundingBox,
                scene.geometryList.get(i));
        }
    }
}
