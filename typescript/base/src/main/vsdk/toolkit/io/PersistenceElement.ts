import { File } from "../../../java/io/File";
import { InputStream } from "../../../java/io/InputStream";
import { OutputStream } from "../../../java/io/OutputStream";
import { Logger as VsdkLogger } from "../common/logging/Logger";
import { VSDK } from "../common/VSDK";

const fs = require("node:fs");
const pathModule = require("node:path");

export abstract class PersistenceElement {
  private static readonly bigEndianArchitecture = false;

  private static readonly byteBuffer1byte = new Uint8Array(1);
  private static readonly byteBuffer2byte = new Uint8Array(2);
  private static readonly byteBuffer4byte = new Uint8Array(4);
  private static readonly byteBuffer8byte = new Uint8Array(8);

  // Long int should use an 8-sized array, not a 4-sized. Check.
  private static readonly bytesForLong = new Uint8Array(4);

  private static signedByte2unsignedInteger(value: number): number {
    return value & 0xFF;
  }

  private static toSignedByte(value: number): number {
    const b = value & 0xFF;
    return b > 127 ? b - 256 : b;
  }

  private static getByte(arr: Uint8Array, index: number): number {
    return arr[index]! & 0xFF;
  }

  public static readByteInt(is: InputStream): number {
    PersistenceElement.readBytes(is, PersistenceElement.byteBuffer1byte);
    return PersistenceElement.toSignedByte(PersistenceElement.byteBuffer1byte[0]!);
  }

  public static readByteUnsignedInt(is: InputStream): number {
    PersistenceElement.readBytes(is, PersistenceElement.byteBuffer1byte);
    return VSDK.signedByte2unsignedInteger(
      PersistenceElement.toSignedByte(PersistenceElement.byteBuffer1byte[0]!)
    );
  }

  public static writeByte(os: OutputStream, value: number): void {
    PersistenceElement.byteBuffer1byte[0] = value & 0xFF;
    PersistenceElement.writeBytes(os, PersistenceElement.byteBuffer1byte);
  }

  public static writeBool(os: OutputStream, value: boolean): void {
    PersistenceElement.writeByte(os, value ? 1 : 0);
  }

  public static readBytes(is: InputStream | null, bytesBuffer: Uint8Array | null): void;
  public static readBytes(is: InputStream | null, bytesBuffer: Uint8Array | null, length: number): void;
  public static readBytes(is: InputStream | null, bytesBuffer: Uint8Array | null, length?: number): void {
    const strict = length === undefined;
    if (bytesBuffer === null) {
      if (strict) {
        throw new globalThis.Error("bytesBuffer can not be null");
      }
      VsdkLogger.error("PersistenceElement::readBytes", "%s", "invalid buffer");
      return;
    }

    const targetLength = strict ? bytesBuffer.length : (length ?? 0);
    if (targetLength < 0) {
      VsdkLogger.error("PersistenceElement::readBytes", "%s", "invalid buffer");
      return;
    }
    if (targetLength === 0) {
      return;
    }

    if (is === null) {
      if (strict) {
        throw new globalThis.Error(`Could not read requested length (0/${targetLength})`);
      }
      for (let i = 0; i < targetLength && i < bytesBuffer.length; i++) {
        bytesBuffer[i] = 0;
      }
      VsdkLogger.error(
        "PersistenceElement::readBytes",
        "could not read requested length (%d/%d)",
        0,
        targetLength
      );
      return;
    }

    let offset = 0;
    let numRead = 0;
    do {
      try {
        numRead = is.read(bytesBuffer, offset, targetLength - offset);
      }
      catch (_e) {
        numRead = -1;
      }
      if (numRead <= 0) {
        break;
      }
      offset += numRead;
    } while (offset < targetLength && numRead >= 0);

    if (offset < targetLength) {
      if (strict) {
        throw new globalThis.Error(`Could not read requested length (${offset}/${targetLength})`);
      }
      for (let i = offset; i < targetLength && i < bytesBuffer.length; i++) {
        bytesBuffer[i] = 0;
      }
      VsdkLogger.error(
        "PersistenceElement::readBytes",
        "could not read requested length (%d/%d)",
        offset,
        targetLength
      );
    }
  }

