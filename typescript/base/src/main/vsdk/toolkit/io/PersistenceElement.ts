import { InputStream } from "../../../java/io/InputStream.js";
import { OutputStream } from "../../../java/io/OutputStream.js";
import { Float } from "../../../java/lang/Float.js";
import { Double } from "../../../java/lang/Double.js";
import { VSDK } from "../common/VSDK.js";

/**
A `PersistenceElement` in VitralSDK is a software element with
algorithms and data structures (i.e. a class) with the specific functionality
of providing persistence operations for a data Entity.

The PersistenceElement abstract class provides an interface for *Persistence
style classes. This serves three purposes:
  - To help in design level organization of persistence classes (this eases the
    study of the class hierarchy)
  - To provide a place to locate operations common to all persistence classes
    and persistence private utility/supporting classes.  In particular, this
    class contains basic low level persistence operations for converting bit
    streams from and to basic numeric data types. Note that this code is NOT
    portable, as it needs explicit programmer configuration for little-endian
    or big-endian hardware platform (programmer should take care about how
    to configure attribute bigEndianArchitecture).
  - To provide means of accessing some operating system's native library
    files and other basic file system management.

Note that there are several methods used to handle byte arrays and change
between little endian and bit endian orders. When the copies are done on
the same order (from little endian to little endian or from big endian to
big endian) the "Direct" versions are used. When copies are done on the
reverse order (from little endian to big endian or from big endian to
little endian) the "Invert" versions are used.

This class is the platform-neutral half of the Java original: the three
methods bound to `java.io.File` (`verifyLibrary`, `checkDirectory` and
`extractExtensionFromFile`) live in the `@vitral/fs` subclass of the same
name, because a browser frontend has no local filesystem to reach them with.
*/
export abstract class PersistenceElement {
    private static readonly bigEndianArchitecture = false;

    // Those are not thread safe / re-entrant... each different thread should
    // use its own arrays
    private static readonly byteBuffer1byte = new Uint8Array(1);
    private static readonly byteBuffer2byte = new Uint8Array(2);
    private static readonly byteBuffer4byte = new Uint8Array(4);
    private static readonly byteBuffer8byte = new Uint8Array(8);

    // Long int should use an 8-sized array, not a 4-sized. Check.
    private static readonly bytesForLong = new Uint8Array(4);

    public static readByteInt(is: InputStream): number {
        let a: number;

        is.readBytes(PersistenceElement.byteBuffer1byte, 0, 1);
        a = VSDK.unsigned8BitInteger2signedByte(PersistenceElement.byteBuffer1byte[0] as number);

        return a;
    }

    public static readByteUnsignedInt(is: InputStream): number {
        let a: number;

        is.readBytes(PersistenceElement.byteBuffer1byte, 0, 1);
        a = PersistenceElement.byteBuffer1byte[0] as number;

        return a;
    }

    /**
    Given a previously initialized array of bytes, this method fills it
    with information readed from the given input stream.  If it is not
    enough information to read, this method generates an Exception.
    */
    public static readBytes(is: InputStream, bytesBuffer: Uint8Array): void {
        let offset = 0;
        let numRead: number;
        const length = bytesBuffer.length;
        do {
            numRead = is.readBytes(bytesBuffer, offset, length - offset);
            offset += numRead;
        } while (offset < length && numRead >= 0);
    }

    /**
    Given a previously initialized array of bytes, this method writes it
    with information readed from the given output stream.  If it is not
    enough information to read, this method generates an Exception.
    */
    public static writeBytes(os: OutputStream, bytesBuffer: Uint8Array): void {
        os.writeBytes(bytesBuffer, 0, bytesBuffer.length);
    }

