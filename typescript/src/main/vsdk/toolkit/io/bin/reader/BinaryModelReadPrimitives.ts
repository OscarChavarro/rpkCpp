import { InputStream } from "../../../../../java/io/InputStream";
import { ColorRgb } from "../../../common/color/ColorRgb";
import { Logger } from "../../../common/logging/Logger";
import { Vector3D } from "../../../common/linealAlgebra/Vector3D";
import { ParseSnapshotContext } from "../../context/ParseSnapshotContext";
import { BoundingBox } from "../../../skin/BoundingBox";
import { BoundingBoxCoordinateIndex } from "../../../skin/BoundingBoxCoordinateIndex";
import { BinaryModelIndexListRef } from "./BinaryModelIndexListRef";
import { BinaryModelSnapshotRecordData } from "./BinaryModelSnapshotRecordData";

export class BinaryModelReadPrimitives {
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

  private static readFully(input: InputStream | null, buffer: Uint8Array, length: number): number {
    if (input === null || length <= 0) {
      return 0;
    }

    let offset = 0;
    let numRead = 0;
    do {
      try {
        numRead = input.read(buffer, offset, length - offset);
      }
      catch (_error) {
        numRead = -1;
      }
      if (numRead <= 0) {
        break;
      }
      offset += numRead;
    } while (offset < length && numRead >= 0);

    return offset;
  }

  private static bytesAsDataView(bytes: Uint8Array): DataView {
    return new DataView(bytes.buffer, bytes.byteOffset, bytes.byteLength);
  }

  private static int64BytesToNumberLE(bytes: Uint8Array): number {
    const view = BinaryModelReadPrimitives.bytesAsDataView(bytes);
    try {
      return Number(view.getBigInt64(0, true));
    }
    catch (_error) {
      const low = view.getUint32(0, true);
      const high = view.getInt32(4, true);
      return low + high * 4294967296.0;
    }
  }

  public static reportReadError(routine: string, message: string): boolean {
    Logger.error(routine, "%s", message);
    return false;
  }

  public static initializeArrayList<T>(list: T[] | null, count: number, initialValue: T, what: string): boolean {
    if (list === null) {
      return BinaryModelReadPrimitives.reportReadError(
        "BinaryModelReadPrimitives::initializeArrayList",
        "Null list pointer"
      );
    }
    if (count < 0) {
      Logger.error(
        "BinaryModelReadPrimitives::initializeArrayList",
        "Negative count while reading binary model (%s)",
        what
      );
      return false;
    }

    for (let i = 0; i < count; i++) {
      list.push(initialValue);
    }
    return true;
  }

  public static releaseIndexListRecord(record: BinaryModelIndexListRef | null): void {
    if (record === null) {
      return;
    }
    if (record.indices !== null) {
      record.indices.length = 0;
      record.indices = null;
    }
    record.isNull = true;
  }

  public static readBytes(input: InputStream | null, buffer: Uint8Array | null, length: number): void {
    if (length <= 0 || buffer === null) {
      return;
    }

    if (length > buffer.length) {
      Logger.error(
        "BinaryModelReadPrimitives::readBytes",
        "Requested length exceeds destination buffer"
      );
      length = buffer.length;
    }

    const offset = BinaryModelReadPrimitives.readFully(input, buffer, length);
    if (offset < length) {
      for (let i = offset; i < length; i++) {
        buffer[i] = 0;
      }
      Logger.error(
        "BinaryModelReadPrimitives::readBytes",
        "could not read requested length (%d/%d)",
        offset,
        length
      );
    }
  }

  public static readBytesChunked(input: InputStream | null, buffer: Uint8Array | null, length: number): boolean {
    if (length <= 0) {
      return true;
    }
    if (buffer === null) {
      return BinaryModelReadPrimitives.reportReadError(
        "BinaryModelReadPrimitives::readBytesChunked",
        "Null input buffer"
      );
    }
    if (length > buffer.length) {
      return BinaryModelReadPrimitives.reportReadError(
        "BinaryModelReadPrimitives::readBytesChunked",
        "Requested length exceeds destination buffer"
      );
    }

    let offset = 0;
    const maxChunk = 2147483647;
    while (offset < length) {
      const remaining = length - offset;
      const chunk = remaining < maxChunk ? remaining : maxChunk;
      const chunkBuffer = new Uint8Array(chunk);
      BinaryModelReadPrimitives.readBytes(input, chunkBuffer, chunk);
      buffer.set(chunkBuffer, offset);
      offset += chunk;
    }

    return true;
  }