  public static writeBytes(os: OutputStream | null, bytesBuffer: Uint8Array | null): void;
  public static writeBytes(os: OutputStream | null, bytesBuffer: Uint8Array | null, length: number): void;
  public static writeBytes(os: OutputStream | null, bytesBuffer: Uint8Array | null, length?: number): void {
    if (bytesBuffer === null || os === null) {
      if (length !== undefined) {
        VsdkLogger.error("PersistenceElement::writeBytes", "%s", "invalid arguments");
      }
      return;
    }
    const targetLength = length === undefined ? bytesBuffer.length : length;
    if (targetLength < 0) {
      VsdkLogger.error("PersistenceElement::writeBytes", "%s", "invalid arguments");
      return;
    }
    if (targetLength === 0) {
      return;
    }
    try {
      os.write(bytesBuffer, 0, targetLength);
    }
    catch (_e) {
    }
  }

  private static signedShort2byteArrayDirect(
    outArrayToBeExported: Uint8Array,
    inStartIndexInsideArray: number,
    inNumberToConvert: number
  ): void {
    const length = 2;
    for (let i = 0; i < length; i++) {
      PersistenceElement.byteBuffer2byte[i] = (inNumberToConvert >> (8 * i)) & 0xFF;
    }
    for (let i = inStartIndexInsideArray, cnt = 0; i < inStartIndexInsideArray + length; i++, cnt++) {
      outArrayToBeExported[i] = PersistenceElement.byteBuffer2byte[cnt]!;
    }
  }

  private static signedShort2byteArrayInvert(
    outArrayToBeExported: Uint8Array,
    inStartIndexInsideArray: number,
    inNumberToConvert: number
  ): void {
    const length = 2;
    for (let i = 0; i < length; i++) {
      PersistenceElement.byteBuffer2byte[length - i - 1] = (inNumberToConvert >> (8 * i)) & 0xFF;
    }
    for (let i = inStartIndexInsideArray, cnt = 0; i < inStartIndexInsideArray + length; i++, cnt++) {
      outArrayToBeExported[i] = PersistenceElement.byteBuffer2byte[cnt]!;
    }
  }

  private static byteArray2signedShortDirect(arr: Uint8Array, start: number): number {
    const low = PersistenceElement.getByte(arr, start);
    const high = PersistenceElement.getByte(arr, start + 1);
    return (high << 8) | low;
  }

  private static byteArray2signedShortInvert(arr: Uint8Array, start: number): number {
    const low = PersistenceElement.getByte(arr, start);
    const high = PersistenceElement.getByte(arr, start + 1);
    return (low << 8) | high;
  }

  private static byteArray2longDirect(arr: Uint8Array, start: number): number {
    let accum = 0;
    for (let i = 0; i < 4; i++) {
      accum += PersistenceElement.getByte(arr, start + i) * globalThis.Math.pow(2, 8 * i);
    }
    return accum;
  }

  private static signedInt2byteArrayDirect(arr: Uint8Array, start: number, num: number): void {
    const n = BigInt.asUintN(32, BigInt(globalThis.Math.trunc(num)));
    for (let i = 0; i < 4; i++) {
      arr[start + i] = Number((n >> BigInt(8 * i)) & 0xFFn);
    }
  }

  private static signedInt2byteArrayInvert(arr: Uint8Array, start: number, num: number): void {
    const n = BigInt.asUintN(32, BigInt(globalThis.Math.trunc(num)));
    for (let i = 0; i < 4; i++) {
      arr[start + (3 - i)] = Number((n >> BigInt(8 * i)) & 0xFFn);
    }
  }

  private static byteArray2longInvert(arr: Uint8Array, start: number): number {
    let accum = 0;
    for (let i = 0; i < 4; i++) {
      const src = start + (3 - i);
      accum += PersistenceElement.getByte(arr, src) * globalThis.Math.pow(2, 8 * i);
    }
    return accum;
  }

  private static byteArray2floatDirect(arr: Uint8Array, start: number): number {
    const tmp = new Uint8Array(4);
    for (let i = 0; i < 4; i++) {
      tmp[i] = arr[start + i]!;
    }
    return new DataView(tmp.buffer).getFloat32(0, true);
  }

  private static byteArray2doubleDirect(arr: Uint8Array, start: number): number {
    const tmp = new Uint8Array(8);
    for (let i = 0; i < 8; i++) {
      tmp[i] = arr[start + i]!;
    }
    return new DataView(tmp.buffer).getFloat64(0, true);
  }

