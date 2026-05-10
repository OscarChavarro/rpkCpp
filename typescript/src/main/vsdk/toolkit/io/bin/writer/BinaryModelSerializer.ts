import { FileOutputStream } from "../../../../../java/io/FileOutputStream";
import { OutputStream } from "../../../../../java/io/OutputStream";
import { ColorRgb } from "../../../common/color/ColorRgb";
import { Error } from "../../../common/Error";
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
import { BoundingBox } from "../../../skin/BoundingBox";
import { Compound } from "../../../skin/Compound";
import { Geometry } from "../../../skin/Geometry";
import { GeometryClassId } from "../../../skin/GeometryClassId";
import { MeshSurface } from "../../../skin/MeshSurface";
import { Patch } from "../../../skin/Patch";
import { PatchSet } from "../../../skin/PatchSet";
import { Vertex } from "../../../skin/Vertex";
import { BinaryModelSerializationGraph } from "./BinaryModelSerializationGraph";

const fs = require("node:fs");

export class BinaryModelSerializer {
  private static readonly BINARY_MODEL_MAGIC = new Uint8Array([
    0x52, 0x50, 0x4B, 0x5F, 0x4D, 0x47, 0x46, 0x5F,
    0x42, 0x49, 0x4E, 0x5F, 0x31, 0x00, 0x00, 0x00
  ]);

  private static readonly BINARY_MODEL_VERSION = 2;
  private static readonly BINARY_MODEL_POINTER_SIZE = 8;
  private static readonly BINARY_MODEL_LONG_SIZE = 8;
  private static readonly BINARY_MODEL_PARSE_SNAPSHOT_CONTEXT_SIZE = 120;

  private constructor() {
  }

  private static writeBytes(output: OutputStream | null, bytes: Uint8Array, length: number): void {
    if (output === null || length <= 0) {
      return;
    }

    try {
      output.write(bytes, 0, length);
    }
    catch (_error) {
    }
  }

  private static fixedCStringBytes(text: string | null, size: number): Uint8Array {
    const output = new Uint8Array(size);
    if (size <= 0) {
      return output;
    }

    const safeText = text === null ? "" : text;
    const bytes = Buffer.from(safeText, "utf8");
    let maxCopy = size - 1;
    if (maxCopy < 0) {
      maxCopy = 0;
    }

    const copyLength = bytes.length < maxCopy ? bytes.length : maxCopy;
    output.set(bytes.subarray(0, copyLength), 0);
    return output;
  }

  private static safeLabel(text: string | null): string {
    if (text === null) {
      return "(null)";
    }
    return text;
  }

  private static writeByte(output: OutputStream | null, value: number): void {
    const data = new Uint8Array(1);
    data[0] = value & 0xFF;
    BinaryModelSerializer.writeBytes(output, data, 1);
  }

  private static writeBool(output: OutputStream | null, value: boolean): void {
    BinaryModelSerializer.writeByte(output, value ? 1 : 0);
  }

  private static writeInt16LE(output: OutputStream | null, value: number): void {
    const data = new Uint8Array(2);
    const view = new DataView(data.buffer);
    view.setInt16(0, value, true);
    BinaryModelSerializer.writeBytes(output, data, 2);
  }

  private static writeInt32LE(output: OutputStream | null, value: number): void {
    const data = new Uint8Array(4);
    const view = new DataView(data.buffer);
    view.setInt32(0, value, true);
    BinaryModelSerializer.writeBytes(output, data, 4);
  }

  private static writeInt64LE(output: OutputStream | null, value: number): void {
    const data = new Uint8Array(8);
    const view = new DataView(data.buffer);
    try {
      view.setBigInt64(0, BigInt(Math.trunc(value)), true);
    }
    catch (_error) {
      const low = value >>> 0;
      const high = (value / 4294967296.0) | 0;
      view.setUint32(0, low, true);
      view.setInt32(4, high, true);
    }
    BinaryModelSerializer.writeBytes(output, data, 8);
  }

  private static writeFloatLE(output: OutputStream | null, value: number): void {
    const data = new Uint8Array(4);
    const view = new DataView(data.buffer);
    view.setFloat32(0, value, true);
    BinaryModelSerializer.writeBytes(output, data, 4);
  }