    /**
    Receives an signed 16 bits integer (C++ short) and exports its data to a
    signed 8 bit byte array in direct endianess order.

    Pending to check: verify if inNumberToConvert parameter could be used
    of short type.
    */
    private static signedShort2byteArrayDirect(
        outArrayToBeExported: Uint8Array,
        inStartIndexInsideArray: number,
        inNumberToConvert: number,
    ): void {
        let i: number;
        const length = 2;

        // Convert number to array
        for (i = 0; i < length; i++) {
            PersistenceElement.byteBuffer2byte[i] = (inNumberToConvert & (0xff << (8 * i))) >> (8 * i);
        }

        // Export subarray to end array
        let cnt: number;
        for (i = inStartIndexInsideArray, cnt = 0; i < inStartIndexInsideArray + length; i++, cnt++) {
            outArrayToBeExported[i] = PersistenceElement.byteBuffer2byte[cnt] as number;
        }
    }

    /**
    Receives an signed 16 bits integer (C++ short) and exports its data to a
    signed 8 bit byte array in reverse endianess order.

    Pending to check: verify if inNumberToConvert parameter could be used
    of short type.
    */
    private static signedShort2byteArrayInvert(
        outArrayToBeExported: Uint8Array,
        inStartIndexInsideArray: number,
        inNumberToConvert: number,
    ): void {
        let i: number;
        const lenght = 2;

        // Convert number to array
        for (i = 0; i < lenght; i++) {
            PersistenceElement.byteBuffer2byte[lenght - i - 1] = (inNumberToConvert & (0xff << (8 * i))) >> (8 * i);
        }

        // Export subarray to end array
        let cnt: number;
        for (i = inStartIndexInsideArray, cnt = 0; i < inStartIndexInsideArray + lenght; i++, cnt++) {
            outArrayToBeExported[i] = PersistenceElement.byteBuffer2byte[cnt] as number;
        }
    }

    private static byteArray2signedShortDirect(arr: Uint8Array, start: number): number {
        const low = (arr[start] as number) & 0xff;
        const high = (arr[start + 1] as number) & 0xff;
        return (high << 8) | low;
    }

    private static byteArray2signedShortInvert(arr: Uint8Array, start: number): number {
        const low = (arr[start] as number) & 0xff;
        const high = (arr[start + 1] as number) & 0xff;
        return (low << 8) | high;
    }

    private static byteArray2longDirect(arr: Uint8Array, start: number): number {
        let i: number;
        const len = 4;
        let cnt = 0;
        const tmp = new Uint8Array(len);
        for (i = start; i < start + len; i++) {
            tmp[cnt] = arr[i] as number;
            cnt++;
        }
        let accum = 0n;
        i = 0;
        for (let shiftBy = 0; shiftBy < 32; shiftBy += 8) {
            accum |= BigInt((tmp[i] as number) & 0xff) << BigInt(shiftBy);
            i++;
        }
        return Number(accum);
    }

    private static signedInt2byteArrayDirect(arr: Uint8Array, start: number, num: number): void {
        let i: number;
        const len = 4;
        const tmp = new Uint8Array(len);

        // Convert number to array
        for (i = 0; i < len; i++) {
            tmp[i] = (num & (0xff << (8 * i))) >> (8 * i);
        }

        // Export subarray to end array
        let cnt: number;
        for (i = start, cnt = 0; i < start + len; i++, cnt++) {
            arr[i] = tmp[cnt] as number;
        }
    }

    private static signedInt2byteArrayInvert(arr: Uint8Array, start: number, num: number): void {
        let i: number;
        const len = 4;
        const tmp = new Uint8Array(len);

        // Convert number to array
        for (i = 0; i < len; i++) {
            tmp[len - i - 1] = (num & (0xff << (8 * i))) >> (8 * i);
        }

        // Export subarray to end array
        let cnt: number;
        for (i = start, cnt = 0; i < start + len; i++, cnt++) {
            arr[i] = tmp[cnt] as number;
        }
    }