  private static byteArray2floatInvert(arr: Uint8Array, start: number): number {
    const tmp = new Uint8Array(4);
    for (let i = 0; i < 4; i++) {
      tmp[3 - i] = arr[start + i]!;
    }
    return new DataView(tmp.buffer).getFloat32(0, true);
  }

  private static byteArray2doubleInvert(arr: Uint8Array, start: number): number {
    const tmp = new Uint8Array(8);
    for (let i = 0; i < 8; i++) {
      tmp[7 - i] = arr[start + i]!;
    }
    return new DataView(tmp.buffer).getFloat64(0, true);
  }

  public static byteArray2signedShortBE(arr: Uint8Array, start: number): number {
    if (PersistenceElement.bigEndianArchitecture) {
      return PersistenceElement.byteArray2signedShortDirect(arr, start);
    }
    return PersistenceElement.byteArray2signedShortInvert(arr, start);
  }

  public static signedShort2byteArrayBE(arr: Uint8Array, start: number, num: number): void {
    if (PersistenceElement.bigEndianArchitecture) {
      PersistenceElement.signedShort2byteArrayDirect(arr, start, num);
    }
    PersistenceElement.signedShort2byteArrayInvert(arr, start, num);
  }

  public static signedShort2byteArrayLE(arr: Uint8Array, start: number, num: number): void {
    if (PersistenceElement.bigEndianArchitecture) {
      PersistenceElement.signedShort2byteArrayInvert(arr, start, num);
    }
    PersistenceElement.signedShort2byteArrayDirect(arr, start, num);
  }

  public static byteArray2signedShortLE(arr: Uint8Array, start: number): number {
    if (PersistenceElement.bigEndianArchitecture) {
      return PersistenceElement.byteArray2signedShortInvert(arr, start);
    }
    return PersistenceElement.byteArray2signedShortDirect(arr, start);
  }

  public static byteArray2longBE(arr: Uint8Array, start: number): number {
    if (PersistenceElement.bigEndianArchitecture) {
      return PersistenceElement.byteArray2longDirect(arr, start);
    }
    return PersistenceElement.byteArray2longInvert(arr, start);
  }

  public static byteArray2longLE(arr: Uint8Array, start: number): number {
    if (PersistenceElement.bigEndianArchitecture) {
      return PersistenceElement.byteArray2longInvert(arr, start);
    }
    return PersistenceElement.byteArray2longDirect(arr, start);
  }

  // Keeps Java behavior (legacy conversion quirk).
  public static byteArray2floatBE(arr: Uint8Array, start: number): number {
    if (PersistenceElement.bigEndianArchitecture) {
      return PersistenceElement.byteArray2longDirect(arr, start);
    }
    return PersistenceElement.byteArray2longInvert(arr, start);
  }

  public static float2byteArrayBE(arr: Uint8Array, start: number, num: number): void {
    const bits = new DataView(new ArrayBuffer(4));
    bits.setFloat32(0, num, true);
    const asInt = bits.getInt32(0, true);
    if (PersistenceElement.bigEndianArchitecture) {
      PersistenceElement.signedInt2byteArrayDirect(arr, start, asInt);
    }
    PersistenceElement.signedInt2byteArrayInvert(arr, start, asInt);
  }

  public static float2byteArrayLE(arr: Uint8Array, start: number, num: number): void {
    const bits = new DataView(new ArrayBuffer(4));
    bits.setFloat32(0, num, true);
    const asInt = bits.getInt32(0, true);
    if (PersistenceElement.bigEndianArchitecture) {
      PersistenceElement.signedInt2byteArrayInvert(arr, start, asInt);
    }
    PersistenceElement.signedInt2byteArrayDirect(arr, start, asInt);
  }

  public static byteArray2floatLE(arr: Uint8Array, start: number): number {
    if (PersistenceElement.bigEndianArchitecture) {
      return PersistenceElement.byteArray2floatInvert(arr, start);
    }
    return PersistenceElement.byteArray2floatDirect(arr, start);
  }

  public static byteArray2doubleLE(arr: Uint8Array, start: number): number {
    if (PersistenceElement.bigEndianArchitecture) {
      return PersistenceElement.byteArray2doubleInvert(arr, start);
    }
    return PersistenceElement.byteArray2doubleDirect(arr, start);
  }