  private static writeDoubleLE(output: OutputStream | null, value: number): void {
    const data = new Uint8Array(8);
    const view = new DataView(data.buffer);
    view.setFloat64(0, value, true);
    BinaryModelSerializer.writeBytes(output, data, 8);
  }

  private static writeBytesChunked(output: OutputStream | null, data: Uint8Array | null, length: number): boolean {
    if (length < 0) {
      Error.error("BinaryModelSerializer::writeBytesChunked", "Negative block length");
      return false;
    }
    if (length === 0) {
      return true;
    }
    if (data === null) {
      Error.error("BinaryModelSerializer::writeBytesChunked", "Null block data");
      return false;
    }
    if (length > data.length) {
      Error.error("BinaryModelSerializer::writeBytesChunked", "Requested length exceeds source buffer");
      return false;
    }

    let offset = 0;
    const maxChunk = 2147483647;
    while (offset < length) {
      const remaining = length - offset;
      const chunk = remaining < maxChunk ? remaining : maxChunk;
      const chunkBuffer = new Uint8Array(chunk);
      chunkBuffer.set(data.subarray(offset, offset + chunk), 0);
      BinaryModelSerializer.writeBytes(output, chunkBuffer, chunk);
      offset += chunk;
    }

    return true;
  }

  private static writeTag(output: OutputStream | null, tag: string): void {
    const tagBytes = Buffer.from(tag, "ascii");
    BinaryModelSerializer.writeBytes(output, tagBytes, 4);
  }

  private static checkedLongToInt32(value: number, what: string, result: number[] | null): boolean {
    if (result === null || result.length === 0) {
      Error.error("BinaryModelSerializer::checkedLongToInt32", "Null output pointer for %s", BinaryModelSerializer.safeLabel(what));
      return false;
    }

    if (!Number.isFinite(value) || value > 2147483647 || value < -2147483648) {
      Error.error("BinaryModelSerializer::checkedLongToInt32", "Overflow converting to int32 for %s", BinaryModelSerializer.safeLabel(what));
      return false;
    }

    result[0] = Math.trunc(value);
    return true;
  }

  private static writeString(output: OutputStream | null, text: string | null): boolean {
    if (text === null) {
      BinaryModelSerializer.writeInt32LE(output, -1);
      return true;
    }

    const bytes = Buffer.from(text, "utf8");
    const size = [0];
    if (!BinaryModelSerializer.checkedLongToInt32(bytes.length, "string length", size)) {
      return false;
    }

    BinaryModelSerializer.writeInt32LE(output, size[0]);
    if (size[0] > 0) {
      BinaryModelSerializer.writeBytes(output, bytes, size[0]);
    }

    return true;
  }

  private static writeColor(output: OutputStream | null, color: ColorRgb): void {
    BinaryModelSerializer.writeFloatLE(output, color.r);
    BinaryModelSerializer.writeFloatLE(output, color.g);
    BinaryModelSerializer.writeFloatLE(output, color.b);
  }

  private static writeVector(output: OutputStream | null, vector: Vector3D): void {
    BinaryModelSerializer.writeFloatLE(output, vector.x);
    BinaryModelSerializer.writeFloatLE(output, vector.y);
    BinaryModelSerializer.writeFloatLE(output, vector.z);
  }

  private static writeBoundingBox(output: OutputStream | null, boundingBox: BoundingBox): void {
    for (let i = 0; i < 6; i++) {
      BinaryModelSerializer.writeFloatLE(output, boundingBox.valueAt(i));
    }
  }

  private static indexOfPointer<T>(
    ptr: T | null,
    indices: Map<T, number>,
    what: string,
    result: number[] | null
  ): boolean {
    if (result === null || result.length === 0) {
      Error.error("BinaryModelSerializer::indexOfPointer", "Missing pointer index for %s", BinaryModelSerializer.safeLabel(what));
      return false;
    }

    if (ptr === null) {
      result[0] = -1;
      return true;
    }

    const index = indices.get(ptr);
    if (index === undefined) {
      Error.error("BinaryModelSerializer::indexOfPointer", "Missing pointer index for %s", BinaryModelSerializer.safeLabel(what));
      return false;
    }

    result[0] = index;
    return true;
  }

