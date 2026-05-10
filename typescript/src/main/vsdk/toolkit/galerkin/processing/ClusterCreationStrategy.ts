import { ColorRgb } from "../../common/color/ColorRgb";
import { Numeric } from "../../common/linealAlgebra/Numeric";
import { GalerkinElement } from "../GalerkinElement";
import { GalerkinIterationMethod } from "../GalerkinIterationMethod";
import { GalerkinState } from "../GalerkinState";
import { ElementFlags } from "../../skin/ElementFlags";
import { Geometry } from "../../skin/Geometry";
import { Patch } from "../../skin/Patch";

export class ClusterCreationStrategy {
  private static irregularElementsToDelete: GalerkinElement[] | null = null;
  private static hierarchyElements: GalerkinElement[] | null = null;

  private static addElementToIrregularChildrenDeletionCache(galerkinElement: GalerkinElement): void {
    if (ClusterCreationStrategy.irregularElementsToDelete === null) {
      ClusterCreationStrategy.irregularElementsToDelete = [];
    }
    ClusterCreationStrategy.irregularElementsToDelete.push(galerkinElement);
  }

  private static addElementToHierarchiesDeletionCache(galerkinElement: GalerkinElement): void {
    if (ClusterCreationStrategy.hierarchyElements === null) {
      ClusterCreationStrategy.hierarchyElements = [];
    }
    ClusterCreationStrategy.hierarchyElements.push(galerkinElement);
  }

  private static geomAddClusterChild(geometry: Geometry, galerkinElement: GalerkinElement, galerkinState: GalerkinState): void {
    const cluster = ClusterCreationStrategy.createClusterHierarchy(geometry, galerkinState);

    if (galerkinElement.irregularSubElements === null) {
      galerkinElement.irregularSubElements = [];
      ClusterCreationStrategy.addElementToIrregularChildrenDeletionCache(galerkinElement);
    }
    galerkinElement.irregularSubElements.push(cluster);
    if (cluster !== null) {
      cluster.parent = galerkinElement;
    }
  }

  private static patchAddClusterChild(patch: Patch, galerkinElement: GalerkinElement): void {
    const surfaceElement = patch === null ? null : GalerkinElement.fromPatch(patch);

    if (galerkinElement.irregularSubElements === null) {
      galerkinElement.irregularSubElements = [];
      ClusterCreationStrategy.addElementToIrregularChildrenDeletionCache(galerkinElement);
    }
    if (surfaceElement !== null) {
      galerkinElement.irregularSubElements.push(surfaceElement);
      surfaceElement.parent = galerkinElement;
    }
  }

  private static clusterInit(galerkinElement: GalerkinElement, galerkinState: GalerkinState): void {
    galerkinElement.area = 0.0;
    galerkinElement.numberOfPatches = 0;
    galerkinElement.minimumArea = Numeric.HUGE_FLOAT_VALUE;
    ColorRgb.arrayClear(galerkinElement.radiance as ColorRgb[], galerkinElement.basisSize);

    for (let i = 0;
      galerkinElement.irregularSubElements !== null && i < galerkinElement.irregularSubElements.length;
      i++) {
      const subCluster = galerkinElement.irregularSubElements[i] as GalerkinElement;
      if (subCluster === null) {
        continue;
      }
      galerkinElement.area += subCluster.area;
      galerkinElement.numberOfPatches += subCluster.numberOfPatches;
      (galerkinElement.radiance as ColorRgb[])[0].addScaled((galerkinElement.radiance as ColorRgb[])[0], subCluster.area, (subCluster.radiance as ColorRgb[])[0]);
      if (subCluster.minimumArea < galerkinElement.minimumArea) {
        galerkinElement.minimumArea = subCluster.minimumArea;
      }
      galerkinElement.flags |= (subCluster.flags & ElementFlags.IS_LIGHT_SOURCE_MASK);
      galerkinElement.Ed.addScaled(galerkinElement.Ed, subCluster.area, subCluster.Ed);
    }

    if (galerkinElement.area > Numeric.EPSILON_FLOAT) {
      (galerkinElement.radiance as ColorRgb[])[0].scale(1.0 / galerkinElement.area);
      galerkinElement.Ed.scale(1.0 / galerkinElement.area);
    }

    if (galerkinState.galerkinIterationMethod === GalerkinIterationMethod.SOUTH_WELL) {
      ColorRgb.arrayClear(galerkinElement.unShotRadiance as ColorRgb[], galerkinElement.basisSize);
      for (let i = 0;
        galerkinElement.irregularSubElements !== null && i < galerkinElement.irregularSubElements.length;
        i++) {
        const subCluster = galerkinElement.irregularSubElements[i] as GalerkinElement;
        if (subCluster === null) {
          continue;
        }
        (galerkinElement.unShotRadiance as ColorRgb[])[0].addScaled(
          (galerkinElement.unShotRadiance as ColorRgb[])[0],
          subCluster.area,
          (subCluster.unShotRadiance as ColorRgb[])[0],
        );
      }
      if (galerkinElement.area > Numeric.EPSILON_FLOAT) {
        (galerkinElement.unShotRadiance as ColorRgb[])[0].scale(1.0 / galerkinElement.area);
      }
    }

    galerkinElement.blockerSize = galerkinElement.geometry === null
      ? 0.0
      : galerkinElement.geometry.boundingBox.maxExtent();
  }

  public static createClusterHierarchy(geometry: Geometry | null, galerkinState: GalerkinState): GalerkinElement {
    if (geometry === null) {
      return null as unknown as GalerkinElement;
    }

    const newGalerkinElement = new GalerkinElement(geometry, galerkinState);
    ClusterCreationStrategy.addElementToHierarchiesDeletionCache(newGalerkinElement);

    geometry.radianceData = newGalerkinElement;

    if (geometry.isCompound()) {
      const geometryList = Geometry.primitiveListCopy(geometry);
      for (let i = 0; geometryList !== null && i < geometryList.length; i++) {
        ClusterCreationStrategy.geomAddClusterChild(geometryList[i], newGalerkinElement, galerkinState);
      }
    }
    else {
      const patchList = Geometry.patchListReference(geometry);
      for (let i = 0; patchList !== null && i < patchList.length; i++) {
        ClusterCreationStrategy.patchAddClusterChild(patchList[i], newGalerkinElement);
      }
    }

    ClusterCreationStrategy.clusterInit(newGalerkinElement, galerkinState);

    return newGalerkinElement;
  }

  public static freeClusterElements(): void {
    if (ClusterCreationStrategy.irregularElementsToDelete !== null) {
      for (let i = 0; i < ClusterCreationStrategy.irregularElementsToDelete.length; i++) {
        const element = ClusterCreationStrategy.irregularElementsToDelete[i];
        if (element !== null) {
          element.irregularSubElements = null;
        }
      }

      ClusterCreationStrategy.irregularElementsToDelete = null;
    }

    if (ClusterCreationStrategy.hierarchyElements !== null) {
      ClusterCreationStrategy.hierarchyElements = null;
    }
  }
}