  public static readByte(input: InputStream | null): number {
    const value = new Uint8Array(1);
    BinaryModelReadPrimitives.readBytes(input, value, 1);
    return value[0];
  }

  public static readBool(input: InputStream | null): boolean {
    return BinaryModelReadPrimitives.readByte(input) !== 0;
  }

  public static readInt16LE(input: InputStream | null): number {
    const value = new Uint8Array(2);
    BinaryModelReadPrimitives.readBytes(input, value, 2);
    return BinaryModelReadPrimitives.bytesAsDataView(value).getInt16(0, true);
  }

  public static readInt32LE(input: InputStream | null): number {
    const value = new Uint8Array(4);
    BinaryModelReadPrimitives.readBytes(input, value, 4);
    return BinaryModelReadPrimitives.bytesAsDataView(value).getInt32(0, true);
  }

  public static readInt64LE(input: InputStream | null): number {
    const bytes = new Uint8Array(8);
    BinaryModelReadPrimitives.readBytes(input, bytes, 8);
    return BinaryModelReadPrimitives.int64BytesToNumberLE(bytes);
  }

  public static readFloatLE(input: InputStream | null): number {
    const value = new Uint8Array(4);
    BinaryModelReadPrimitives.readBytes(input, value, 4);
    return BinaryModelReadPrimitives.bytesAsDataView(value).getFloat32(0, true);
  }

  public static readDoubleLE(input: InputStream | null): number {
    const value = new Uint8Array(8);
    BinaryModelReadPrimitives.readBytes(input, value, 8);
    return BinaryModelReadPrimitives.bytesAsDataView(value).getFloat64(0, true);
  }

  public static expectTag(input: InputStream | null, expected: string | null): boolean {
    const tag = new Uint8Array(4);
    BinaryModelReadPrimitives.readBytes(input, tag, 4);

    if (expected === null || expected.length !== 4) {
      return BinaryModelReadPrimitives.reportReadError(
        "BinaryModelReadPrimitives::expectTag",
        "Unexpected section tag while reading binary model"
      );
    }

    if (
      tag[0] !== expected.charCodeAt(0)
      || tag[1] !== expected.charCodeAt(1)
      || tag[2] !== expected.charCodeAt(2)
      || tag[3] !== expected.charCodeAt(3)
    ) {
      return BinaryModelReadPrimitives.reportReadError(
        "BinaryModelReadPrimitives::expectTag",
        "Unexpected section tag while reading binary model"
      );
    }

    return true;
  }

  public static readNonNegativeCount(input: InputStream | null, what: string, count: number[] | null): boolean {
    if (count === null || count.length === 0) {
      return BinaryModelReadPrimitives.reportReadError(
        "BinaryModelReadPrimitives::readNonNegativeCount",
        "Null output count pointer"
      );
    }

    count[0] = BinaryModelReadPrimitives.readInt32LE(input);
    if (count[0] < 0) {
      Logger.error(
        "BinaryModelReadPrimitives::readNonNegativeCount",
        "Negative count while reading binary model (%s)",
        what
      );
      return false;
    }

    return true;
  }

  public static readNullableString(
    input: InputStream | null,
    value: Array<string | null> | null,
    hasValue: boolean[] | null
  ): boolean {
    if (value === null || value.length === 0 || hasValue === null || hasValue.length === 0) {
      return BinaryModelReadPrimitives.reportReadError(
        "BinaryModelReadPrimitives::readNullableString",
        "Null string output pointer"
      );
    }

    value[0] = null;
    hasValue[0] = false;

    const size = BinaryModelReadPrimitives.readInt32LE(input);
    if (size === -1) {
      return true;
    }
    if (size < -1) {
      return BinaryModelReadPrimitives.reportReadError(
        "BinaryModelReadPrimitives::readNullableString",
        "Invalid negative string size"
      );
    }

    const bytes = new Uint8Array(size);
    if (size > 0) {
      BinaryModelReadPrimitives.readBytes(input, bytes, size);
    }

    value[0] = Buffer.from(bytes).toString("utf8");
    hasValue[0] = true;
    return true;
  }