  private static writeIndexList<T>(
    output: OutputStream | null,
    list: Array<T | null> | null,
    indices: Map<T, number>,
    what: string
  ): boolean {
    if (list === null) {
      BinaryModelSerializer.writeInt32LE(output, -1);
      return true;
    }

    const size = [0];
    if (!BinaryModelSerializer.checkedLongToInt32(list.length, what, size)) {
      return false;
    }

    BinaryModelSerializer.writeInt32LE(output, size[0]);
    for (let i = 0; i < size[0]; i++) {
      const elementIndex = [0];
      if (!BinaryModelSerializer.indexOfPointer(list[i], indices, what, elementIndex)) {
        return false;
      }
      BinaryModelSerializer.writeInt32LE(output, elementIndex[0]);
    }

    return true;
  }

  private static writeMaterialRecord(output: OutputStream | null, material: Material): boolean {
    if (!BinaryModelSerializer.writeString(output, material.getName())) {
      return false;
    }
    BinaryModelSerializer.writeBool(output, material.isSided());

    const edf = material.getEdf() as PhongEmittanceDistributionFunction | null;
    BinaryModelSerializer.writeBool(output, edf !== null);
    if (edf !== null) {
      BinaryModelSerializer.writeColor(output, edf.getKd());
      BinaryModelSerializer.writeColor(output, edf.getKs());
      BinaryModelSerializer.writeFloatLE(output, edf.getNs());
    }

    const bsdf = material.getBsdf() as PhongBidirectionalScatteringDistributionFunction | null;
    BinaryModelSerializer.writeBool(output, bsdf !== null);
    if (bsdf === null) {
      return true;
    }

    const brdf = bsdf.getBrdf() as PhongBidirectionalReflectanceDistributionFunction | null;
    BinaryModelSerializer.writeBool(output, brdf !== null);
    if (brdf !== null) {
      BinaryModelSerializer.writeColor(output, brdf.getKd());
      BinaryModelSerializer.writeColor(output, brdf.getKs());
      BinaryModelSerializer.writeFloatLE(output, brdf.getNs());
    }

    const btdf = bsdf.getBtdf() as PhongBidirectionalTransmittanceDistributionFunction | null;
    BinaryModelSerializer.writeBool(output, btdf !== null);
    if (btdf !== null) {
      BinaryModelSerializer.writeColor(output, btdf.getKd());
      BinaryModelSerializer.writeColor(output, btdf.getKs());
      BinaryModelSerializer.writeFloatLE(output, btdf.getNs());
      BinaryModelSerializer.writeFloatLE(output, btdf.getRefractionIndex().getNr());
      BinaryModelSerializer.writeFloatLE(output, btdf.getRefractionIndex().getNi());
    }

    const texture = bsdf.getTexture() as Texture | null;
    BinaryModelSerializer.writeBool(output, texture !== null);
    if (texture !== null) {
      const width = texture.getWidth();
      const height = texture.getHeight();
      const channels = texture.getChannels();
      if (width < 0 || height < 0 || channels < 0) {
        Error.error("BinaryModelSerializer::writeMaterialRecord", "Invalid texture dimensions");
        return false;
      }

      BinaryModelSerializer.writeInt32LE(output, width);
      BinaryModelSerializer.writeInt32LE(output, height);
      BinaryModelSerializer.writeInt32LE(output, channels);

      const dataBytes = width * height * channels;
      BinaryModelSerializer.writeInt64LE(output, dataBytes);

      if (dataBytes > 0) {
        const data = texture.getData();
        if (data === null) {
          Error.error("BinaryModelSerializer::writeMaterialRecord", "Texture data is null with non-zero size");
          return false;
        }
        if (!BinaryModelSerializer.writeBytesChunked(output, data, dataBytes)) {
          return false;
        }
      }
    }

    return true;
  }