    private static byteArray2longInvert(arr: Uint8Array, start: number): number {
        let i: number;
        const len = 4;
        let cnt = 3;
        const tmp = new Uint8Array(len);
        for (i = start; i < start + len; i++) {
            tmp[cnt] = arr[i] as number;
            cnt--;
        }
        let accum = 0n;
        i = 0;
        for (let shiftBy = 0; shiftBy < 32; shiftBy += 8) {
            accum |= BigInt((tmp[i] as number) & 0xff) << BigInt(shiftBy);
            i++;
        }
        return Number(accum);
    }

    private static byteArray2floatDirect(arr: Uint8Array, start: number): number {
        let i: number;
        const len = 4;
        let cnt: number;
        const tmp = new Uint8Array(len);

        for (i = start, cnt = 0; i < start + len; i++, cnt++) {
            tmp[cnt] = arr[i] as number;
        }
        let accum = 0;
        i = 0;
        for (let shiftBy = 0; shiftBy < 32; shiftBy += 8) {
            accum |= ((tmp[i] as number) & 0xff) << shiftBy;
            i++;
        }
        return Float.intBitsToFloat(accum);
    }

    private static byteArray2doubleDirect(arr: Uint8Array, start: number): number {
        let i: number;
        const len = 8;
        let cnt: number;
        const tmp = new Uint8Array(len);

        for (i = start, cnt = 0; i < start + len; i++, cnt++) {
            tmp[cnt] = arr[i] as number;
        }
        let accum = 0n;
        i = 0;
        for (let shiftBy = 0; shiftBy < 64; shiftBy += 8) {
            accum |= BigInt((tmp[i] as number) & 0xff) << BigInt(shiftBy);
            i++;
        }
        return Double.longBitsToDouble(BigInt.asIntN(64, accum));
    }

    private static byteArray2floatInvert(arr: Uint8Array, start: number): number {
        let i: number;
        const len = 4;
        let cnt = 3;
        const tmp = new Uint8Array(len);
        for (i = start; i < start + len; i++) {
            tmp[cnt] = arr[i] as number;
            cnt--;
        }
        let accum = 0;
        i = 0;
        for (let shiftBy = 0; shiftBy < 32; shiftBy += 8) {
            accum |= ((tmp[i] as number) & 0xff) << shiftBy;
            i++;
        }
        return Float.intBitsToFloat(accum);
    }

    private static byteArray2doubleInvert(arr: Uint8Array, start: number): number {
        let i: number;
        const len = 8;
        let cnt = 3;
        const tmp = new Uint8Array(len);
        for (i = start; i < start + len; i++) {
            tmp[cnt] = arr[i] as number;
            cnt--;
        }
        let accum = 0n;
        i = 0;
        for (let shiftBy = 0; shiftBy < 64; shiftBy += 8) {
            accum |= BigInt((tmp[i] as number) & 0xff) << BigInt(shiftBy);
            i++;
        }
        return Double.longBitsToDouble(BigInt.asIntN(64, accum));
    }

    /**
    This method is responsible of taking into account the endianess of the
    original data
    @return integer representation for given bit stream on big endian order
    */
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

    /**
    This method is responsible of taking into account the endianess of the
    original data
    @return integer representation for given bit stream on little endian order
    */
    public static byteArray2signedShortLE(arr: Uint8Array, start: number): number {
        if (PersistenceElement.bigEndianArchitecture) {
            return PersistenceElement.byteArray2signedShortInvert(arr, start);
        }
        return PersistenceElement.byteArray2signedShortDirect(arr, start);
    }

    /**
    This method is responsible of taking into account the endianess of the
    original data
    @return long integer representation for given bit stream on big endian order
    */
    public static byteArray2longBE(arr: Uint8Array, start: number): number {
        if (PersistenceElement.bigEndianArchitecture) {
            return PersistenceElement.byteArray2longDirect(arr, start);
        }
        return PersistenceElement.byteArray2longInvert(arr, start);
    }

    /**
    This method is responsible of taking into account the endianess of the
    original data
    @return long integer representation for given bit stream on little
    endian order
    */
    public static byteArray2longLE(arr: Uint8Array, start: number): number {
        if (PersistenceElement.bigEndianArchitecture) {
            return PersistenceElement.byteArray2longInvert(arr, start);
        }
        return PersistenceElement.byteArray2longDirect(arr, start);
    }

