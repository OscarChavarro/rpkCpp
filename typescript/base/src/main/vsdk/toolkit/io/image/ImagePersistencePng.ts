import { OutputStream } from "../../../../java/io/OutputStream.js";
import { VSDK } from "../../common/VSDK.js";
import { Image } from "../../media/Image.js";
import { RGBPixel } from "../../media/RGBPixel.js";
import { ImagePersistenceHelper } from "./ImagePersistenceHelper.js";

/**
A compressor for the PNG `IDAT` payload: it receives the raw, already filtered
scanline bytes and answers a complete zlib (RFC 1950) stream.
*/
export type ImageDeflater = (rawBytes: Uint8Array) => Uint8Array;

/**
Platform-neutral, in-memory PNG (RFC 2083) writer for 24-bit RGB images.

This class has no Java counterpart. Java delegates PNG encoding to
`javax.imageio` through `ImagePersistenceAwt`, and the C++ port delegates to
`libpng`; neither of those is available to a browser sandbox. Since a web
frontend must still be able to compute a PNG in memory and ship the bytes to a
remote service, the encoder itself lives here, in the platform-neutral package,
and only the file-system binding is left to `@vitral/fs`.

The `IDAT` payload is produced by a replaceable {@link ImageDeflater}. The
built-in default is dependency-free and emits stored (uncompressed) DEFLATE
blocks, which every PNG decoder accepts; a runtime that owns a real compressor
installs it with {@link setDeflater} (`@vitral/fs` installs `node:zlib`), and
the resulting file is then byte-for-byte a normally compressed PNG.
*/
export class ImagePersistencePng extends ImagePersistenceHelper {
    private static readonly SIGNATURE = new Uint8Array([137, 80, 78, 71, 13, 10, 26, 10]);
    private static crcTable: Uint32Array | null = null;
    private static deflater: ImageDeflater = ImagePersistencePng.deflateStored;

    /**
    Installs the compressor used for the `IDAT` payload. Pass `null` to go back
    to the dependency-free stored-blocks default.
    */
    public static setDeflater(deflater: ImageDeflater | null): void {
        ImagePersistencePng.deflater = deflater === null ? ImagePersistencePng.deflateStored : deflater;
    }

    /**
    Encodes the given image as a complete 24-bit RGB PNG file, in memory.
    */
    public static encodeRGB24(img: Image): Uint8Array {
        const width = img.getXSize();
        const height = img.getYSize();
        const header = new Uint8Array(13);
        const headerView = new DataView(header.buffer);

        headerView.setUint32(0, width);
        headerView.setUint32(4, height);
        header[8] = 8; // Bit depth
        header[9] = 2; // Color type: truecolor RGB
        header[10] = 0; // Compression method: deflate
        header[11] = 0; // Filter method: adaptive
        header[12] = 0; // Interlace method: none

        const raw = new Uint8Array(height * (1 + width * 3));
        const pixel = new RGBPixel();
        let offset = 0;
        let x: number;
        let y: number;

        for (y = 0; y < height; y++) {
            raw[offset++] = 0; // Per-scanline filter type: none
            for (x = 0; x < width; x++) {
                img.getPixelRgb(x, y, pixel);
                raw[offset++] = VSDK.signedByte2unsignedInteger(pixel.r);
                raw[offset++] = VSDK.signedByte2unsignedInteger(pixel.g);
                raw[offset++] = VSDK.signedByte2unsignedInteger(pixel.b);
            }
        }

        const chunks: Uint8Array[] = [
            ImagePersistencePng.SIGNATURE,
            ImagePersistencePng.buildChunk("IHDR", header),
            ImagePersistencePng.buildChunk("IDAT", ImagePersistencePng.deflater(raw)),
            ImagePersistencePng.buildChunk("IEND", new Uint8Array(0)),
        ];

        let total = 0;
        for (const chunk of chunks) {
            total += chunk.length;
        }

        const png = new Uint8Array(total);
        offset = 0;
        for (const chunk of chunks) {
            png.set(chunk, offset);
            offset += chunk.length;
        }
        return png;
    }

    private static buildChunk(type: string, data: Uint8Array): Uint8Array {
        const chunk = new Uint8Array(12 + data.length);
        const view = new DataView(chunk.buffer);

        view.setUint32(0, data.length);
        for (let i = 0; i < 4; i++) {
            chunk[4 + i] = type.charCodeAt(i);
        }
        chunk.set(data, 8);
        view.setUint32(8 + data.length, ImagePersistencePng.crc32(chunk.subarray(4, 8 + data.length)));
        return chunk;
    }

    private static crc32(bytes: Uint8Array): number {
        if (ImagePersistencePng.crcTable === null) {
            const table = new Uint32Array(256);
            for (let n = 0; n < 256; n++) {
                let c = n;
                for (let k = 0; k < 8; k++) {
                    c = (c & 1) !== 0 ? 0xedb88320 ^ (c >>> 1) : c >>> 1;
                }
                table[n] = c >>> 0;
            }
            ImagePersistencePng.crcTable = table;
        }

        const table = ImagePersistencePng.crcTable;
        let crc = 0xffffffff;
        for (let i = 0; i < bytes.length; i++) {
            crc = table[(crc ^ bytes[i]!) & 0xff]! ^ (crc >>> 8);
        }
        return (crc ^ 0xffffffff) >>> 0;
    }

    private static adler32(bytes: Uint8Array): number {
        let a = 1;
        let b = 0;
        for (let i = 0; i < bytes.length; i++) {
            a = (a + bytes[i]!) % 65521;
            b = (b + a) % 65521;
        }
        return ((b << 16) | a) >>> 0;
    }

    /**
    Dependency-free zlib stream made of stored (uncompressed) DEFLATE blocks.
    */
    private static deflateStored(rawBytes: Uint8Array): Uint8Array {
        const maximumBlockSize = 65535;
        const blockCount = Math.max(1, Math.ceil(rawBytes.length / maximumBlockSize));
        const stream = new Uint8Array(2 + blockCount * 5 + rawBytes.length + 4);
        let offset = 0;

        stream[offset++] = 0x78; // CM = deflate, CINFO = 32K window
        stream[offset++] = 0x01; // No preset dictionary, fastest level, valid FCHECK

        for (let block = 0; block < blockCount; block++) {
            const start = block * maximumBlockSize;
            const length = Math.min(maximumBlockSize, rawBytes.length - start);
            stream[offset++] = block === blockCount - 1 ? 1 : 0; // BFINAL, BTYPE = stored
            stream[offset++] = length & 0xff;
            stream[offset++] = (length >>> 8) & 0xff;
            stream[offset++] = ~length & 0xff;
            stream[offset++] = (~length >>> 8) & 0xff;
            stream.set(rawBytes.subarray(start, start + length), offset);
            offset += length;
        }

        const checksum = ImagePersistencePng.adler32(rawBytes);
        stream[offset++] = (checksum >>> 24) & 0xff;
        stream[offset++] = (checksum >>> 16) & 0xff;
        stream[offset++] = (checksum >>> 8) & 0xff;
        stream[offset++] = checksum & 0xff;

        return stream;
    }

    public override pngExportSupported(): boolean {
        return true;
    }

    public override exportPNG(os: OutputStream, img: Image): void {
        this.exportPNG_24bitRgb(os, img);
    }

    public override exportPNG_24bitRgb(os: OutputStream, img: Image): void {
        os.writeBytes(ImagePersistencePng.encodeRGB24(img));
    }
}