  private static writeColorContextRecord(output: OutputStream | null, colorContext: ColorContext): void {
    BinaryModelSerializer.writeInt32LE(output, colorContext.clock);
    BinaryModelSerializer.writeInt16LE(output, colorContext.flags);
    for (let i = 0; i < ColorContext.NUMBER_OF_SPECTRAL_SAMPLES; i++) {
      BinaryModelSerializer.writeInt16LE(output, colorContext.straightSamples[i]);
    }
    BinaryModelSerializer.writeInt64LE(output, colorContext.spectralStraightSum);
    BinaryModelSerializer.writeFloatLE(output, colorContext.cx);
    BinaryModelSerializer.writeFloatLE(output, colorContext.cy);
    BinaryModelSerializer.writeFloatLE(output, colorContext.eff);
  }

  private static writeReaderContextRecord(
    output: OutputStream | null,
    readerContext: ReaderContext,
    context: BinaryModelSerializationGraph
  ): boolean {
    const fileName = BinaryModelSerializer.fixedCStringBytes(readerContext.fileName, 96);
    BinaryModelSerializer.writeBytes(output, fileName, 96);
    BinaryModelSerializer.writeBool(output, readerContext.inputStream !== null);
    BinaryModelSerializer.writeInt32LE(output, readerContext.fileContextId);

    const inputLine = BinaryModelSerializer.fixedCStringBytes(readerContext.inputLine, ReaderContext.MGF_MAXIMUM_INPUT_LINE_LENGTH);
    BinaryModelSerializer.writeBytes(output, inputLine, ReaderContext.MGF_MAXIMUM_INPUT_LINE_LENGTH);

    BinaryModelSerializer.writeInt32LE(output, readerContext.lineNumber);
    BinaryModelSerializer.writeByte(output, readerContext.isPipe);

    const previousIndex = [0];
    if (!BinaryModelSerializer.indexOfPointer(readerContext.prev, context.readerContextIndices, "readerContext.prev", previousIndex)) {
      return false;
    }
    BinaryModelSerializer.writeInt32LE(output, previousIndex[0]);

    return true;
  }

  private static writeTransformArrayRecord(output: OutputStream | null, transformArray: TransformSequenceContext): void {
    BinaryModelSerializer.writeInt32LE(output, transformArray.startingPosition.fileId);
    BinaryModelSerializer.writeInt32LE(output, transformArray.startingPosition.lineNumber);
    BinaryModelSerializer.writeInt64LE(output, transformArray.startingPosition.offset);
    BinaryModelSerializer.writeInt32LE(output, transformArray.numberOfDimensions);

    for (let i = 0; i < TransformSequenceContext.TRANSFORM_MAXIMUM_DIMENSIONS; i++) {
      BinaryModelSerializer.writeInt16LE(output, transformArray.transformArguments[i].i);
      BinaryModelSerializer.writeInt16LE(output, transformArray.transformArguments[i].n);
      const argument = BinaryModelSerializer.fixedCStringBytes(transformArray.transformArguments[i].arg, 8);
      BinaryModelSerializer.writeBytes(output, argument, 8);
    }
  }

  private static writeTransformContextRecord(
    output: OutputStream | null,
    transformContext: TransformStackContext,
    context: BinaryModelSerializationGraph
  ): boolean {
    BinaryModelSerializer.writeInt64LE(output, transformContext.xid);
    BinaryModelSerializer.writeInt16LE(output, transformContext.xac);
    BinaryModelSerializer.writeInt16LE(output, transformContext.rev);

    for (let i = 0; i < 4; i++) {
      for (let j = 0; j < 4; j++) {
        BinaryModelSerializer.writeDoubleLE(output, transformContext.xf.transformMatrix.m[i][j]);
      }
    }
    BinaryModelSerializer.writeDoubleLE(output, transformContext.xf.scaleFactor);

    const transformArrayIndex = [0];
    if (!BinaryModelSerializer.indexOfPointer(
      transformContext.transformationArray,
      context.transformArrayIndices,
      "transformContext.transformationArray",
      transformArrayIndex
    )) {
      return false;
    }
    BinaryModelSerializer.writeInt32LE(output, transformArrayIndex[0]);

    const previousIndex = [0];
    if (!BinaryModelSerializer.indexOfPointer(transformContext.prev, context.transformContextIndices, "transformContext.prev", previousIndex)) {
      return false;
    }
    BinaryModelSerializer.writeInt32LE(output, previousIndex[0]);

    return true;
  }