  public static byteArray2doubleBE(arr: Uint8Array, start: number): number {
    if (PersistenceElement.bigEndianArchitecture) {
      return PersistenceElement.byteArray2doubleDirect(arr, start);
    }
    return PersistenceElement.byteArray2doubleInvert(arr, start);
  }

  public static readSignedShortLE(is: InputStream): number {
    PersistenceElement.readBytes(is, PersistenceElement.byteBuffer2byte);
    return PersistenceElement.byteArray2signedShortLE(PersistenceElement.byteBuffer2byte, 0);
  }

  public static readSignedShortBE(is: InputStream): number {
    const arr = new Uint8Array(2);
    PersistenceElement.readBytes(is, arr);
    return PersistenceElement.byteArray2signedShortBE(arr, 0);
  }

  public static writeSignedShortBE(os: OutputStream, num: number): void {
    PersistenceElement.signedShort2byteArrayBE(PersistenceElement.byteBuffer2byte, 0, num);
    PersistenceElement.writeBytes(os, PersistenceElement.byteBuffer2byte);
  }

  public static writeSignedShortLE(os: OutputStream, num: number): void {
    PersistenceElement.signedShort2byteArrayLE(PersistenceElement.byteBuffer2byte, 0, num);
    PersistenceElement.writeBytes(os, PersistenceElement.byteBuffer2byte);
  }

  public static readLongLE(is: InputStream): number {
    PersistenceElement.readBytes(is, PersistenceElement.bytesForLong);
    return PersistenceElement.byteArray2longLE(PersistenceElement.bytesForLong, 0);
  }

  public static writeInt32LE(os: OutputStream, num: number): void {
    PersistenceElement.writeLongLE(os, num);
  }

  public static writeInt64LE(os: OutputStream, num: number): void {
    const bits = BigInt.asUintN(64, BigInt(globalThis.Math.trunc(num)));
    const low = Number(bits & 0xFFFFFFFFn);
    const high = Number((bits >> 32n) & 0xFFFFFFFFn);
    PersistenceElement.writeInt32LE(os, low);
    PersistenceElement.writeInt32LE(os, high);
  }

  public static writeDoubleLE(os: OutputStream, num: number): void {
    const dv = new DataView(new ArrayBuffer(8));
    dv.setFloat64(0, num, true);
    const bits = dv.getBigUint64(0, true);
    const low = Number(bits & 0xFFFFFFFFn);
    const high = Number((bits >> 32n) & 0xFFFFFFFFn);
    PersistenceElement.writeInt32LE(os, low);
    PersistenceElement.writeInt32LE(os, high);
  }

  public static readLongBE(is: InputStream): number {
    PersistenceElement.readBytes(is, PersistenceElement.bytesForLong);
    return PersistenceElement.byteArray2longBE(PersistenceElement.bytesForLong, 0);
  }

  public static readFloatLE(is: InputStream): number {
    PersistenceElement.readBytes(is, PersistenceElement.byteBuffer4byte);
    return PersistenceElement.byteArray2floatLE(PersistenceElement.byteBuffer4byte, 0);
  }

  public static readDoubleLE(is: InputStream): number {
    PersistenceElement.readBytes(is, PersistenceElement.byteBuffer8byte);
    return PersistenceElement.byteArray2doubleLE(PersistenceElement.byteBuffer8byte, 0);
  }

  public static readDoubleBE(is: InputStream): number {
    PersistenceElement.readBytes(is, PersistenceElement.byteBuffer8byte);
    return PersistenceElement.byteArray2doubleBE(PersistenceElement.byteBuffer8byte, 0);
  }

  public static readFloatBE(is: InputStream): number {
    PersistenceElement.readBytes(is, PersistenceElement.byteBuffer4byte);
    const i = PersistenceElement.byteArray2longBE(PersistenceElement.byteBuffer4byte, 0);
    const dv = new DataView(new ArrayBuffer(4));
    dv.setInt32(0, i | 0, true);
    return dv.getFloat32(0, true);
  }

  public static writeFloatBE(os: OutputStream, num: number): void {
    PersistenceElement.float2byteArrayBE(PersistenceElement.byteBuffer4byte, 0, num);
    PersistenceElement.writeBytes(os, PersistenceElement.byteBuffer4byte);
  }

  public static writeFloatLE(os: OutputStream, num: number): void {
    PersistenceElement.float2byteArrayLE(PersistenceElement.byteBuffer4byte, 0, num);
    PersistenceElement.writeBytes(os, PersistenceElement.byteBuffer4byte);
  }

