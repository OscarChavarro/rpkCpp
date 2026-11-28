package vsdk.toolkit.galerkin.processing;

import vsdk.toolkit.galerkin.ColorRgbMath;
import java.util.ArrayList;
import vsdk.toolkit.common.color.ColorRgb;
import vsdk.toolkit.skin.Numeric;
import vsdk.toolkit.galerkin.GalerkinElement;
import vsdk.toolkit.galerkin.GalerkinIterationMethod;
import vsdk.toolkit.galerkin.GalerkinState;
import vsdk.toolkit.environment.geometry.elements.Element;
import vsdk.toolkit.environment.geometry.elements.ElementFlags;
import vsdk.toolkit.skin.RpkGeometry;
import vsdk.toolkit.environment.geometry.elements.Patch;

public class ClusterCreationStrategy {
    private static ArrayList<GalerkinElement> irregularElementsToDelete = null;
    private static ArrayList<GalerkinElement> hierarchyElements = null;

    private static void addElementToIrregularChildrenDeletionCache(GalerkinElement galerkinElement) {
        if ( irregularElementsToDelete == null ) {
            irregularElementsToDelete = new ArrayList<>();
        }
        irregularElementsToDelete.add(galerkinElement);
    }

    private static void addElementToHierarchiesDeletionCache(GalerkinElement galerkinElement) {
        if ( hierarchyElements == null ) {
            hierarchyElements = new ArrayList<>();
        }
        hierarchyElements.add(galerkinElement);
    }

    /**
Creates a cluster hierarchy for the Geometry and adds it to the sub-cluster list of the
given parent cluster
*/
    private static void geomAddClusterChild(RpkGeometry geometry, GalerkinElement galerkinElement, GalerkinState galerkinState) {
        GalerkinElement cluster = ClusterCreationStrategy.createClusterHierarchy(geometry, galerkinState);

        if ( galerkinElement.irregularSubElements == null ) {
            galerkinElement.irregularSubElements = new ArrayList<>();
            ClusterCreationStrategy.addElementToIrregularChildrenDeletionCache(galerkinElement);
        }
        galerkinElement.irregularSubElements.add(cluster);
        if ( cluster != null ) {
            cluster.parent = galerkinElement;
        }
    }

    /**
Adds the toplevel (surface) element of the patch to the list of irregular
sub-elements of the galerkinElement
*/
    private static void patchAddClusterChild(Patch patch, GalerkinElement galerkinElement) {
        GalerkinElement surfaceElement = patch == null ? null : galerkinElement.galerkinState.topLevelElement(patch);

        if ( galerkinElement.irregularSubElements == null ) {
            galerkinElement.irregularSubElements = new ArrayList<>();
            ClusterCreationStrategy.addElementToIrregularChildrenDeletionCache(galerkinElement);
        }
        galerkinElement.irregularSubElements.add(surfaceElement);
        if ( surfaceElement != null ) {
            surfaceElement.parent = galerkinElement;
        }
    }