  private static writeVertexRecord(
    output: OutputStream | null,
    vertex: Vertex,
    context: BinaryModelSerializationGraph
  ): boolean {
    BinaryModelSerializer.writeInt32LE(output, vertex.id);

    const pointIndex = [0];
    if (!BinaryModelSerializer.indexOfPointer(vertex.point, context.vectorIndices, "vertex.point", pointIndex)) {
      return false;
    }
    BinaryModelSerializer.writeInt32LE(output, pointIndex[0]);

    const normalIndex = [0];
    if (!BinaryModelSerializer.indexOfPointer(vertex.normal, context.vectorIndices, "vertex.normal", normalIndex)) {
      return false;
    }
    BinaryModelSerializer.writeInt32LE(output, normalIndex[0]);

    const textureIndex = [0];
    if (!BinaryModelSerializer.indexOfPointer(vertex.textureCoordinates, context.vectorIndices, "vertex.textureCoordinates", textureIndex)) {
      return false;
    }
    BinaryModelSerializer.writeInt32LE(output, textureIndex[0]);

    BinaryModelSerializer.writeColor(output, vertex.color);

    const backIndex = [0];
    if (!BinaryModelSerializer.indexOfPointer(vertex.back, context.vertexIndices, "vertex.back", backIndex)) {
      return false;
    }
    BinaryModelSerializer.writeInt32LE(output, backIndex[0]);

    BinaryModelSerializer.writeInt32LE(output, vertex.tmp);
    BinaryModelSerializer.writeBool(output, vertex.radianceData !== null);
    return BinaryModelSerializer.writeIndexList(output, vertex.patches, context.patchIndices, "vertex.patches");
  }

  private static writePatchRecord(
    output: OutputStream | null,
    patch: Patch,
    context: BinaryModelSerializationGraph
  ): boolean {
    BinaryModelSerializer.writeInt32LE(output, patch.id);

    const twinIndex = [0];
    if (!BinaryModelSerializer.indexOfPointer(patch.twin, context.patchIndices, "patch.twin", twinIndex)) {
      return false;
    }
    BinaryModelSerializer.writeInt32LE(output, twinIndex[0]);

    BinaryModelSerializer.writeInt32LE(output, patch.numberOfVertices);
    for (let i = 0; i < Patch.MAXIMUM_VERTICES_PER_PATCH; i++) {
      const vertexIndex = [0];
      if (!BinaryModelSerializer.indexOfPointer(patch.vertex[i], context.vertexIndices, "patch.vertex", vertexIndex)) {
        return false;
      }
      BinaryModelSerializer.writeInt32LE(output, vertexIndex[0]);
    }

    BinaryModelSerializer.writeBool(output, patch.boundingBox !== null);
    if (patch.boundingBox !== null) {
      BinaryModelSerializer.writeBoundingBox(output, patch.boundingBox);
    }

    BinaryModelSerializer.writeVector(output, patch.normal);
    BinaryModelSerializer.writeFloatLE(output, patch.planeConstant);
    BinaryModelSerializer.writeFloatLE(output, patch.tolerance);
    BinaryModelSerializer.writeFloatLE(output, patch.area);
    BinaryModelSerializer.writeVector(output, patch.midPoint);

    BinaryModelSerializer.writeBool(output, patch.jacobian !== null);
    if (patch.jacobian !== null) {
      BinaryModelSerializer.writeFloatLE(output, patch.jacobian.A);
      BinaryModelSerializer.writeFloatLE(output, patch.jacobian.B);
      BinaryModelSerializer.writeFloatLE(output, patch.jacobian.C);
    }

    BinaryModelSerializer.writeFloatLE(output, patch.directPotential);
    const dominantIndex = patch.index === null ? 0 : (patch.index as number);
    BinaryModelSerializer.writeInt32LE(output, dominantIndex);
    BinaryModelSerializer.writeBool(output, patch.omit !== 0);
    BinaryModelSerializer.writeByte(output, patch.getFlags());
    BinaryModelSerializer.writeColor(output, patch.color);

    const materialIndex = [0];
    if (!BinaryModelSerializer.indexOfPointer(patch.material, context.materialIndices, "patch.material", materialIndex)) {
      return false;
    }
    BinaryModelSerializer.writeInt32LE(output, materialIndex[0]);

    BinaryModelSerializer.writeBool(output, patch.radianceData !== null);
    return true;
  }

