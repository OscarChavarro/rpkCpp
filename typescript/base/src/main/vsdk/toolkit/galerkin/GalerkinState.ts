import { ColorRgb } from "../common/color/ColorRgb";
import { Vector3D } from "../common/linealAlgebra/Vector3D";
import { CubatureDegree } from "../numericalAnalysis/CubatureDegree";
import { CubatureRule } from "../numericalAnalysis/CubatureRule";
import { QuadCubatureRule } from "../numericalAnalysis/QuadCubatureRule";
import { TriangleCubatureRule } from "../numericalAnalysis/TriangleCubatureRule";
import { SglContext } from "../render/sgl/SglContext";
import { ToneMappingContext } from "../tonemap/ToneMappingContext";
import { GalerkinBasisType } from "./GalerkinBasisType";
import { GalerkinClusteringStrategy } from "./GalerkinClusteringStrategy";
import { GalerkinElement } from "./GalerkinElement";
import { GalerkinErrorNorm } from "./GalerkinErrorNorm";
import { GalerkinIterationMethod } from "./GalerkinIterationMethod";
import { GalerkinShaftCullMode } from "./GalerkinShaftCullMode";
import { ShaftCullStrategy } from "./ShaftCullStrategy";

export class GalerkinState {
  public iterationNumber: number;
  public hierarchical: boolean;
  public importanceDriven: number;
  public clustered: number;
  public galerkinIterationMethod: GalerkinIterationMethod;
  public lazyLinking: number;
  public exactVisibility: number;
  public multiResolutionVisibility: number;
  public useConstantRadiance: number;
  public useAmbientRadiance: number;
  public constantRadiance: ColorRgb;
  public ambientRadiance: ColorRgb;
  public shaftCullMode: GalerkinShaftCullMode;

  public receiverDegree: CubatureDegree;
  public sourceDegree: CubatureDegree;
  public receiverTriangleCubatureRule: CubatureRule | null;
  public receiverQuadCubatureRule: CubatureRule | null;
  public sourceTriangleCubatureRule: CubatureRule | null;
  public sourceQuadCubatureRule: CubatureRule | null;
  public clusterRule: CubatureRule;

  public topCluster: GalerkinElement | null;

  public errorNorm: GalerkinErrorNorm;
  public relMinElemArea: number;
  public relLinkErrorThreshold: number;

  public basisType: GalerkinBasisType;
  public clusteringStrategy: GalerkinClusteringStrategy;

  public scratch: SglContext | null;
  public scratchFrameBufferSize: number;
  public lastClusterId: number;
  public lastEye: Vector3D;

  public lastClock: bigint;
  public cpuSeconds: number;

  public shaftCullStrategy: ShaftCullStrategy;
  public toneMapOptions: ToneMappingContext | null;

  private static readonly DEFAULT_GAL_HIERARCHICAL = true;
  private static readonly DEFAULT_GAL_ITERATION_METHOD = GalerkinIterationMethod.JACOBI;
  private static readonly DEFAULT_GAL_REL_MIN_ELEM_AREA = 1e-6;
  private static readonly DEFAULT_GAL_REL_LINK_ERROR_THRESHOLD = 1e-5;
  private static readonly DEFAULT_GAL_IMPORTANCE_DRIVEN = false;
  private static readonly DEFAULT_GAL_CLUSTERED = true;
  private static readonly DEFAULT_GAL_LAZY_LINKING = true;
  private static readonly DEFAULT_GAL_AMBIENT_RADIANCE = false;
  private static readonly DEFAULT_GAL_RCV_CUBATURE_DEGREE = CubatureDegree.DEGREE_5;
  private static readonly DEFAULT_GAL_SRC_CUBATURE_DEGREE = CubatureDegree.DEGREE_4;
  private static readonly DEFAULT_GAL_CLUSTERING_STRATEGY = GalerkinClusteringStrategy.ISOTROPIC;
  private static readonly DEFAULT_GAL_SHAFT_CULL_MODE = GalerkinShaftCullMode.DO_SHAFT_CULLING_FOR_REFINEMENT;
  private static readonly DEFAULT_GAL_ERROR_NORM = GalerkinErrorNorm.POWER_ERROR;
  private static readonly DEFAULT_GAL_BASIS_TYPE = GalerkinBasisType.GALERKIN_LINEAR;
  private static readonly DEFAULT_GAL_CONSTANT_RADIANCE = false;
  private static readonly DEFAULT_GAL_EXACT_VISIBILITY = true;
  private static readonly DEFAULT_GAL_MULTI_RESOLUTION_VISIBILITY = false;
  private static readonly DEFAULT_GAL_SCRATCH_FRAME_BUFFER_SIDE_SIZE_IN_PIXELS = 200;
  private static readonly DEFAULT_GAL_SHAFT_CULL_STRATEGY = ShaftCullStrategy.OVERLAP_OPEN;
  private static readonly DEFAULT_GAL_ITERATION_NOT_INITIALIZED = -1;