    /**
    This method is responsible of taking into account the endianess of the
    original data
    @return single precision float number representation for given bit stream
    on big endian order
    */
    public static byteArray2floatBE(arr: Uint8Array, start: number): number {
        if (PersistenceElement.bigEndianArchitecture) {
            return PersistenceElement.byteArray2longDirect(arr, start);
        }
        return PersistenceElement.byteArray2longInvert(arr, start);
    }

    public static float2byteArrayBE(arr: Uint8Array, start: number, num: number): void {
        const a = Float.floatToIntBits(num);
        if (PersistenceElement.bigEndianArchitecture) {
            PersistenceElement.signedInt2byteArrayDirect(arr, start, a);
        }
        PersistenceElement.signedInt2byteArrayInvert(arr, start, a);
    }

    public static float2byteArrayLE(arr: Uint8Array, start: number, num: number): void {
        const a = Float.floatToIntBits(num);
        if (PersistenceElement.bigEndianArchitecture) {
            PersistenceElement.signedInt2byteArrayInvert(arr, start, a);
        }
        PersistenceElement.signedInt2byteArrayDirect(arr, start, a);
    }

    /**
    This method is responsible of taking into account the endianess of the
    original data
    @return single precision float representation for given bit stream on
    little endian order
    */
    public static byteArray2floatLE(arr: Uint8Array, start: number): number {
        if (PersistenceElement.bigEndianArchitecture) {
            return PersistenceElement.byteArray2floatInvert(arr, start);
        }
        return PersistenceElement.byteArray2floatDirect(arr, start);
    }

    /**
    This method is responsible of taking into account the endianess of the
    original data
    @return double precission float representation for given bit stream on
    little endian order
    */
    public static byteArray2doubleLE(arr: Uint8Array, start: number): number {
        if (PersistenceElement.bigEndianArchitecture) {
            return PersistenceElement.byteArray2doubleInvert(arr, start);
        }
        return PersistenceElement.byteArray2doubleDirect(arr, start);
    }

    /**
    @return double precission float representation for given bit stream on
    bit endian order
    */
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

    /**
    Pending to check. Is this really managing 64 bit long integers?
    */
    public static readLongLE(is: InputStream): number {
        PersistenceElement.readBytes(is, PersistenceElement.bytesForLong);
        return PersistenceElement.byteArray2longLE(PersistenceElement.bytesForLong, 0);
    }

    /**
    Pending to check. Is this really managing 64 bit long integers?
    */
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
        const j = i | 0;
        return Float.intBitsToFloat(j);
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

        // Alternative implementation by Leidy Lozano:
        const bytesForString = new Uint8Array(size);
        PersistenceElement.readBytes(is, bytesForString);

        const msg = new TextDecoder("utf-8").decode(bytesForString);

        const skip = new Uint8Array(1);
        PersistenceElement.readBytes(is, skip);