  private static writeGeometryRecord(
    output: OutputStream | null,
    geometry: Geometry,
    context: BinaryModelSerializationGraph
  ): boolean {
    BinaryModelSerializer.writeInt32LE(output, geometry.className);
    BinaryModelSerializer.writeInt32LE(output, geometry.id);
    BinaryModelSerializer.writeInt32LE(output, geometry.itemCount);
    BinaryModelSerializer.writeBool(output, geometry.bounded);
    BinaryModelSerializer.writeBool(output, geometry.shaftCullGeometry);
    BinaryModelSerializer.writeBool(output, geometry.omit);
    BinaryModelSerializer.writeBool(output, geometry.isDuplicate);
    BinaryModelSerializer.writeBoundingBox(output, geometry.boundingBox);
    BinaryModelSerializer.writeBool(output, geometry.rayIntersectionBox !== null);
    BinaryModelSerializer.writeBool(output, geometry.radianceData !== null);

    if (geometry.className === GeometryClassId.SURFACE_MESH) {
      const surface = geometry as MeshSurface;
      if (!BinaryModelSerializer.writeString(output, surface.objectName)) {
        return false;
      }
      BinaryModelSerializer.writeInt32LE(output, surface.meshId);

      const materialIndex = [0];
      if (!BinaryModelSerializer.indexOfPointer(surface.material, context.materialIndices, "surface.material", materialIndex)) {
        return false;
      }
      BinaryModelSerializer.writeInt32LE(output, materialIndex[0]);

      if (!BinaryModelSerializer.writeIndexList(output, surface.positions, context.vectorIndices, "surface.positions")) {
        return false;
      }
      if (!BinaryModelSerializer.writeIndexList(output, surface.normals, context.vectorIndices, "surface.normals")) {
        return false;
      }
      if (!BinaryModelSerializer.writeIndexList(output, surface.vertices, context.vertexIndices, "surface.vertices")) {
        return false;
      }
      if (!BinaryModelSerializer.writeIndexList(output, surface.faces, context.patchIndices, "surface.faces")) {
        return false;
      }
    }
    else if (geometry.className === GeometryClassId.COMPOUND) {
      const compound = geometry as Compound;
      if (!BinaryModelSerializer.writeIndexList(output, compound.children, context.geometryIndices, "compound.children")) {
        return false;
      }
    }
    else if (geometry.className === GeometryClassId.PATCH_SET) {
      const patchSet = geometry as PatchSet;
      if (!BinaryModelSerializer.writeIndexList(output, patchSet.getPatchList(), context.patchIndices, "patchSet.patchList")) {
        return false;
      }
    }
    else {
      Error.error("BinaryModelSerializer::writeGeometryRecord", "Unsupported geometry class while writing");
      return false;
    }

    return true;
  }

