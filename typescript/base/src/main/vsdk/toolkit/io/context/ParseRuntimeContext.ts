import { LookUpTable } from "../../common/dataStructures/LookUpTable";
import { Vector3D } from "../../common/linealAlgebra/Vector3D";
import { Material } from "../../material/Material";
import { RadianceMethod } from "../../scene/RadianceMethod";
import { Geometry } from "../../skin/Geometry";
import { Patch } from "../../environment/geometry/elements/Patch";
import { Vertex } from "../../environment/geometry/elements/Vertex";
import { ColorContext } from "./ColorContext";
import { ColorRegistryContext } from "./ColorRegistryContext";
import { GeometryAssemblyContext } from "./GeometryAssemblyContext";
import { MaterialRegistryContext } from "./MaterialRegistryContext";
import { MaterialSelectionContext } from "./MaterialSelectionContext";
import { ObjectScopeContext } from "./ObjectScopeContext";
import { ParseContext } from "./ParseContext";
import { ParseOptionsContext } from "./ParseOptionsContext";
import { ParseSnapshotContext } from "./ParseSnapshotContext";
import { ReaderContext } from "./ReaderContext";
import { ReaderDispatchContext } from "./ReaderDispatchContext";
import { TransformScopeContext } from "./TransformScopeContext";
import { TransformStackContext } from "./TransformStackContext";
import { VertexContext } from "./VertexContext";
import { VertexRegistryContext } from "./VertexRegistryContext";

export class ParseRuntimeContext extends ParseContext {
  public parserConfig: ParseOptionsContext;
  public readerStackState: ReaderDispatchContext;
  public geometryBuildState: GeometryAssemblyContext;
  public materialState: MaterialSelectionContext;
  public colorRepository: ColorRegistryContext;
  public materialRepository: MaterialRegistryContext;
  public vertexRepository: VertexRegistryContext;
  public objectHierarchyState: ObjectScopeContext;
  public transformStack: TransformScopeContext;

  public model: ParseSnapshotContext | null;

  public radianceMethod: RadianceMethod | null;
  public singleSided: boolean;
  public currentVertexName: string | null;
  public numberOfQuarterCircleDivisions: number;
  public monochrome: boolean;
  public currentMaterial: Material | null;
  public entityNames: string[];
  public errorCodeMessages: string[];
  public entityLookUpTable: LookUpTable<string>;
  public nextFileContextId: number;
  public readerContext: ReaderContext | null;
  public currentMaterialName: string | null;
  public geometryStackHeadIndex: number;
  public geometryStack: Array<Geometry[] | null>;
  public currentPointList: Vector3D[] | null;
  public currentNormalList: Vector3D[] | null;
  public currentVertexList: Vertex[] | null;
  public currentFaceList: Patch[] | null;
  public currentGeometryList: Geometry[] | null;
  public currentObjectName: string | null;
  public transformContext: TransformStackContext | null;
  public unNamedColorContext: ColorContext | null;
  public currentColor: ColorContext | null;
  public inSurface: boolean;
  public inComplex: boolean;
  public warpConeEnds: boolean;
  public vertexLookUpTable: LookUpTable<VertexContext> | null;
  public allGeometries: Geometry[] | null;
  public geometries: Geometry[] | null;
  public materials: Material[] | null;

  public constructor() {
    super();
    this.parserConfig = new ParseOptionsContext();
    this.readerStackState = new ReaderDispatchContext();
    this.geometryBuildState = new GeometryAssemblyContext();
    this.materialState = new MaterialSelectionContext();
    this.colorRepository = new ColorRegistryContext();
    this.materialRepository = new MaterialRegistryContext();
    this.vertexRepository = new VertexRegistryContext();
    this.objectHierarchyState = new ObjectScopeContext();
    this.transformStack = new TransformScopeContext();
    this.model = null;

    this.radianceMethod = this.parserConfig.radianceMethod;
    this.singleSided = this.parserConfig.singleSided;
    this.currentVertexName = this.geometryBuildState.currentVertexName;
    this.numberOfQuarterCircleDivisions = this.parserConfig.numberOfQuarterCircleDivisions;
    this.monochrome = this.parserConfig.monochrome;
    this.currentMaterial = this.materialState.currentMaterial;
    this.entityNames = this.readerStackState.entityNames;
    this.errorCodeMessages = this.readerStackState.errorCodeMessages;
    this.entityLookUpTable = this.readerStackState.entityLookUpTable;
    this.nextFileContextId = this.readerStackState.nextFileContextId;
    this.readerContext = this.readerStackState.readerContext;
    this.currentMaterialName = this.materialState.currentMaterialName;
    this.geometryStackHeadIndex = this.geometryBuildState.geometryStackHeadIndex;
    this.geometryStack = this.geometryBuildState.geometryStack;
    this.currentPointList = this.geometryBuildState.currentPointList;
    this.currentNormalList = this.geometryBuildState.currentNormalList;
    this.currentVertexList = this.geometryBuildState.currentVertexList;
    this.currentFaceList = this.geometryBuildState.currentFaceList;
    this.currentGeometryList = this.geometryBuildState.currentGeometryList;
    this.currentObjectName = this.geometryBuildState.currentObjectName;
    this.transformContext = this.transformStack.transformContext;
    this.unNamedColorContext = this.colorRepository.unNamedColorContext;
    this.currentColor = this.colorRepository.currentColor;
    this.inSurface = this.geometryBuildState.inSurface;
    this.inComplex = this.geometryBuildState.inComplex;
    this.warpConeEnds = this.geometryBuildState.warpConeEnds;
    this.vertexLookUpTable = this.vertexRepository.vertexLookUpTable;
    this.allGeometries = this.geometryBuildState.allGeometries;
    this.geometries = this.geometryBuildState.geometries;
    this.materials = this.materialState.materials;
  }

  public destroy(): void {
    if (this.model !== null) {
      this.model = null;
    }
  }
}