  public static writeLongBE(os: OutputStream, num: number): void {
    if (PersistenceElement.bigEndianArchitecture) {
      PersistenceElement.signedInt2byteArrayDirect(PersistenceElement.bytesForLong, 0, num);
    }
    PersistenceElement.signedInt2byteArrayInvert(PersistenceElement.bytesForLong, 0, num);
    PersistenceElement.writeBytes(os, PersistenceElement.bytesForLong);
  }

  public static writeLongLE(os: OutputStream, num: number): void {
    if (PersistenceElement.bigEndianArchitecture) {
      PersistenceElement.signedInt2byteArrayInvert(PersistenceElement.bytesForLong, 0, num);
    }
    PersistenceElement.signedInt2byteArrayDirect(PersistenceElement.bytesForLong, 0, num);
    PersistenceElement.writeBytes(os, PersistenceElement.bytesForLong);
  }

  public static readAsciiFixedSizeString(is: InputStream, size: number): string {
    if (size <= 0) {
      return "";
    }
    const bytesForString = new Uint8Array(size);
    PersistenceElement.readBytes(is, bytesForString);
    const msg = Buffer.from(bytesForString).toString("utf8");
    const skip = new Uint8Array(1);
    PersistenceElement.readBytes(is, skip);
    return msg;
  }

  public static readAsciiString(is: InputStream): string {
    const character = new Uint8Array(1);
    let msg = "";
    do {
      PersistenceElement.readBytes(is, character);
      if (character[0] !== 0x00) {
        msg += String.fromCharCode(character[0]!);
      }
    } while (character[0] !== 0x00);
    return msg;
  }

  private static streamAvailable(is: InputStream): number {
    const anyStream = is as unknown as { available?: () => number };
    if (typeof anyStream.available === "function") {
      try {
        return anyStream.available();
      }
      catch (_e) {
      }
    }
    return 1;
  }

  public static readUtf8String(is: InputStream): string {
    const character = new Uint8Array(1);
    let msg = "";
    const a = new Uint8Array(2);

    do {
      PersistenceElement.readBytes(is, character);
      const letter = character[0]!;
      if (character[0] !== 0x00 && ((letter >> 7) === 0)) {
        msg += String.fromCharCode(letter);
      }
      else if (character[0] !== 0x00) {
        a[0] = character[0]!;
        if (PersistenceElement.streamAvailable(is) >= 1) {
          PersistenceElement.readBytes(is, character);
          a[1] = character[0]!;
          const cc = PersistenceElement.buildUtf8Char(a);
          if (cc !== null) {
            msg += cc;
          }
          else {
            process.stdout.write(`* UNHANDLED UTF! ********************************************************** ->${msg}\n`);
          }
        }
      }
    } while (character[0] !== 0x00);

    return msg;
  }

  public static buildUtf8Char(arr: Uint8Array): string | null;
  public static buildUtf8Char(arr: Uint8Array, outBytes: Uint8Array): boolean;
  public static buildUtf8Char(arr: Uint8Array, outBytes?: Uint8Array): string | boolean | null {
    if (outBytes === undefined) {
      const out = new Uint8Array(2);
      if (!PersistenceElement.buildUtf8Char(arr, out)) {
        return null;
      }
      return Buffer.from(out).toString("utf8");
    }

    if (arr.length < 2 || outBytes.length < 2) {
      return false;
    }
    const a = PersistenceElement.signedByte2unsignedInteger(PersistenceElement.toSignedByte(arr[0]!));
    const b = PersistenceElement.signedByte2unsignedInteger(PersistenceElement.toSignedByte(arr[1]!));
    if (((a >> 5) === 0x06) && ((b >> 6) === 0x02)) {
      outBytes[0] = arr[0]!;
      outBytes[1] = arr[1]!;
      return true;
    }
    return false;
  }