  private static writeModelRecord(
    output: OutputStream | null,
    model: ParseSnapshotContext,
    context: BinaryModelSerializationGraph
  ): boolean {
    const currentColorIndex = [0];
    if (!BinaryModelSerializer.indexOfPointer(model.currentColor, context.colorContextIndices, "model.currentColor", currentColorIndex)) {
      return false;
    }
    BinaryModelSerializer.writeInt32LE(output, currentColorIndex[0]);

    if (!BinaryModelSerializer.writeString(output, model.currentMaterialName)) {
      return false;
    }
    if (!BinaryModelSerializer.writeString(output, model.currentObjectName)) {
      return false;
    }
    if (!BinaryModelSerializer.writeString(output, model.currentVertexName)) {
      return false;
    }

    BinaryModelSerializer.writeInt32LE(output, model.geometryStackHeadIndex);
    BinaryModelSerializer.writeBool(output, model.inComplex);
    BinaryModelSerializer.writeBool(output, model.inSurface);
    BinaryModelSerializer.writeBool(output, model.monochrome);
    BinaryModelSerializer.writeBool(output, model.singleSided);
    BinaryModelSerializer.writeBool(output, model.warpConeEnds);
    BinaryModelSerializer.writeInt32LE(output, model.numberOfQuarterCircleDivisions);

    const readerContextIndex = [0];
    if (!BinaryModelSerializer.indexOfPointer(model.readerContext, context.readerContextIndices, "model.readerContext", readerContextIndex)) {
      return false;
    }
    BinaryModelSerializer.writeInt32LE(output, readerContextIndex[0]);

    const transformContextIndex = [0];
    if (!BinaryModelSerializer.indexOfPointer(model.transformContext, context.transformContextIndices, "model.transformContext", transformContextIndex)) {
      return false;
    }
    BinaryModelSerializer.writeInt32LE(output, transformContextIndex[0]);

    if (!BinaryModelSerializer.writeIndexList(output, model.currentFaceList, context.patchIndices, "model.currentFaceList")) {
      return false;
    }
    if (!BinaryModelSerializer.writeIndexList(output, model.currentGeometryList, context.geometryIndices, "model.currentGeometryList")) {
      return false;
    }
    if (!BinaryModelSerializer.writeIndexList(output, model.currentNormalList, context.vectorIndices, "model.currentNormalList")) {
      return false;
    }
    if (!BinaryModelSerializer.writeIndexList(output, model.currentPointList, context.vectorIndices, "model.currentPointList")) {
      return false;
    }
    if (!BinaryModelSerializer.writeIndexList(output, model.currentVertexList, context.vertexIndices, "model.currentVertexList")) {
      return false;
    }
    if (!BinaryModelSerializer.writeIndexList(output, model.geometries, context.geometryIndices, "model.geometries")) {
      return false;
    }
    if (!BinaryModelSerializer.writeIndexList(output, model.materials, context.materialIndices, "model.materials")) {
      return false;
    }

    return true;
  }

