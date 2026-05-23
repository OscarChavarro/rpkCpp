import { BufferedInputStream } from "../../../../../java/io/BufferedInputStream";
import { FileInputStream } from "../../../../../java/io/FileInputStream";
import { ColorRgb } from "../../../common/color/ColorRgb";
import { Logger as VsdkLogger } from "../../../common/logging/Logger";
import { CoordinateAxis } from "../../../common/linealAlgebra/CoordinateAxis";
import { Jacobian } from "../../../common/linealAlgebra/Jacobian";
import { Vector3D } from "../../../common/linealAlgebra/Vector3D";
import { ColorContext } from "../../context/ColorContext";
import { ParseSnapshotContext } from "../../context/ParseSnapshotContext";
import { ReaderContext } from "../../context/ReaderContext";
import { TransformSequenceContext } from "../../context/TransformSequenceContext";
import { TransformStackContext } from "../../context/TransformStackContext";
import { Material } from "../../../material/Material";
import { PhongBidirectionalReflectanceDistributionFunction } from "../../../material/PhongBidirectionalReflectanceDistributionFunction";
import { PhongBidirectionalScatteringDistributionFunction } from "../../../material/PhongBidirectionalScatteringDistributionFunction";
import { PhongBidirectionalTransmittanceDistributionFunction } from "../../../material/PhongBidirectionalTransmittanceDistributionFunction";
import { PhongEmittanceDistributionFunction } from "../../../material/PhongEmittanceDistributionFunction";
import { Texture } from "../../../material/Texture";
import { BoundingBox } from "../../../skin/AxisAlignedBoundingBox";
import { Compound } from "../../../skin/Compound";
import { Geometry } from "../../../skin/Geometry";
import { GeometryClassId } from "../../../skin/GeometryClassId";
import { MaterialColorFlags } from "../../../material/MaterialColorFlags";
import { MeshSurface } from "../../../skin/MeshSurface";
import { MinMaxBox } from "../../../skin/MinMaxBox";
import { Patch } from "../../../environment/geometry/elements/Patch";
import { PatchSet } from "../../../environment/geometry/elements/PatchSet";
import { Vertex } from "../../../environment/geometry/elements/Vertex";
import { BinaryModelGeometryRecordData } from "./BinaryModelGeometryRecordData";
import { BinaryModelPatchRecordData } from "./BinaryModelPatchRecordData";
import { BinaryModelReadCleanup } from "./BinaryModelReadCleanup";
import { BinaryModelReadPrimitives } from "./BinaryModelReadPrimitives";
import { BinaryModelSnapshotRecordData } from "./BinaryModelSnapshotRecordData";
import { BinaryModelVertexRecordData } from "./BinaryModelVertexRecordData";

const fs = require("node:fs");

class ReadFailureException {
}

export class BinaryModelDeserializer {
  private constructor() {
  }

  private static require(ok: boolean): void {
    if (!ok) {
      throw new ReadFailureException();
    }
  }

  private static decodeCString(bytes: Uint8Array | null): string {
    if (bytes === null) {
      return "";
    }

    let len = 0;
    while (len < bytes.length && bytes[len] !== 0) {
      len++;
    }

    return Buffer.from(bytes.subarray(0, len)).toString("utf8");
  }

  private static readFixedCString(
    input: BufferedInputStream,
    size: number,
    forcedNullIndex: number
  ): string {
    const bytes = new Uint8Array(size);
    BinaryModelReadPrimitives.readBytes(input, bytes, size);
    if (forcedNullIndex >= 0 && forcedNullIndex < size) {
      bytes[forcedNullIndex] = 0;
    }
    return BinaryModelDeserializer.decodeCString(bytes);
  }