        return msg;
    }

    public static readAsciiString(is: InputStream): string {
        const character = new Uint8Array(1);
        let letter: string;
        let msg = "";

        do {
            PersistenceElement.readBytes(is, character);
            letter = globalThis.String.fromCharCode(character[0] as number);
            if (character[0] !== 0x00) {
                msg = msg + letter;
            }
        } while (character[0] !== 0x00);

        return msg;
    }

    public static readUtf8String(is: InputStream): string {
        const character = new Uint8Array(1);
        let letter: number;
        let msg = "";
        const a = new Uint8Array(2);

        do {
            PersistenceElement.readBytes(is, character);
            letter = character[0] as number;
            if (character[0] !== 0x00 && letter >> 7 === 0) {
                msg = msg + globalThis.String.fromCharCode(letter);
            } else if (character[0] !== 0x00) {
                a[0] = character[0] as number;
                if (is.available() >= 1) {
                    PersistenceElement.readBytes(is, character);
                    a[1] = character[0] as number;
                    const cc = PersistenceElement.buildUtf8Char(a);
                    if (cc !== null) {
                        msg += cc;
                    } else {
                        console.log(
                            "* UNHANDLED UTF! ********************************************************** ->" + msg,
                        );
                    }
                }
            }
        } while (character[0] !== 0x00);

        return msg;
    }

    public static buildUtf8Char(arr: Uint8Array): string | null {
        let c: string;
        const a = arr[0] as number;
        const b = arr[1] as number;

        if (a >> 5 === 0x06 && b >> 6 === 0x02) {
            c = new TextDecoder("utf-8").decode(arr);
        } else {
            return null;
        }
        return c;
    }

    public static readUtf8Line(is: InputStream): string {
        const character = new Uint8Array(1);
        let letter: number;
        let msg = "";
        const a = new Uint8Array(2);

        do {
            if (is.available() < 1) return "";
            PersistenceElement.readBytes(is, character);
            letter = character[0] as number;
            if (character[0] !== 0x0a && character[0] !== 0x0d && letter >> 7 === 0) {
                msg += globalThis.String.fromCharCode(letter);
            } else if (character[0] !== 0x0a && character[0] !== 0x0d) {
                a[0] = character[0] as number;
                if (is.available() >= 1) {
                    PersistenceElement.readBytes(is, character);
                    a[1] = character[0] as number;
                    const cc = PersistenceElement.buildUtf8Char(a);
                    if (cc !== null) {
                        msg += cc;
                    }
                }
            }
        } while (character[0] !== 0x0a);

        return msg;
    }

    public static readAsciiLine(is: InputStream): string {
        const character = new Uint8Array(1);
        let letter: string;
        let stringBuffer = "";

        do {
            PersistenceElement.readBytes(is, character);
            letter = globalThis.String.fromCharCode(character[0] as number);
            if (character[0] !== 0x0a && character[0] !== 0x0d) {
                stringBuffer += letter;
            }
        } while (character[0] !== 0x0a);

        return stringBuffer;
    }

    private static isInSet(key: number, set: Uint8Array): boolean {
        let i: number;

        for (i = 0; i < set.length; i++) {
            if (key === set[i]) {
                return true;
            }
        }
        return false;
    }

    public static readAsciiToken(is: InputStream, separators: Uint8Array): string {
        const character = new Uint8Array(1);
        let msg = "";

        do {
            PersistenceElement.readBytes(is, character);
            if (!PersistenceElement.isInSet(character[0] as number, separators)) {
                msg = msg + globalThis.String.fromCharCode(character[0] as number);
            }
        } while (!PersistenceElement.isInSet(character[0] as number, separators));

        return msg;
    }

    public static writeAsciiString(writer: OutputStream, cad: string): void {
        const arr = new TextEncoder().encode(cad);
        writer.writeBytes(arr, 0, arr.length);

        const end = new Uint8Array(1);
        end[0] = 0x00;
        writer.writeBytes(end, 0, end.length);
    }

    public static writeUtf8String(writer: OutputStream, cad: string): void {
        const arr = new TextEncoder().encode(cad);
        writer.writeBytes(arr, 0, arr.length);

        const end = new Uint8Array(1);
        end[0] = 0x00;
        writer.writeBytes(end, 0, end.length);
    }

    public static writeAsciiLine(writer: OutputStream, cad: string): void {
        const arr = new TextEncoder().encode(cad);
        writer.writeBytes(arr, 0, arr.length);

        const end = new Uint8Array(1);
        end[0] = 0x0a;
        writer.writeBytes(end, 0, end.length);
    }

    public static writeUtf8Line(writer: OutputStream, cad: string): void {
        const arr = new TextEncoder().encode(cad);
        writer.writeBytes(arr, 0, arr.length);

        const end = new Uint8Array(1);
        end[0] = 0x0a;
        writer.writeBytes(end, 0, end.length);
    }
}