    /**
Initializes the galerkinElement element. Called bottom-up: first the
lowest level clusters and so up
*/
    private static void clusterInit(GalerkinElement galerkinElement, GalerkinState galerkinState) {
        // Total area of surfaces inside the galerkinElement is sum of the areas of the sub-clusters + pull radiance
        galerkinElement.area = 0.0;
        galerkinElement.numberOfPatches = 0;
        galerkinElement.minimumArea = Double.MAX_VALUE;
        ColorRgbMath.arrayClear(galerkinElement.radiance, galerkinElement.basisSize);
        for ( int i = 0;
              galerkinElement.irregularSubElements != null && i < galerkinElement.irregularSubElements.size();
              i++ ) {
            GalerkinElement subCluster = (GalerkinElement)galerkinElement.irregularSubElements.get(i);
            if ( subCluster == null ) {
                continue;
            }
            galerkinElement.area += subCluster.area;
            galerkinElement.numberOfPatches += subCluster.numberOfPatches;
            galerkinElement.radiance[0] = galerkinElement.radiance[0].addScaled(subCluster.area, subCluster.radiance[0]);
            if ( subCluster.minimumArea < galerkinElement.minimumArea ) {
                galerkinElement.minimumArea = subCluster.minimumArea;
            }
            galerkinElement.flags |= (subCluster.flags & ElementFlags.IS_LIGHT_SOURCE_MASK);
            galerkinElement.Ed = galerkinElement.Ed.addScaled(subCluster.area, subCluster.Ed);
        }

        if ( galerkinElement.area > Numeric.EPSILON ) {
            galerkinElement.radiance[0] = galerkinElement.radiance[0].multiply(1.0 / galerkinElement.area);
            galerkinElement.Ed = galerkinElement.Ed.multiply(1.0 / galerkinElement.area);
        }

        // Also pull un-shot radiance for the "shooting" methods
        if ( galerkinState.galerkinIterationMethod == GalerkinIterationMethod.SOUTH_WELL ) {
            ColorRgbMath.arrayClear(galerkinElement.unShotRadiance, galerkinElement.basisSize);
            for ( int i = 0;
                  galerkinElement.irregularSubElements != null && i < galerkinElement.irregularSubElements.size();
                  i++ ) {
                GalerkinElement subCluster = (GalerkinElement)galerkinElement.irregularSubElements.get(i);
                if ( subCluster == null ) {
                    continue;
                }
                galerkinElement.unShotRadiance[0] = galerkinElement.unShotRadiance[0].addScaled(subCluster.area, subCluster.unShotRadiance[0]);
            }
            if ( galerkinElement.area > Numeric.EPSILON ) {
                galerkinElement.unShotRadiance[0] = galerkinElement.unShotRadiance[0].multiply(1.0 / galerkinElement.area);
            }
        }

        // Compute equivalent blocker (or blocker complement) size for multi-resolution visibility
        galerkinElement.blockerSize = galerkinElement.geometry == null
            ? 0.0
            : galerkinElement.geometry.boundingBox.maxExtent();
    }

    /**
Creates a cluster for the Geometry, recurse for the children geometries, initializes and
returns the created cluster.

Note that geometry is always of types Compound or PatchSet.
*/
    public static GalerkinElement createClusterHierarchy(RpkGeometry geometry, GalerkinState galerkinState) {
        if ( geometry == null ) {
            return null;
        }

        // Parent element
        GalerkinElement newGalerkinElement = new GalerkinElement(geometry, galerkinState);
        ClusterCreationStrategy.addElementToHierarchiesDeletionCache(newGalerkinElement);

        geometry.radianceData = newGalerkinElement;

        // Recursively creates list of sub-clusters
        if ( geometry.isCompound() ) {
            ArrayList<RpkGeometry> geometryList = RpkGeometry.primitiveListCopy(geometry);
            for ( int i = 0; geometryList != null && i < geometryList.size(); i++ ) {
                geomAddClusterChild(geometryList.get(i), newGalerkinElement, galerkinState);
            }
        }
        else {
            ArrayList<Patch> patchList = RpkGeometry.patchListReference(geometry);
            for ( int i = 0; patchList != null && i < patchList.size(); i++ ) {
                patchAddClusterChild(patchList.get(i), newGalerkinElement);
            }
        }

        ClusterCreationStrategy.clusterInit(newGalerkinElement, galerkinState);

        return newGalerkinElement;
    }

    public static void freeClusterElements() {
        if ( irregularElementsToDelete != null ) {
            for ( int i = 0; i < irregularElementsToDelete.size(); i++ ) {
                GalerkinElement element = irregularElementsToDelete.get(i);
                if ( element != null ) {
                    element.irregularSubElements = null;
                }
            }

            irregularElementsToDelete.clear();
            irregularElementsToDelete = null;
        }

        if ( hierarchyElements != null ) {
            hierarchyElements.clear();
            hierarchyElements = null;
        }
    }
}