  public static read(fileName: string | null): ParseSnapshotContext | null {
    if (fileName === null || fileName.length === 0) {
      return null;
    }

    try {
      if (!fs.existsSync(fileName)) {
        return null;
      }
      const stats = fs.statSync(fileName);
      if (!stats.isFile()) {
        return null;
      }
      fs.accessSync(fileName, fs.constants.R_OK);
    }
    catch (_error) {
      return null;
    }

    const vectors: Array<Vector3D | null> = [];
    const vertices: Array<Vertex | null> = [];
    const patches: Array<Patch | null> = [];
    const materials: Array<Material | null> = [];
    const geometries: Array<Geometry | null> = [];
    const colorContexts: Array<ColorContext | null> = [];
    const readerContexts: Array<ReaderContext | null> = [];
    const transformArrays: Array<TransformSequenceContext | null> = [];
    const transformContexts: Array<TransformStackContext | null> = [];
    const vertexRecords: Array<BinaryModelVertexRecordData | null> = [];
    const patchRecords: Array<BinaryModelPatchRecordData | null> = [];
    const geometryRecords: Array<BinaryModelGeometryRecordData | null> = [];

    const modelRecord = new BinaryModelSnapshotRecordData();
    let model: ParseSnapshotContext | null = null;
    let ok = false;

    let vectorCount = 0;
    let vertexCount = 0;
    let patchCount = 0;
    let materialCount = 0;
    let geometryCount = 0;
    let colorContextCount = 0;
    let readerContextCount = 0;
    let transformArrayCount = 0;
    let transformContextCount = 0;

    let input: BufferedInputStream | null = null;

    try {
      input = new BufferedInputStream(new FileInputStream(fileName));
      BinaryModelDeserializer.require(BinaryModelReadPrimitives.validateBinaryHeader(input));

      const countOut = [0];

      BinaryModelDeserializer.require(BinaryModelReadPrimitives.readNonNegativeCount(input, "vectors", countOut));
      vectorCount = countOut[0]!;
      BinaryModelDeserializer.require(BinaryModelReadPrimitives.readNonNegativeCount(input, "vertices", countOut));
      vertexCount = countOut[0]!;
      BinaryModelDeserializer.require(BinaryModelReadPrimitives.readNonNegativeCount(input, "patches", countOut));
      patchCount = countOut[0]!;
      BinaryModelDeserializer.require(BinaryModelReadPrimitives.readNonNegativeCount(input, "materials", countOut));
      materialCount = countOut[0]!;
      BinaryModelDeserializer.require(BinaryModelReadPrimitives.readNonNegativeCount(input, "geometries", countOut));
      geometryCount = countOut[0]!;
      BinaryModelDeserializer.require(BinaryModelReadPrimitives.readNonNegativeCount(input, "color contexts", countOut));
      colorContextCount = countOut[0]!;
      BinaryModelDeserializer.require(BinaryModelReadPrimitives.readNonNegativeCount(input, "reader contexts", countOut));
      readerContextCount = countOut[0]!;
      BinaryModelDeserializer.require(BinaryModelReadPrimitives.readNonNegativeCount(input, "transform arrays", countOut));
      transformArrayCount = countOut[0]!;
      BinaryModelDeserializer.require(BinaryModelReadPrimitives.readNonNegativeCount(input, "transform contexts", countOut));
      transformContextCount = countOut[0]!;

      BinaryModelDeserializer.require(BinaryModelReadPrimitives.initializeArrayList(vectors, vectorCount, null, "vectors"));
      BinaryModelDeserializer.require(BinaryModelReadPrimitives.initializeArrayList(vertices, vertexCount, null, "vertices"));
      BinaryModelDeserializer.require(BinaryModelReadPrimitives.initializeArrayList(patches, patchCount, null, "patches"));
      BinaryModelDeserializer.require(BinaryModelReadPrimitives.initializeArrayList(materials, materialCount, null, "materials"));
      BinaryModelDeserializer.require(BinaryModelReadPrimitives.initializeArrayList(geometries, geometryCount, null, "geometries"));
      BinaryModelDeserializer.require(BinaryModelReadPrimitives.initializeArrayList(colorContexts, colorContextCount, null, "color contexts"));
      BinaryModelDeserializer.require(BinaryModelReadPrimitives.initializeArrayList(readerContexts, readerContextCount, null, "reader contexts"));
      BinaryModelDeserializer.require(BinaryModelReadPrimitives.initializeArrayList(transformArrays, transformArrayCount, null, "transform arrays"));
      BinaryModelDeserializer.require(BinaryModelReadPrimitives.initializeArrayList(transformContexts, transformContextCount, null, "transform contexts"));
      BinaryModelDeserializer.require(BinaryModelReadPrimitives.initializeArrayList(vertexRecords, vertexCount, new BinaryModelVertexRecordData(), "vertex records"));
      BinaryModelDeserializer.require(BinaryModelReadPrimitives.initializeArrayList(patchRecords, patchCount, new BinaryModelPatchRecordData(), "patch records"));
      BinaryModelDeserializer.require(BinaryModelReadPrimitives.initializeArrayList(geometryRecords, geometryCount, new BinaryModelGeometryRecordData(), "geometry records"));

      BinaryModelDeserializer.require(BinaryModelReadPrimitives.expectTag(input, "VEC3"));
      for (let i = 0; i < vectorCount; i++) {
        const vector = new Vector3D();
        BinaryModelDeserializer.require(BinaryModelReadPrimitives.readVector(input, vector));
        vectors[i] = vector;
      }

      BinaryModelDeserializer.require(BinaryModelReadPrimitives.expectTag(input, "MTLS"));
      for (let i = 0; i < materialCount; i++) {
        const materialName: Array<string | null> = [null];
        const hasMaterialName = [false];
        BinaryModelDeserializer.require(BinaryModelReadPrimitives.readNullableString(input, materialName, hasMaterialName));
        const sided = BinaryModelReadPrimitives.readBool(input);

        let edf: PhongEmittanceDistributionFunction | null = null;
        const hasEdf = BinaryModelReadPrimitives.readBool(input);
        if (hasEdf) {
          const kd = new ColorRgb();
          const ks = new ColorRgb();
          BinaryModelDeserializer.require(BinaryModelReadPrimitives.readColor(input, kd));
          BinaryModelDeserializer.require(BinaryModelReadPrimitives.readColor(input, ks));
          const ns = BinaryModelReadPrimitives.readFloatLE(input);
          edf = new PhongEmittanceDistributionFunction(kd, ks, ns);
        }

        let bsdf: PhongBidirectionalScatteringDistributionFunction | null = null;
        const hasBsdf = BinaryModelReadPrimitives.readBool(input);
        if (hasBsdf) {
          let brdf: PhongBidirectionalReflectanceDistributionFunction | null = null;
          let btdf: PhongBidirectionalTransmittanceDistributionFunction | null = null;
          let texture: Texture | null = null;

          const hasBrdf = BinaryModelReadPrimitives.readBool(input);
          if (hasBrdf) {
            const kd = new ColorRgb();
            const ks = new ColorRgb();
            BinaryModelDeserializer.require(BinaryModelReadPrimitives.readColor(input, kd));
            BinaryModelDeserializer.require(BinaryModelReadPrimitives.readColor(input, ks));
            const ns = BinaryModelReadPrimitives.readFloatLE(input);
            brdf = new PhongBidirectionalReflectanceDistributionFunction(kd, ks, ns);
          }

          const hasBtdf = BinaryModelReadPrimitives.readBool(input);
          if (hasBtdf) {
            const kd = new ColorRgb();
            const ks = new ColorRgb();
            BinaryModelDeserializer.require(BinaryModelReadPrimitives.readColor(input, kd));
            BinaryModelDeserializer.require(BinaryModelReadPrimitives.readColor(input, ks));
            const ns = BinaryModelReadPrimitives.readFloatLE(input);
            const nr = BinaryModelReadPrimitives.readFloatLE(input);
            const ni = BinaryModelReadPrimitives.readFloatLE(input);
            btdf = new PhongBidirectionalTransmittanceDistributionFunction(kd, ks, ns, nr, ni);
          }

          const hasTexture = BinaryModelReadPrimitives.readBool(input);
          if (hasTexture) {
            const width = BinaryModelReadPrimitives.readInt32LE(input);
            const height = BinaryModelReadPrimitives.readInt32LE(input);
            const channels = BinaryModelReadPrimitives.readInt32LE(input);
            const dataBytes = BinaryModelReadPrimitives.readInt64LE(input);

            if (width < 0 || height < 0 || channels < 0 || dataBytes < 0) {
              VsdkLogger.error("BinaryModelDeserializer::read", "%s", "Invalid texture dimensions in binary material");
              throw new ReadFailureException();
            }

            const expectedBytes = width * height * channels;
            if (expectedBytes !== dataBytes) {
              VsdkLogger.error("BinaryModelDeserializer::read", "%s", "Texture byte count mismatch in binary material");
              throw new ReadFailureException();
            }

            let textureData: Uint8Array | null = null;
            if (dataBytes > 0) {
              if (dataBytes > 2147483647) {
                VsdkLogger.error("BinaryModelDeserializer::read", "%s", "Texture data too large for current platform");
                throw new ReadFailureException();
              }
              textureData = new Uint8Array(dataBytes);
              BinaryModelDeserializer.require(BinaryModelReadPrimitives.readBytesChunked(input, textureData, dataBytes));
            }

            texture = new Texture(width, height, channels, textureData);
          }

          bsdf = new PhongBidirectionalScatteringDistributionFunction(brdf, btdf, texture);
        }

        const materialNameText = hasMaterialName[0] ? (materialName[0] ?? "") : "";
        materials[i] = new Material(
          materialNameText,
          edf as unknown as PhongEmittanceDistributionFunction,
          bsdf as unknown as PhongBidirectionalScatteringDistributionFunction,
          sided
        );
      }

      BinaryModelDeserializer.require(BinaryModelReadPrimitives.expectTag(input, "COLR"));
      for (let i = 0; i < colorContextCount; i++) {
        const colorContext = new ColorContext();
        colorContext.clock = BinaryModelReadPrimitives.readInt32LE(input);
        colorContext.flags = BinaryModelReadPrimitives.readInt16LE(input);
        for (let j = 0; j < ColorContext.NUMBER_OF_SPECTRAL_SAMPLES; j++) {
          colorContext.straightSamples[j] = BinaryModelReadPrimitives.readInt16LE(input);
        }
        colorContext.spectralStraightSum = BinaryModelReadPrimitives.readInt64LE(input);
        colorContext.cx = BinaryModelReadPrimitives.readFloatLE(input);
        colorContext.cy = BinaryModelReadPrimitives.readFloatLE(input);
        colorContext.eff = BinaryModelReadPrimitives.readFloatLE(input);
        colorContexts[i] = colorContext;
      }

      BinaryModelDeserializer.require(BinaryModelReadPrimitives.expectTag(input, "RCTX"));
      const readerContextPrevIndex: Array<number | null> = [];
      BinaryModelDeserializer.require(BinaryModelReadPrimitives.initializeArrayList(readerContextPrevIndex, readerContextCount, -1, "reader context prev index"));
      for (let i = 0; i < readerContextCount; i++) {
        const readerContext = new ReaderContext();
        readerContext.fileName = BinaryModelDeserializer.readFixedCString(input, 96, 95);

        const hasInputStream = BinaryModelReadPrimitives.readBool(input);
        readerContext.inputStream = null;
        if (hasInputStream) {
          readerContext.inputStream = null;
        }

        readerContext.fileContextId = BinaryModelReadPrimitives.readInt32LE(input);
        readerContext.inputLine = BinaryModelDeserializer.readFixedCString(
          input,
          ReaderContext.MGF_MAXIMUM_INPUT_LINE_LENGTH,
          ReaderContext.MGF_MAXIMUM_INPUT_LINE_LENGTH - 1
        );
        readerContext.lineNumber = BinaryModelReadPrimitives.readInt32LE(input);
        readerContext.isPipe = BinaryModelReadPrimitives.readByte(input) & 0xFF;
        readerContextPrevIndex[i] = BinaryModelReadPrimitives.readInt32LE(input);
        readerContext.prev = null;
        readerContexts[i] = readerContext;
      }

      for (let i = 0; i < readerContextCount; i++) {
        const prevOut = new BinaryModelReadPrimitives.Out<ReaderContext | null>();
        BinaryModelDeserializer.require(BinaryModelReadPrimitives.pointerFromIndex(
          readerContexts,
          readerContextPrevIndex[i] ?? -1,
          "readerContext.prev",
          prevOut
        ));
        const readerContext = readerContexts[i];
        if (readerContext !== null) {
          readerContext!.prev = prevOut.value;
        }
      }

      BinaryModelDeserializer.require(BinaryModelReadPrimitives.expectTag(input, "XFAR"));
      for (let i = 0; i < transformArrayCount; i++) {
        const transformArray = new TransformSequenceContext();
        transformArray.startingPosition.fileId = BinaryModelReadPrimitives.readInt32LE(input);
        transformArray.startingPosition.lineNumber = BinaryModelReadPrimitives.readInt32LE(input);
        transformArray.startingPosition.offset = BinaryModelReadPrimitives.readInt64LE(input);
        transformArray.numberOfDimensions = BinaryModelReadPrimitives.readInt32LE(input);
        for (let j = 0; j < TransformSequenceContext.TRANSFORM_MAXIMUM_DIMENSIONS; j++) {
          transformArray.transformArguments[j]!.i = BinaryModelReadPrimitives.readInt16LE(input);
          transformArray.transformArguments[j]!.n = BinaryModelReadPrimitives.readInt16LE(input);
          transformArray.transformArguments[j]!.arg = BinaryModelDeserializer.readFixedCString(input, 8, 7);
        }
        transformArrays[i] = transformArray;
      }

      BinaryModelDeserializer.require(BinaryModelReadPrimitives.expectTag(input, "XFCT"));
      const transformContextArrayIndex: Array<number | null> = [];
      const transformContextPrevIndex: Array<number | null> = [];
      BinaryModelDeserializer.require(BinaryModelReadPrimitives.initializeArrayList(transformContextArrayIndex, transformContextCount, -1, "transform context array index"));
      BinaryModelDeserializer.require(BinaryModelReadPrimitives.initializeArrayList(transformContextPrevIndex, transformContextCount, -1, "transform context prev index"));
      for (let i = 0; i < transformContextCount; i++) {
        const transformContext = new TransformStackContext();
        transformContext.xid = BinaryModelReadPrimitives.readInt64LE(input);
        transformContext.xac = BinaryModelReadPrimitives.readInt16LE(input);
        transformContext.rev = BinaryModelReadPrimitives.readInt16LE(input);

        for (let row = 0; row < 4; row++) {
          for (let col = 0; col < 4; col++) {
            transformContext.xf.transformMatrix.m[row]![col] = BinaryModelReadPrimitives.readDoubleLE(input);
          }
        }
        transformContext.xf.scaleFactor = BinaryModelReadPrimitives.readDoubleLE(input);
        transformContextArrayIndex[i] = BinaryModelReadPrimitives.readInt32LE(input);
        transformContextPrevIndex[i] = BinaryModelReadPrimitives.readInt32LE(input);
        transformContext.transformationArray = null;
        transformContext.prev = null;
        transformContexts[i] = transformContext;
      }

      for (let i = 0; i < transformContextCount; i++) {
        const transformContext = transformContexts[i];
        if (transformContext === null) {
          continue;
        }

        const transformArrayOut = new BinaryModelReadPrimitives.Out<TransformSequenceContext | null>();
        BinaryModelDeserializer.require(BinaryModelReadPrimitives.pointerFromIndex(
          transformArrays,
          transformContextArrayIndex[i] ?? -1,
          "transformContext.transformationArray",
          transformArrayOut
        ));
        transformContext!.transformationArray = transformArrayOut.value;

        const prevOut = new BinaryModelReadPrimitives.Out<TransformStackContext | null>();
        BinaryModelDeserializer.require(BinaryModelReadPrimitives.pointerFromIndex(
          transformContexts,
          transformContextPrevIndex[i] ?? -1,
          "transformContext.prev",
          prevOut
        ));
        transformContext!.prev = prevOut.value;
      }

      BinaryModelDeserializer.require(BinaryModelReadPrimitives.expectTag(input, "VRTX"));
      for (let i = 0; i < vertexCount; i++) {
        const record = vertexRecords[i] as BinaryModelVertexRecordData;
        record.id = BinaryModelReadPrimitives.readInt32LE(input);
        record.pointIndex = BinaryModelReadPrimitives.readInt32LE(input);
        record.normalIndex = BinaryModelReadPrimitives.readInt32LE(input);
        record.textureCoordinateIndex = BinaryModelReadPrimitives.readInt32LE(input);
        BinaryModelDeserializer.require(BinaryModelReadPrimitives.readColor(input, record.color));
        record.backIndex = BinaryModelReadPrimitives.readInt32LE(input);
        record.tmp = BinaryModelReadPrimitives.readInt32LE(input);
        record.hasRadianceData = BinaryModelReadPrimitives.readBool(input);
        if (record.hasRadianceData) {
          VsdkLogger.error("BinaryModelDeserializer::read", "%s", "Vertex radianceData is not supported in binary reader");
          throw new ReadFailureException();
        }
        BinaryModelDeserializer.require(BinaryModelReadPrimitives.readIndexList(input, "vertex.patches", record.patchIndices));

        const pointOut = new BinaryModelReadPrimitives.Out<Vector3D | null>();
        const normalOut = new BinaryModelReadPrimitives.Out<Vector3D | null>();
        const texCoordsOut = new BinaryModelReadPrimitives.Out<Vector3D | null>();
        BinaryModelDeserializer.require(BinaryModelReadPrimitives.pointerFromIndex(vectors, record.pointIndex, "vertex.point", pointOut));
        BinaryModelDeserializer.require(BinaryModelReadPrimitives.pointerFromIndex(vectors, record.normalIndex, "vertex.normal", normalOut));
        BinaryModelDeserializer.require(BinaryModelReadPrimitives.pointerFromIndex(vectors, record.textureCoordinateIndex, "vertex.textureCoordinates", texCoordsOut));

        const vertex = new Vertex(
          pointOut.value as Vector3D,
          normalOut.value,
          texCoordsOut.value,
          []
        );
        vertex.id = record.id;
        vertex.color = new ColorRgb(record.color.r, record.color.g, record.color.b);
        vertex.tmp = record.tmp;
        vertex.radianceData = null;
        vertices[i] = vertex;
      }

      for (let i = 0; i < vertexCount; i++) {
        const vertex = vertices[i];
        if (vertex === null) {
          continue;
        }
        const record = vertexRecords[i] as BinaryModelVertexRecordData;
        const backOut = new BinaryModelReadPrimitives.Out<Vertex | null>();
        BinaryModelDeserializer.require(BinaryModelReadPrimitives.pointerFromIndex(vertices, record.backIndex, "vertex.back", backOut));
        vertex!.back = backOut.value;
      }

      BinaryModelDeserializer.require(BinaryModelReadPrimitives.expectTag(input, "PTCH"));
      for (let i = 0; i < patchCount; i++) {
        const record = patchRecords[i] as BinaryModelPatchRecordData;
        record.id = BinaryModelReadPrimitives.readInt32LE(input);
        record.twinIndex = BinaryModelReadPrimitives.readInt32LE(input);
        record.numberOfVertices = BinaryModelReadPrimitives.readInt32LE(input);
        if (record.numberOfVertices !== 3 && record.numberOfVertices !== 4) {
          VsdkLogger.error("BinaryModelDeserializer::read", "%s", "Invalid patch vertex count while loading binary model");
          throw new ReadFailureException();
        }

        for (let j = 0; j < Patch.MAXIMUM_VERTICES_PER_PATCH; j++) {
          record.vertexIndices[j] = BinaryModelReadPrimitives.readInt32LE(input);
        }

        record.hasBoundingBox = BinaryModelReadPrimitives.readBool(input);
        if (record.hasBoundingBox) {
          BinaryModelDeserializer.require(BinaryModelReadPrimitives.readBoundingBoxCoordinates(input, record.boundingBoxCoordinates));
        }

        BinaryModelDeserializer.require(BinaryModelReadPrimitives.readVector(input, record.normal));
        record.planeConstant = BinaryModelReadPrimitives.readFloatLE(input);
        record.tolerance = BinaryModelReadPrimitives.readFloatLE(input);
        record.area = BinaryModelReadPrimitives.readFloatLE(input);
        BinaryModelDeserializer.require(BinaryModelReadPrimitives.readVector(input, record.midPoint));

        record.hasJacobian = BinaryModelReadPrimitives.readBool(input);
        record.jacobianA = 0.0;
        record.jacobianB = 0.0;
        record.jacobianC = 0.0;
        if (record.hasJacobian) {
          record.jacobianA = BinaryModelReadPrimitives.readFloatLE(input);
          record.jacobianB = BinaryModelReadPrimitives.readFloatLE(input);
          record.jacobianC = BinaryModelReadPrimitives.readFloatLE(input);
        }

        record.directPotential = BinaryModelReadPrimitives.readFloatLE(input);
        record.dominantIndex = BinaryModelReadPrimitives.readInt32LE(input);
        record.omit = BinaryModelReadPrimitives.readBool(input);
        record.flags = BinaryModelReadPrimitives.readByte(input);
        BinaryModelDeserializer.require(BinaryModelReadPrimitives.readColor(input, record.color));
        record.materialIndex = BinaryModelReadPrimitives.readInt32LE(input);
        record.hasRadianceData = BinaryModelReadPrimitives.readBool(input);
        if (record.hasRadianceData) {
          VsdkLogger.error("BinaryModelDeserializer::read", "%s", "Patch radianceData is not supported in binary reader");
          throw new ReadFailureException();
        }

        const v1Out = new BinaryModelReadPrimitives.Out<Vertex | null>();
        const v2Out = new BinaryModelReadPrimitives.Out<Vertex | null>();
        const v3Out = new BinaryModelReadPrimitives.Out<Vertex | null>();
        const v4Out = new BinaryModelReadPrimitives.Out<Vertex | null>();
        BinaryModelDeserializer.require(BinaryModelReadPrimitives.pointerFromIndex(vertices, record.vertexIndices[0]!, "patch.vertex[0]", v1Out));
        BinaryModelDeserializer.require(BinaryModelReadPrimitives.pointerFromIndex(vertices, record.vertexIndices[1]!, "patch.vertex[1]", v2Out));
        BinaryModelDeserializer.require(BinaryModelReadPrimitives.pointerFromIndex(vertices, record.vertexIndices[2]!, "patch.vertex[2]", v3Out));
        if (record.numberOfVertices === 4) {
          BinaryModelDeserializer.require(BinaryModelReadPrimitives.pointerFromIndex(vertices, record.vertexIndices[3]!, "patch.vertex[3]", v4Out));
        }

        const patch = new Patch(record.numberOfVertices, v1Out.value, v2Out.value, v3Out.value, v4Out.value);
        patch.id = record.id;
        patch.normal.copy(record.normal);
        patch.planeConstant = record.planeConstant;
        patch.tolerance = record.tolerance;
        patch.area = record.area;
        patch.midPoint.copy(record.midPoint);
        patch.directPotential = record.directPotential;
        if (record.dominantIndex >= CoordinateAxis.X && record.dominantIndex <= CoordinateAxis.Z) {
          patch.index = record.dominantIndex as CoordinateAxis;
        }
        patch.omit = record.omit ? 1 : 0;
        patch.setFlags(record.flags & 0xFF);
        patch.color = new ColorRgb(record.color.r, record.color.g, record.color.b);

        const materialOut = new BinaryModelReadPrimitives.Out<Material | null>();
        BinaryModelDeserializer.require(BinaryModelReadPrimitives.pointerFromIndex(materials, record.materialIndex, "patch.material", materialOut));
        patch.material = materialOut.value;
        patch.radianceData = null;

        patch.jacobian = null;
        if (record.hasJacobian) {
          patch.jacobian = new Jacobian(record.jacobianA, record.jacobianB, record.jacobianC);
        }

        patch.boundingBox = null;
        if (record.hasBoundingBox) {
          patch.boundingBox = new BoundingBox();
          BinaryModelDeserializer.require(BinaryModelReadPrimitives.setBoundingBoxFromCoordinates(patch.boundingBox, record.boundingBoxCoordinates));
        }

        patches[i] = patch;
      }

      for (let i = 0; i < patchCount; i++) {
        const patch = patches[i]!;
        if (patch === null) {
          continue;
        }

        const record = patchRecords[i] as BinaryModelPatchRecordData;
        const twinOut = new BinaryModelReadPrimitives.Out<Patch | null>();
        BinaryModelDeserializer.require(BinaryModelReadPrimitives.pointerFromIndex(patches, record.twinIndex, "patch.twin", twinOut));
        patch!.twin = twinOut.value;
      }

      for (let i = 0; i < vertexCount; i++) {
        const vertex = vertices[i];
        if (vertex === null) {
          continue;
        }

        const patchListOut = new BinaryModelReadPrimitives.Out<Array<Patch | null> | null>();
        BinaryModelDeserializer.require(BinaryModelReadPrimitives.arrayListFromIndices(
          (vertexRecords[i] as BinaryModelVertexRecordData).patchIndices,
          patches,
          "vertex.patches",
          patchListOut
        ));
        vertex!.patches = patchListOut.value as Array<Patch> | null;
      }

      BinaryModelDeserializer.require(BinaryModelReadPrimitives.expectTag(input, "GEOM"));
      for (let i = 0; i < geometryCount; i++) {
        const record = geometryRecords[i] as BinaryModelGeometryRecordData;
        record.classId = BinaryModelReadPrimitives.readInt32LE(input);
        record.id = BinaryModelReadPrimitives.readInt32LE(input);
        record.itemCount = BinaryModelReadPrimitives.readInt32LE(input);
        record.bounded = BinaryModelReadPrimitives.readBool(input);
        record.shaftCullGeometry = BinaryModelReadPrimitives.readBool(input);
        record.omit = BinaryModelReadPrimitives.readBool(input);
        record.isDuplicate = BinaryModelReadPrimitives.readBool(input);
        BinaryModelDeserializer.require(BinaryModelReadPrimitives.readBoundingBoxCoordinates(input, record.boundingBoxCoordinates));
        record.hasRayIntersectionBox = BinaryModelReadPrimitives.readBool(input);
        record.hasRadianceData = BinaryModelReadPrimitives.readBool(input);
        if (record.hasRadianceData) {
          VsdkLogger.error("BinaryModelDeserializer::read", "%s", "Geometry radianceData is not supported in binary reader");
          throw new ReadFailureException();
        }

        record.hasObjectName = false;
        record.objectName = null;
        record.meshId = 0;
        record.materialIndex = -1;

        if (record.classId === GeometryClassId.SURFACE_MESH) {
          const objectName: Array<string | null> = [null];
          const hasObjectName = [false];
          BinaryModelDeserializer.require(BinaryModelReadPrimitives.readNullableString(input, objectName, hasObjectName));
          record.objectName = objectName[0]!;
          record.hasObjectName = hasObjectName[0]!;
          record.meshId = BinaryModelReadPrimitives.readInt32LE(input);
          record.materialIndex = BinaryModelReadPrimitives.readInt32LE(input);
          BinaryModelDeserializer.require(BinaryModelReadPrimitives.readIndexList(input, "surface.positions", record.positions));
          BinaryModelDeserializer.require(BinaryModelReadPrimitives.readIndexList(input, "surface.normals", record.normals));
          BinaryModelDeserializer.require(BinaryModelReadPrimitives.readIndexList(input, "surface.vertices", record.vertices));
          BinaryModelDeserializer.require(BinaryModelReadPrimitives.readIndexList(input, "surface.faces", record.faces));
        }
        else if (record.classId === GeometryClassId.COMPOUND) {
          BinaryModelDeserializer.require(BinaryModelReadPrimitives.readIndexList(input, "compound.children", record.children));
        }
        else if (record.classId === GeometryClassId.PATCH_SET) {
          BinaryModelDeserializer.require(BinaryModelReadPrimitives.readIndexList(input, "patchSet.patchList", record.patchSetPatches));
        }
        else {
          VsdkLogger.error("BinaryModelDeserializer::read", "%s", "Unsupported geometry type in binary model");
          throw new ReadFailureException();
        }
      }

      for (let i = 0; i < geometryCount; i++) {
        const record = geometryRecords[i] as BinaryModelGeometryRecordData;
        let geometry: Geometry | null = null;

        if (record.classId === GeometryClassId.SURFACE_MESH) {
          const positionsOut = new BinaryModelReadPrimitives.Out<Array<Vector3D | null> | null>();
          const normalsOut = new BinaryModelReadPrimitives.Out<Array<Vector3D | null> | null>();
          const verticesOut = new BinaryModelReadPrimitives.Out<Array<Vertex | null> | null>();
          const facesOut = new BinaryModelReadPrimitives.Out<Array<Patch | null> | null>();
          const materialOut = new BinaryModelReadPrimitives.Out<Material | null>();

          BinaryModelDeserializer.require(BinaryModelReadPrimitives.arrayListFromIndices(record.positions, vectors, "surface.positions", positionsOut));
          BinaryModelDeserializer.require(BinaryModelReadPrimitives.arrayListFromIndices(record.normals, vectors, "surface.normals", normalsOut));
          BinaryModelDeserializer.require(BinaryModelReadPrimitives.arrayListFromIndices(record.vertices, vertices, "surface.vertices", verticesOut));
          BinaryModelDeserializer.require(BinaryModelReadPrimitives.arrayListFromIndices(record.faces, patches, "surface.faces", facesOut));
          BinaryModelDeserializer.require(BinaryModelReadPrimitives.pointerFromIndex(materials, record.materialIndex, "surface.material", materialOut));

          const objectName = record.hasObjectName ? `${record.objectName}` : null;
          const surface = new MeshSurface(
            objectName as unknown as string,
            materialOut.value as unknown as Material,
            positionsOut.value as unknown as Array<Vector3D>,
            normalsOut.value as unknown as Array<Vector3D>,
            null,
            verticesOut.value as unknown as Array<Vertex>,
            facesOut.value as unknown as Array<Patch>,
            MaterialColorFlags.NO_COLORS
          );
          surface.meshId = record.meshId;
          geometry = surface;
        }
        else if (record.classId === GeometryClassId.COMPOUND) {
          geometry = new Compound([]);
        }
        else if (record.classId === GeometryClassId.PATCH_SET) {
          const patchListOut = new BinaryModelReadPrimitives.Out<Array<Patch | null> | null>();
          BinaryModelDeserializer.require(BinaryModelReadPrimitives.arrayListFromIndices(record.patchSetPatches, patches, "patchSet.patchList", patchListOut));
          geometry = new PatchSet(patchListOut.value as unknown as Array<Patch>);
        }

        if (geometry === null) {
          VsdkLogger.error("BinaryModelDeserializer::read", "%s", "Could not instantiate geometry while loading binary model");
          throw new ReadFailureException();
        }

        geometry.className = record.classId;
        geometry.id = record.id;
        geometry.itemCount = record.itemCount;
        geometry.bounded = record.bounded;
        geometry.shaftCullGeometry = record.shaftCullGeometry;
        geometry.omit = record.omit;
        geometry.isDuplicate = record.isDuplicate;
        BinaryModelDeserializer.require(BinaryModelReadPrimitives.setBoundingBoxFromCoordinates(geometry.boundingBox, record.boundingBoxCoordinates));

        if (record.hasRayIntersectionBox) {
          if (geometry.rayIntersectionBox === null) {
            geometry.rayIntersectionBox = new MinMaxBox(geometry.boundingBox);
          }
          else {
            geometry.rayIntersectionBox.updateFromBoundingBox(geometry.boundingBox);
          }
        }
        else {
          geometry.rayIntersectionBox = null;
        }

        geometry.radianceData = null;
        geometries[i] = geometry;
      }

      for (let i = 0; i < geometryCount; i++) {
        const record = geometryRecords[i] as BinaryModelGeometryRecordData;
        if (record.classId === GeometryClassId.COMPOUND) {
          const compound = geometries[i] as Compound;
          const childrenOut = new BinaryModelReadPrimitives.Out<Array<Geometry | null> | null>();
          BinaryModelDeserializer.require(BinaryModelReadPrimitives.arrayListFromIndices(record.children, geometries, "compound.children", childrenOut));
          compound.children = childrenOut.value as unknown as Array<Geometry>;
        }
      }

      BinaryModelDeserializer.require(BinaryModelReadPrimitives.expectTag(input, "MODL"));
      modelRecord.currentColorIndex = BinaryModelReadPrimitives.readInt32LE(input);
      const textOut: Array<string | null> = [null];
      const hasTextOut = [false];
      BinaryModelDeserializer.require(BinaryModelReadPrimitives.readNullableString(input, textOut, hasTextOut));
      modelRecord.currentMaterialName = textOut[0]!;
      modelRecord.hasCurrentMaterialName = hasTextOut[0]!;
      BinaryModelDeserializer.require(BinaryModelReadPrimitives.readNullableString(input, textOut, hasTextOut));
      modelRecord.currentObjectName = textOut[0]!;
      modelRecord.hasCurrentObjectName = hasTextOut[0]!;
      BinaryModelDeserializer.require(BinaryModelReadPrimitives.readNullableString(input, textOut, hasTextOut));
      modelRecord.currentVertexName = textOut[0]!;
      modelRecord.hasCurrentVertexName = hasTextOut[0]!;
      modelRecord.geometryStackHeadIndex = BinaryModelReadPrimitives.readInt32LE(input);
      modelRecord.inComplex = BinaryModelReadPrimitives.readBool(input);
      modelRecord.inSurface = BinaryModelReadPrimitives.readBool(input);
      modelRecord.monochrome = BinaryModelReadPrimitives.readBool(input);
      modelRecord.singleSided = BinaryModelReadPrimitives.readBool(input);
      modelRecord.warpConeEnds = BinaryModelReadPrimitives.readBool(input);
      modelRecord.numberOfQuarterCircleDivisions = BinaryModelReadPrimitives.readInt32LE(input);
      modelRecord.readerContextIndex = BinaryModelReadPrimitives.readInt32LE(input);
      modelRecord.transformContextIndex = BinaryModelReadPrimitives.readInt32LE(input);

      BinaryModelDeserializer.require(BinaryModelReadPrimitives.readIndexList(input, "model.currentFaceList", modelRecord.currentFaceList));
      BinaryModelDeserializer.require(BinaryModelReadPrimitives.readIndexList(input, "model.currentGeometryList", modelRecord.currentGeometryList));
      BinaryModelDeserializer.require(BinaryModelReadPrimitives.readIndexList(input, "model.currentNormalList", modelRecord.currentNormalList));
      BinaryModelDeserializer.require(BinaryModelReadPrimitives.readIndexList(input, "model.currentPointList", modelRecord.currentPointList));
      BinaryModelDeserializer.require(BinaryModelReadPrimitives.readIndexList(input, "model.currentVertexList", modelRecord.currentVertexList));
      BinaryModelDeserializer.require(BinaryModelReadPrimitives.readIndexList(input, "model.geometries", modelRecord.geometries));
      BinaryModelDeserializer.require(BinaryModelReadPrimitives.readIndexList(input, "model.materials", modelRecord.materials));

      model = new ParseSnapshotContext();
      const currentColorOut = new BinaryModelReadPrimitives.Out<ColorContext | null>();
      const readerContextOut = new BinaryModelReadPrimitives.Out<ReaderContext | null>();
      const transformContextOut = new BinaryModelReadPrimitives.Out<TransformStackContext | null>();
      BinaryModelDeserializer.require(BinaryModelReadPrimitives.pointerFromIndex(colorContexts, modelRecord.currentColorIndex, "model.currentColor", currentColorOut));
      BinaryModelDeserializer.require(BinaryModelReadPrimitives.pointerFromIndex(readerContexts, modelRecord.readerContextIndex, "model.readerContext", readerContextOut));
      BinaryModelDeserializer.require(BinaryModelReadPrimitives.pointerFromIndex(transformContexts, modelRecord.transformContextIndex, "model.transformContext", transformContextOut));

      model.currentColor = currentColorOut.value;
      model.geometryStackHeadIndex = modelRecord.geometryStackHeadIndex;
      model.inComplex = modelRecord.inComplex;
      model.inSurface = modelRecord.inSurface;
      model.monochrome = modelRecord.monochrome;
      model.singleSided = modelRecord.singleSided;
      model.warpConeEnds = modelRecord.warpConeEnds;
      model.numberOfQuarterCircleDivisions = modelRecord.numberOfQuarterCircleDivisions;
      model.readerContext = readerContextOut.value;
      model.transformContext = transformContextOut.value;

      BinaryModelDeserializer.require(BinaryModelReadPrimitives.populateModelStrings(model, modelRecord));

      const modelFaceListOut = new BinaryModelReadPrimitives.Out<Array<Patch | null> | null>();
      const modelGeometryListOut = new BinaryModelReadPrimitives.Out<Array<Geometry | null> | null>();
      const modelNormalListOut = new BinaryModelReadPrimitives.Out<Array<Vector3D | null> | null>();
      const modelPointListOut = new BinaryModelReadPrimitives.Out<Array<Vector3D | null> | null>();
      const modelVertexListOut = new BinaryModelReadPrimitives.Out<Array<Vertex | null> | null>();
      const modelGeometriesOut = new BinaryModelReadPrimitives.Out<Array<Geometry | null> | null>();
      const modelMaterialsOut = new BinaryModelReadPrimitives.Out<Array<Material | null> | null>();

      BinaryModelDeserializer.require(BinaryModelReadPrimitives.arrayListFromIndices(modelRecord.currentFaceList, patches, "model.currentFaceList", modelFaceListOut));
      BinaryModelDeserializer.require(BinaryModelReadPrimitives.arrayListFromIndices(modelRecord.currentGeometryList, geometries, "model.currentGeometryList", modelGeometryListOut));
      BinaryModelDeserializer.require(BinaryModelReadPrimitives.arrayListFromIndices(modelRecord.currentNormalList, vectors, "model.currentNormalList", modelNormalListOut));
      BinaryModelDeserializer.require(BinaryModelReadPrimitives.arrayListFromIndices(modelRecord.currentPointList, vectors, "model.currentPointList", modelPointListOut));
      BinaryModelDeserializer.require(BinaryModelReadPrimitives.arrayListFromIndices(modelRecord.currentVertexList, vertices, "model.currentVertexList", modelVertexListOut));
      BinaryModelDeserializer.require(BinaryModelReadPrimitives.arrayListFromIndices(modelRecord.geometries, geometries, "model.geometries", modelGeometriesOut));
      BinaryModelDeserializer.require(BinaryModelReadPrimitives.arrayListFromIndices(modelRecord.materials, materials, "model.materials", modelMaterialsOut));

      model.currentFaceList = modelFaceListOut.value as unknown as Array<Patch> | null;
      model.currentGeometryList = modelGeometryListOut.value as unknown as Array<Geometry> | null;
      model.currentNormalList = modelNormalListOut.value as unknown as Array<Vector3D> | null;
      model.currentPointList = modelPointListOut.value as unknown as Array<Vector3D> | null;
      model.currentVertexList = modelVertexListOut.value as unknown as Array<Vertex> | null;
      model.geometries = modelGeometriesOut.value as unknown as Array<Geometry> | null;
      model.materials = modelMaterialsOut.value as unknown as Array<Material> | null;

      let maxPatchId = 0;
      for (let i = 0; i < patches.length; i++) {
        const patch = patches[i]!;
        if (patch !== null && patch.id > maxPatchId) {
          maxPatchId = patch.id;
        }
      }
      Patch.setNextId(maxPatchId + 1);

      let maxGeometryId = -1;
      for (let i = 0; i < geometries.length; i++) {
        const geometry = geometries[i]!;
        if (geometry !== null && geometry.id > maxGeometryId) {
          maxGeometryId = geometry.id;
        }
      }
      Geometry.nextGeometryId = maxGeometryId + 1;

      ok = true;
    }
    catch (error) {
      if (error instanceof ReadFailureException) {
        ok = false;
      }
      else {
        VsdkLogger.error("BinaryModelDeserializer::read", "%s", "Unexpected failure while reading binary model");
        ok = false;
      }
    }
    finally {
      if (input !== null) {
        input.dispose();
      }
    }

    BinaryModelReadCleanup.releaseVertexRecordIndexLists(vertexRecords as unknown as Array<BinaryModelVertexRecordData>);
    BinaryModelReadCleanup.releaseGeometryRecordIndexLists(geometryRecords as unknown as Array<BinaryModelGeometryRecordData>);
    BinaryModelReadCleanup.releaseModelRecordIndexLists(modelRecord);

    if (!ok) {
      BinaryModelReadCleanup.cleanupPartialModel(
        vectors as unknown as Array<Vector3D>,
        vertices as unknown as Array<Vertex>,
        patches as unknown as Array<Patch>,
        materials as unknown as Array<Material>,
        geometries as unknown as Array<Geometry>,
        colorContexts as unknown as Array<ColorContext>,
        readerContexts as unknown as Array<ReaderContext>,
        transformArrays as unknown as Array<TransformSequenceContext>,
        transformContexts as unknown as Array<TransformStackContext>,
        model
      );
      return null;
    }

    return model;
  }
}