  public static readUtf8Line(is: InputStream): string {
    const character = new Uint8Array(1);
    let msg = "";
    const a = new Uint8Array(2);

    do {
      if (PersistenceElement.streamAvailable(is) < 1) {
        return "";
      }
      PersistenceElement.readBytes(is, character);
      const letter = character[0]!;
      if (character[0] !== "\n".charCodeAt(0) && character[0] !== "\r".charCodeAt(0) && ((letter >> 7) === 0)) {
        msg += String.fromCharCode(letter);
      }
      else if (character[0] !== "\n".charCodeAt(0) && character[0] !== "\r".charCodeAt(0)) {
        a[0] = character[0]!;
        if (PersistenceElement.streamAvailable(is) >= 1) {
          PersistenceElement.readBytes(is, character);
          a[1] = character[0]!;
          const cc = PersistenceElement.buildUtf8Char(a);
          if (cc !== null) {
            msg += cc;
          }
        }
      }
    } while (character[0] !== "\n".charCodeAt(0));

    return msg;
  }

  public static readAsciiLine(is: InputStream): string {
    const character = new Uint8Array(1);
    let out = "";
    do {
      PersistenceElement.readBytes(is, character);
      if (character[0] !== "\n".charCodeAt(0) && character[0] !== "\r".charCodeAt(0)) {
        out += String.fromCharCode(character[0]!);
      }
    } while (character[0] !== "\n".charCodeAt(0));
    return out;
  }

  private static isInSet(key: number, set: Uint8Array): boolean;
  private static isInSet(key: number, set: Uint8Array, setLength: number): boolean;
  private static isInSet(key: number, set: Uint8Array, setLength?: number): boolean {
    const limit = setLength === undefined ? set.length : globalThis.Math.min(setLength, set.length);
    for (let i = 0; i < limit; i++) {
      if (key === set[i]!) {
        return true;
      }
    }
    return false;
  }

  public static readAsciiToken(is: InputStream, separators: Uint8Array): string;
  public static readAsciiToken(is: InputStream, separators: Uint8Array, separatorsLength: number): string;
  public static readAsciiToken(is: InputStream, separators: Uint8Array, separatorsLength?: number): string {
    const character = new Uint8Array(1);
    let msg = "";

    if (separatorsLength === undefined) {
      do {
        PersistenceElement.readBytes(is, character);
        if (!PersistenceElement.isInSet(character[0]!, separators)) {
          msg += String.fromCharCode(character[0]!);
        }
      } while (!PersistenceElement.isInSet(character[0]!, separators));
      return msg;
    }

    do {
      PersistenceElement.readBytes(is, character, 1);
      if (character[0]! === 0x00) {
        break;
      }
      if (!PersistenceElement.isInSet(character[0]!, separators, separatorsLength)) {
        msg += String.fromCharCode(character[0]!);
      }
    } while (!PersistenceElement.isInSet(character[0]!, separators, separatorsLength));

    return msg;
  }

  public static writeAsciiString(writer: OutputStream, cad: string | null): void {
    const text = cad ?? "";
    const arr = Buffer.from(text, "utf8");
    PersistenceElement.writeBytes(writer, arr, arr.length);
    const end = new Uint8Array(1);
    end[0] = 0;
    PersistenceElement.writeBytes(writer, end, 1);
  }

  public static writeUtf8String(writer: OutputStream, cad: string | null): void {
    const text = cad ?? "";
    const arr = Buffer.from(text, "utf8");
    PersistenceElement.writeBytes(writer, arr, arr.length);
    const end = new Uint8Array(1);
    end[0] = 0;
    PersistenceElement.writeBytes(writer, end, 1);
  }

  public static writeAsciiLine(writer: OutputStream, cad: string | null): void {
    const text = cad ?? "";
    const arr = Buffer.from(text, "utf8");
    PersistenceElement.writeBytes(writer, arr, arr.length);
    const end = new Uint8Array(1);
    end[0] = "\n".charCodeAt(0);
    PersistenceElement.writeBytes(writer, end, 1);
  }

  public static writeUtf8Line(writer: OutputStream, cad: string | null): void {
    const text = cad ?? "";
    const arr = Buffer.from(text, "utf8");
    PersistenceElement.writeBytes(writer, arr, arr.length);
    const end = new Uint8Array(1);
    end[0] = "\n".charCodeAt(0);
    PersistenceElement.writeBytes(writer, end, 1);
  }

  private static duplicateCString(text: string | null): string {
    return `${text ?? ""}`;
  }

  private static joinCString2(left: string | null, right: string | null): string {
    return `${left ?? ""}${right ?? ""}`;
  }

  private static joinCString3(first: string | null, second: string | null, third: string | null): string {
    return `${first ?? ""}${second ?? ""}${third ?? ""}`;
  }