  public static duplicateNullableString(
    hasValue: boolean,
    value: string | null,
    text: Array<string | null> | null
  ): boolean {
    if (text === null || text.length === 0) {
      return BinaryModelReadPrimitives.reportReadError(
        "BinaryModelReadPrimitives::duplicateNullableString",
        "Null output string pointer"
      );
    }

    text[0] = null;
    if (!hasValue || value === null) {
      return true;
    }
    text[0] = `${value}`;
    return true;
  }

  public static readColor(input: InputStream | null, color: ColorRgb | null): boolean {
    if (color === null) {
      return BinaryModelReadPrimitives.reportReadError(
        "BinaryModelReadPrimitives::readColor",
        "Null color output pointer"
      );
    }

    color.r = BinaryModelReadPrimitives.readFloatLE(input);
    color.g = BinaryModelReadPrimitives.readFloatLE(input);
    color.b = BinaryModelReadPrimitives.readFloatLE(input);
    return true;
  }

  public static readVector(input: InputStream | null, vector: Vector3D | null): boolean {
    if (vector === null) {
      return BinaryModelReadPrimitives.reportReadError(
        "BinaryModelReadPrimitives::readVector",
        "Null vector output pointer"
      );
    }

    vector.x = BinaryModelReadPrimitives.readFloatLE(input);
    vector.y = BinaryModelReadPrimitives.readFloatLE(input);
    vector.z = BinaryModelReadPrimitives.readFloatLE(input);
    return true;
  }

  public static readBoundingBoxCoordinates(input: InputStream | null, coordinates: number[] | null): boolean {
    if (coordinates === null || coordinates.length < 6) {
      return BinaryModelReadPrimitives.reportReadError(
        "BinaryModelReadPrimitives::readBoundingBoxCoordinates",
        "Null bounding box coordinate buffer"
      );
    }

    for (let i = 0; i < 6; i++) {
      coordinates[i] = BinaryModelReadPrimitives.readFloatLE(input);
    }

    return true;
  }

  public static setBoundingBoxFromCoordinates(boundingBox: BoundingBox | null, coordinates: number[] | null): boolean {
    if (boundingBox === null || coordinates === null || coordinates.length < 6) {
      return BinaryModelReadPrimitives.reportReadError(
        "BinaryModelReadPrimitives::setBoundingBoxFromCoordinates",
        "Invalid bounding box assignment"
      );
    }

    const parsed = new BoundingBox();
    const minPoint = new Vector3D();
    const maxPoint = new Vector3D();

    minPoint.set(
      coordinates[BoundingBoxCoordinateIndex.MIN_X],
      coordinates[BoundingBoxCoordinateIndex.MIN_Y],
      coordinates[BoundingBoxCoordinateIndex.MIN_Z]
    );
    maxPoint.set(
      coordinates[BoundingBoxCoordinateIndex.MAX_X],
      coordinates[BoundingBoxCoordinateIndex.MAX_Y],
      coordinates[BoundingBoxCoordinateIndex.MAX_Z]
    );

    parsed.enlargeToIncludePoint(minPoint);
    parsed.enlargeToIncludePoint(maxPoint);
    boundingBox.copyFrom(parsed);

    return true;
  }

  public static readIndexList(
    input: InputStream | null,
    what: string,
    record: BinaryModelIndexListRef | null
  ): boolean {
    if (record === null) {
      return BinaryModelReadPrimitives.reportReadError(
        "BinaryModelReadPrimitives::readIndexList",
        "Null output record"
      );
    }

    record.isNull = false;
    record.indices = null;

    const count = BinaryModelReadPrimitives.readInt32LE(input);
    if (count === -1) {
      record.isNull = true;
      return true;
    }
    if (count < -1) {
      Logger.error(
        "BinaryModelReadPrimitives::readIndexList",
        "Negative index list count while reading binary model (%s)",
        what
      );
      return false;
    }

    const indices: number[] = [];
    for (let i = 0; i < count; i++) {
      indices.push(BinaryModelReadPrimitives.readInt32LE(input));
    }
    record.indices = indices;

    return true;
  }

