import { Random48 } from "../../common/Random48";
import { ColorRgb } from "../../common/color/ColorRgb";
import { BsdfComponent } from "../../material/BsdfComponent";
import { PhongBidirectionalScatteringDistributionFunction } from "../../material/PhongBidirectionalScatteringDistributionFunction";
import { Background } from "../../scene/Background";
import { Camera } from "../../scene/Camera";
import { VoxelGrid } from "../../scene/VoxelGrid";
import { SimpleRaytracingPathNode } from "../common/SimpleRaytracingPathNode";
import { Sampler } from "../raytracing/Sampler";
import { SamplerConfig } from "../raytracing/SamplerConfig";
import { Importon } from "./Importon";
import { ImportanceMap } from "./ImportanceMap";
import { PhotonMap } from "./PhotonMap";
import { PhotonMapConfig } from "./PhotonMapConfig";
import { PhotonMapState } from "./PhotonMapState";

/**
Importon tracing
*/
export class PhotonMapImportance {
  /**
Store a importon/poton. Some acceptance tests are performed first
**/
  private static hasDiffuseOrGlossy(node: SimpleRaytracingPathNode): boolean {
    if (node.m_hit.getPatch() !== null && node.m_hit.getPatch()!.material !== null) {
      const bsdf = node.m_hit.getPatch()!.material!.getBsdf() as PhongBidirectionalScatteringDistributionFunction | null;
      return !PhotonMap.zeroAlbedo(
        bsdf,
        node.m_hit,
        BsdfComponent.BRDF_DIFFUSE_COMPONENT
          | BsdfComponent.BTDF_DIFFUSE_COMPONENT
          | BsdfComponent.BRDF_GLOSSY_COMPONENT
          | BsdfComponent.BTDF_GLOSSY_COMPONENT
      );
    }
    return false;
  }

  private static bounceDiffuseOrGlossy(node: SimpleRaytracingPathNode): boolean {
    const mask = BsdfComponent.BRDF_DIFFUSE_COMPONENT
      | BsdfComponent.BTDF_DIFFUSE_COMPONENT
      | BsdfComponent.BRDF_GLOSSY_COMPONENT
      | BsdfComponent.BTDF_GLOSSY_COMPONENT;
    return (node.m_usedComponents & mask) !== 0;
  }

  private static doImportanceStore(map: ImportanceMap, node: SimpleRaytracingPathNode, importance: ColorRgb): boolean {
    if (PhotonMapImportance.hasDiffuseOrGlossy(node)) {
      const importanceF = importance.average();
      const potentialF = 1.0;
      const footprintF = 1.0;

      const importon = new Importon(node.m_hit.getPoint(), importanceF, potentialF, footprintF, node.m_inDirF);

      return map.addPhoton(importon, node.m_hit.getNormal(), 0);
    }
    return false;
  }

  // Returns whether a valid potential path was returned.
  private static tracePotentialPath(
    camera: Camera,
    sceneVoxelGrid: VoxelGrid,
    sceneBackground: Background | null,
    photonMapState: PhotonMapState,
    photonMapConfig: PhotonMapConfig
  ): boolean {
    let path = photonMapConfig.biPath.m_eyePath;
    const scfg: SamplerConfig = photonMapConfig.eyeConfig;

    // The C++ port passes drand48() twice as arguments and gcc evaluates
    // function arguments right to left: first draw is x2, second is x1.
    const eyeX2 = Random48.drand48();
    const eyeX1 = Random48.drand48();
    path = scfg.traceNode(camera, sceneVoxelGrid, sceneBackground, path, eyeX1, eyeX2, Sampler.BSDF_ALL_COMPONENTS);
    if (path === null) {
      return false;
    }
    photonMapConfig.biPath.m_eyePath = path;

    const accImportance = new ColorRgb();
    accImportance.setMonochrome(1.0);

    let factor = path.m_G / path.m_pdfFromPrev;
    accImportance.scale(factor);

    let indirectImportance = false;

    path.ensureNext();
    let node = path.next() as SimpleRaytracingPathNode;

    let x1 = Random48.drand48();
    let x2 = Random48.drand48();

    const specFlags = BsdfComponent.BRDF_SPECULAR_COMPONENT | BsdfComponent.BTDF_SPECULAR_COMPONENT;

    while (
      scfg.traceNode(
        camera,
        sceneVoxelGrid,
        sceneBackground,
        node,
        x1,
        x2,
        indirectImportance ? specFlags : Sampler.BSDF_ALL_COMPONENTS
      ) !== null
    ) {
      const prev = node.previous() as SimpleRaytracingPathNode;

      const didDG = PhotonMapImportance.bounceDiffuseOrGlossy(prev);
      const tooClose = node.m_G > photonMapState.gThreshold;

      if (didDG && !tooClose) {
        indirectImportance = true;
      }

      accImportance.selfScalarProduct(prev.m_bsdfEval);
      factor = node.m_G / node.m_pdfFromPrev;
      accImportance.scale(factor);

      const imap = indirectImportance ? photonMapConfig.importanceMap : photonMapConfig.importanceCMap;
      if (imap !== null) {
        PhotonMapImportance.doImportanceStore(imap, node, accImportance);
      }

      node.ensureNext();
      node = node.next() as SimpleRaytracingPathNode;
      x1 = Random48.drand48();
      x2 = Random48.drand48();
    }

    return true;
  }

  public static tracePotentialPaths(
    camera: Camera,
    sceneVoxelGrid: VoxelGrid,
    sceneBackground: Background | null,
    numberOfPaths: number,
    photonMapState: PhotonMapState,
    photonMapConfig: PhotonMapConfig
  ): void {
    photonMapConfig.eyeConfig.maxDepth = 7;
    photonMapConfig.eyeConfig.minDepth = 3;

    for (let i = 0; i < numberOfPaths; i++) {
      PhotonMapImportance.tracePotentialPath(
        camera,
        sceneVoxelGrid,
        sceneBackground,
        photonMapState,
        photonMapConfig
      );
    }

    photonMapConfig.eyeConfig.maxDepth = 1;
    photonMapConfig.eyeConfig.minDepth = 1;
  }
}