  public static write(model: ParseSnapshotContext | null, fileName: string | null): boolean {
    if (model === null || fileName === null || fileName.length === 0) {
      Error.error("BinaryModelSerializer::write", "Invalid model or fileName");
      return false;
    }

    try {
      if (fs.existsSync(fileName) && fs.statSync(fileName).isDirectory()) {
        Error.error("BinaryModelSerializer::write", "Could not open output file '%s'", fileName);
        return false;
      }
    }
    catch (_error) {
      Error.error("BinaryModelSerializer::write", "Could not open output file '%s'", fileName);
      return false;
    }

    let output: FileOutputStream | null = null;
    try {
      output = new FileOutputStream(fileName);

      const context = new BinaryModelSerializationGraph();
      if (!context.collectModel(model)) {
        return false;
      }

      BinaryModelSerializer.writeBytes(output, BinaryModelSerializer.BINARY_MODEL_MAGIC, 16);
      BinaryModelSerializer.writeInt32LE(output, BinaryModelSerializer.BINARY_MODEL_VERSION);
      BinaryModelSerializer.writeInt32LE(output, BinaryModelSerializer.BINARY_MODEL_POINTER_SIZE);
      BinaryModelSerializer.writeInt32LE(output, BinaryModelSerializer.BINARY_MODEL_LONG_SIZE);
      BinaryModelSerializer.writeInt32LE(output, BinaryModelSerializer.BINARY_MODEL_PARSE_SNAPSHOT_CONTEXT_SIZE);

      const count = [0];

      if (!BinaryModelSerializer.checkedLongToInt32(context.vectors.length, "vectors count", count)) {
        return false;
      }
      BinaryModelSerializer.writeInt32LE(output, count[0]);

      if (!BinaryModelSerializer.checkedLongToInt32(context.vertices.length, "vertices count", count)) {
        return false;
      }
      BinaryModelSerializer.writeInt32LE(output, count[0]);

      if (!BinaryModelSerializer.checkedLongToInt32(context.patches.length, "patches count", count)) {
        return false;
      }
      BinaryModelSerializer.writeInt32LE(output, count[0]);

      if (!BinaryModelSerializer.checkedLongToInt32(context.materials.length, "materials count", count)) {
        return false;
      }
      BinaryModelSerializer.writeInt32LE(output, count[0]);

      if (!BinaryModelSerializer.checkedLongToInt32(context.geometries.length, "geometries count", count)) {
        return false;
      }
      BinaryModelSerializer.writeInt32LE(output, count[0]);

      if (!BinaryModelSerializer.checkedLongToInt32(context.colorContexts.length, "color contexts count", count)) {
        return false;
      }
      BinaryModelSerializer.writeInt32LE(output, count[0]);

      if (!BinaryModelSerializer.checkedLongToInt32(context.readerContexts.length, "reader contexts count", count)) {
        return false;
      }
      BinaryModelSerializer.writeInt32LE(output, count[0]);

      if (!BinaryModelSerializer.checkedLongToInt32(context.transformArrays.length, "transform arrays count", count)) {
        return false;
      }
      BinaryModelSerializer.writeInt32LE(output, count[0]);

      if (!BinaryModelSerializer.checkedLongToInt32(context.transformContexts.length, "transform contexts count", count)) {
        return false;
      }
      BinaryModelSerializer.writeInt32LE(output, count[0]);

      BinaryModelSerializer.writeTag(output, "VEC3");
      for (let i = 0; i < context.vectors.length; i++) {
        BinaryModelSerializer.writeVector(output, context.vectors[i]);
      }

      BinaryModelSerializer.writeTag(output, "MTLS");
      for (let i = 0; i < context.materials.length; i++) {
        if (!BinaryModelSerializer.writeMaterialRecord(output, context.materials[i])) {
          return false;
        }
      }

      BinaryModelSerializer.writeTag(output, "COLR");
      for (let i = 0; i < context.colorContexts.length; i++) {
        BinaryModelSerializer.writeColorContextRecord(output, context.colorContexts[i]);
      }

      BinaryModelSerializer.writeTag(output, "RCTX");
      for (let i = 0; i < context.readerContexts.length; i++) {
        if (!BinaryModelSerializer.writeReaderContextRecord(output, context.readerContexts[i], context)) {
          return false;
        }
      }

      BinaryModelSerializer.writeTag(output, "XFAR");
      for (let i = 0; i < context.transformArrays.length; i++) {
        BinaryModelSerializer.writeTransformArrayRecord(output, context.transformArrays[i]);
      }

      BinaryModelSerializer.writeTag(output, "XFCT");
      for (let i = 0; i < context.transformContexts.length; i++) {
        if (!BinaryModelSerializer.writeTransformContextRecord(output, context.transformContexts[i], context)) {
          return false;
        }
      }

      BinaryModelSerializer.writeTag(output, "VRTX");
      for (let i = 0; i < context.vertices.length; i++) {
        if (!BinaryModelSerializer.writeVertexRecord(output, context.vertices[i], context)) {
          return false;
        }
      }

      BinaryModelSerializer.writeTag(output, "PTCH");
      for (let i = 0; i < context.patches.length; i++) {
        if (!BinaryModelSerializer.writePatchRecord(output, context.patches[i], context)) {
          return false;
        }
      }

      BinaryModelSerializer.writeTag(output, "GEOM");
      for (let i = 0; i < context.geometries.length; i++) {
        if (!BinaryModelSerializer.writeGeometryRecord(output, context.geometries[i], context)) {
          return false;
        }
      }

      BinaryModelSerializer.writeTag(output, "MODL");
      if (!BinaryModelSerializer.writeModelRecord(output, model, context)) {
        return false;
      }

      return true;
    }
    catch (_error) {
      Error.error("BinaryModelSerializer::write", "%s", "Unexpected failure while writing binary model");
      return false;
    }
    finally {
      if (output !== null) {
        output.dispose();
      }
    }
  }
}