  public static pointerFromIndex<T>(
    values: Array<T | null> | null,
    index: number,
    what: string,
    result: BinaryModelReadPrimitives.Out<T | null> | null
  ): boolean {
    if (result === null) {
      return BinaryModelReadPrimitives.reportReadError(
        "BinaryModelReadPrimitives::pointerFromIndex",
        "Null output pointer"
      );
    }

    result.value = null;
    if (index === -1) {
      return true;
    }

    if (values === null || index < 0 || index >= values.length) {
      Logger.error(
        "BinaryModelReadPrimitives::pointerFromIndex",
        "Out of range index while reading binary model (%s)",
        what
      );
      return false;
    }

    result.value = values[index];
    return true;
  }

  public static arrayListFromIndices<T>(
    record: BinaryModelIndexListRef | null,
    values: Array<T | null> | null,
    what: string,
    result: BinaryModelReadPrimitives.Out<Array<T | null> | null> | null
  ): boolean {
    if (result === null) {
      return BinaryModelReadPrimitives.reportReadError(
        "BinaryModelReadPrimitives::arrayListFromIndices",
        "Null output pointer"
      );
    }

    result.value = null;
    if (record === null || record.isNull) {
      return true;
    }
    if (record.indices === null) {
      return BinaryModelReadPrimitives.reportReadError(
        "BinaryModelReadPrimitives::arrayListFromIndices",
        "Missing index list while reading binary model"
      );
    }

    const list: Array<T | null> = [];
    const element = new BinaryModelReadPrimitives.Out<T | null>();
    for (let i = 0; i < record.indices.length; i++) {
      if (!BinaryModelReadPrimitives.pointerFromIndex(values, record.indices[i], what, element)) {
        return false;
      }
      list.push(element.value);
    }

    result.value = list;
    return true;
  }

  public static validateBinaryHeader(input: InputStream | null): boolean {
    const magic = new Uint8Array(16);
    BinaryModelReadPrimitives.readBytes(input, magic, 16);
    for (let i = 0; i < 16; i++) {
      if (magic[i] !== BinaryModelReadPrimitives.BINARY_MODEL_MAGIC[i]) {
        return BinaryModelReadPrimitives.reportReadError(
          "BinaryModelReadPrimitives::validateBinaryHeader",
          "Invalid binary model magic header"
        );
      }
    }

    const version = BinaryModelReadPrimitives.readInt32LE(input);
    if (version !== BinaryModelReadPrimitives.BINARY_MODEL_VERSION) {
      return BinaryModelReadPrimitives.reportReadError(
        "BinaryModelReadPrimitives::validateBinaryHeader",
        "Unsupported binary model version"
      );
    }

    const pointerSize = BinaryModelReadPrimitives.readInt32LE(input);
    const longSize = BinaryModelReadPrimitives.readInt32LE(input);
    const modelSize = BinaryModelReadPrimitives.readInt32LE(input);

    if (
      pointerSize !== BinaryModelReadPrimitives.BINARY_MODEL_POINTER_SIZE
      || longSize !== BinaryModelReadPrimitives.BINARY_MODEL_LONG_SIZE
      || modelSize !== BinaryModelReadPrimitives.BINARY_MODEL_PARSE_SNAPSHOT_CONTEXT_SIZE
    ) {
      return BinaryModelReadPrimitives.reportReadError(
        "BinaryModelReadPrimitives::validateBinaryHeader",
        "Incompatible binary model platform/type sizes"
      );
    }

    return true;
  }

  public static populateModelStrings(model: ParseSnapshotContext | null, record: BinaryModelSnapshotRecordData): boolean {
    if (model === null) {
      return BinaryModelReadPrimitives.reportReadError(
        "BinaryModelReadPrimitives::populateModelStrings",
        "Null model in string population"
      );
    }

    const tmp: Array<string | null> = [null];

    if (!BinaryModelReadPrimitives.duplicateNullableString(record.hasCurrentMaterialName, record.currentMaterialName, tmp)) {
      return false;
    }
    model.currentMaterialName = tmp[0];

    if (!BinaryModelReadPrimitives.duplicateNullableString(record.hasCurrentObjectName, record.currentObjectName, tmp)) {
      return false;
    }
    model.currentObjectName = tmp[0];

    if (!BinaryModelReadPrimitives.duplicateNullableString(record.hasCurrentVertexName, record.currentVertexName, tmp)) {
      return false;
    }
    model.currentVertexName = tmp[0];

    return true;
  }
}

export namespace BinaryModelReadPrimitives {
  export class Out<T> {
    public value: T | null;

    public constructor() {
      this.value = null;
    }
  }
}