  private static containsCString(text: string | null, fragment: string | null): boolean {
    if (text === null || fragment === null) {
      return false;
    }
    return text.includes(fragment);
  }

  private static startsWithCString(text: string | null, prefix: string | null): boolean {
    if (text === null || prefix === null) {
      return false;
    }
    return text.startsWith(prefix);
  }

  private static endsWithCString(text: string | null, suffix: string | null): boolean {
    if (text === null || suffix === null) {
      return false;
    }
    return text.endsWith(suffix);
  }

  private static containsExistingLibrary(pathList: string | null, pathSeparator: string, nativeLibname: string): boolean {
    if (pathList === null || nativeLibname.length === 0) {
      return false;
    }

    let start = 0;
    while (start <= pathList.length) {
      let separatorIndex = pathList.indexOf(pathSeparator, start);
      if (separatorIndex < 0) {
        separatorIndex = pathList.length;
      }

      const token = pathList.substring(start, separatorIndex);
      if (token.length > 0) {
        const fullPath = PersistenceElement.joinCString3(token, "/", nativeLibname);
        const candidate = new File(fullPath);
        const ok = candidate.exists() && candidate.canRead() && candidate.isFile();
        candidate.dispose();
        if (ok) {
          return true;
        }
      }

      if (separatorIndex >= pathList.length) {
        break;
      }
      start = separatorIndex + 1;
    }

    return false;
  }

  private static mapLibraryName(libname: string): string {
    if (process.platform === "win32") {
      return `${libname}.dll`;
    }
    if (process.platform === "darwin") {
      return `lib${libname}.dylib`;
    }
    return `lib${libname}.so`;
  }

  public static verifyLibrary(libname: string): boolean {
    const nativeLibname = PersistenceElement.mapLibraryName(libname ?? "");
    let paths = process.env.JAVA_LIBRARY_PATH ?? process.env.PATH ?? "";
    const osName = process.platform.toLowerCase();

    if (osName.startsWith("linux") || osName.startsWith("sunos") || osName.startsWith("unix")) {
      paths = PersistenceElement.joinCString2(paths, ":/lib");
      paths = PersistenceElement.joinCString2(paths, ":/usr/lib");
      paths = PersistenceElement.joinCString2(paths, ":/usr/local/lib");
      paths = PersistenceElement.joinCString2(paths, ":/usr/X11R6/lib");
      paths = PersistenceElement.joinCString2(paths, ":/usr/X11R6/lib64");
      paths = PersistenceElement.joinCString2(paths, ":/usr/openwin/lib");
      paths = PersistenceElement.joinCString2(paths, ":/usr/dt/lib");
      paths = PersistenceElement.joinCString2(paths, ":/lib64");
      paths = PersistenceElement.joinCString2(paths, ":/usr/lib64");
      paths = PersistenceElement.joinCString2(paths, ":/usr/local/lib64");
      paths = PersistenceElement.joinCString2(paths, `:${process.env.LD_LIBRARY_PATH ?? ""}`);
    }

    const separator = pathModule.delimiter.charAt(0);
    return PersistenceElement.containsExistingLibrary(paths, separator, nativeLibname);
  }

  public static checkDirectory(dirName: string | null): boolean {
    if (dirName === null || dirName.length === 0) {
      process.stderr.write("Directory name is empty.\n");
      return false;
    }

    try {
      const stats = fs.statSync(dirName);
      if (!stats.isDirectory()) {
        process.stderr.write(
          `Directory ${dirName} is not accessible and automatic creation is disabled.\n`
        );
        return false;
      }
      fs.accessSync(dirName, fs.constants.R_OK | fs.constants.W_OK);
      return true;
    }
    catch (_e) {
      process.stderr.write(
        `Directory ${dirName} is not accessible and automatic creation is disabled.\n`
      );
      return false;
    }
  }

  protected static extractExtensionFromFile(fd: File): string {
    const javaFilename = fd.getName().toCString();
    if (javaFilename === null || javaFilename.length === 0) {
      return PersistenceElement.duplicateCString("");
    }

    let extension = javaFilename;
    let cursor = 0;
    for (;;) {
      const dot = javaFilename.indexOf(".", cursor);
      if (dot < 0) {
        break;
      }
      extension = javaFilename.substring(dot + 1);
      cursor = dot + 1;
    }
    return PersistenceElement.duplicateCString(extension);
  }
}