  public constructor() {
    this.constantRadiance = new ColorRgb();
    this.ambientRadiance = new ColorRgb();
    this.receiverTriangleCubatureRule = null;
    this.receiverQuadCubatureRule = null;
    this.sourceTriangleCubatureRule = null;
    this.sourceQuadCubatureRule = null;
    this.topCluster = null;
    this.lastClusterId = 0;
    this.lastEye = new Vector3D();
    this.lastClock = 0n;
    this.cpuSeconds = 0.0;
    this.toneMapOptions = null;

    this.hierarchical = GalerkinState.DEFAULT_GAL_HIERARCHICAL;
    this.galerkinIterationMethod = GalerkinState.DEFAULT_GAL_ITERATION_METHOD;
    this.relMinElemArea = GalerkinState.DEFAULT_GAL_REL_MIN_ELEM_AREA;
    this.relLinkErrorThreshold = GalerkinState.DEFAULT_GAL_REL_LINK_ERROR_THRESHOLD;
    this.importanceDriven = GalerkinState.DEFAULT_GAL_IMPORTANCE_DRIVEN ? 1 : 0;
    this.clustered = GalerkinState.DEFAULT_GAL_CLUSTERED ? 1 : 0;
    this.lazyLinking = GalerkinState.DEFAULT_GAL_LAZY_LINKING ? 1 : 0;
    this.useAmbientRadiance = GalerkinState.DEFAULT_GAL_AMBIENT_RADIANCE ? 1 : 0;

    this.receiverDegree = GalerkinState.DEFAULT_GAL_RCV_CUBATURE_DEGREE;
    this.sourceDegree = GalerkinState.DEFAULT_GAL_SRC_CUBATURE_DEGREE;
    this.clusteringStrategy = GalerkinState.DEFAULT_GAL_CLUSTERING_STRATEGY;
    this.shaftCullMode = GalerkinState.DEFAULT_GAL_SHAFT_CULL_MODE;
    this.errorNorm = GalerkinState.DEFAULT_GAL_ERROR_NORM;
    this.basisType = GalerkinState.DEFAULT_GAL_BASIS_TYPE;
    this.useConstantRadiance = GalerkinState.DEFAULT_GAL_CONSTANT_RADIANCE ? 1 : 0;
    this.exactVisibility = GalerkinState.DEFAULT_GAL_EXACT_VISIBILITY ? 1 : 0;
    this.multiResolutionVisibility = GalerkinState.DEFAULT_GAL_MULTI_RESOLUTION_VISIBILITY ? 1 : 0;
    this.scratchFrameBufferSize = GalerkinState.DEFAULT_GAL_SCRATCH_FRAME_BUFFER_SIDE_SIZE_IN_PIXELS;
    this.scratch = null;
    this.iterationNumber = GalerkinState.DEFAULT_GAL_ITERATION_NOT_INITIALIZED;
    this.shaftCullStrategy = GalerkinState.DEFAULT_GAL_SHAFT_CULL_STRATEGY;
    this.toneMapOptions = null;

    const triRuleRef: Array<CubatureRule | null> = [null];
    const quadRuleRef: Array<CubatureRule | null> = [null];

    TriangleCubatureRule.setTriangleCubatureRules(triRuleRef, this.receiverDegree);
    this.receiverTriangleCubatureRule = triRuleRef[0] ?? null;
    TriangleCubatureRule.setTriangleCubatureRules(triRuleRef, this.sourceDegree);
    this.sourceTriangleCubatureRule = triRuleRef[0] ?? null;
    QuadCubatureRule.setQuadCubatureRules(quadRuleRef, this.receiverDegree);
    this.receiverQuadCubatureRule = quadRuleRef[0] ?? null;
    QuadCubatureRule.setQuadCubatureRules(quadRuleRef, this.sourceDegree);
    this.sourceQuadCubatureRule = quadRuleRef[0] ?? null;
    this.clusterRule = QuadCubatureRule.degree1BoxRule();
  }
}
