//= References:                                                             =
//= [wHAEB2006] Haeberli, Paul. "The SGI Image File Format", version 1.00,  =
//=     available at http://local.wasp.uwa.edu.au/~pbourke/dataformats/     =
//=     sgirgb/sgiversion.html, accessed december 19 2006.                  =

import { VSDK } from "../../common/VSDK.js";
import { Logger } from "../../common/logging/Logger.js";
import { GrayScalePalette } from "../../media/GrayScalePalette.js";
import type { Image } from "../../media/Image.js";
import { IndexedColorImageUncompressed } from "../../media/IndexedColorImageUncompressed.js";
import { RGBAImageUncompressed } from "../../media/RGBAImageUncompressed.js";
import { RGBImageUncompressed } from "../../media/RGBImageUncompressed.js";
import { PersistenceElement } from "../PersistenceElement.js";

/**
Port of `vsdk.toolkit.io.image.ImagePersistenceSGI`.

This is a factory class intended to create Image entities from binary image
files in SGI format. It is supposed to implement a persistence schema for
the SGI image file format, as described in [wHAEB2006].

\todo  Not all subformats are supported, and no writting operations are
      implemented yet.

One boundary is crossed, the same one `Md2Persistence` crosses: Java names a
file and opens a `RandomAccessFile` over it, with a `FileInputStream` built
from that same file descriptor, so `fd.seek` and the stream reads share one
file pointer. A browser has no file system, and the resource is fetched whole
before anything is parsed, so what this class takes is the bytes and the cursor
below plays the part of that single shared pointer. Java's own `is.close()`
and `fd.close()` have nothing to close here.

Everything else is the Java reader: the 512-byte header with its magic number
474, the channel count choosing between an indexed-color, an RGB and an RGBA
image, the RLE offset and length tables, the high bit of a run byte selecting
between a literal copy and a repeat, and the `ySize - y - 1` vertical flip. So
are its limits, including the fact that the scan-line loop casts every image to
`IndexedColorImageUncompressed` and therefore only really reads the grayscale
subformat, and the catch-all that logs a failure and answers whatever was built
so far — a `null` when the header itself did not parse.
*/
export class ImagePersistenceSGI extends PersistenceElement {
    private static processScanLineCase8bpp(
        cursor: _SgiByteCursor,
        start: number,
        length: number,
        img: IndexedColorImageUncompressed,
        y: number,
    ): void {
        let x = 0;
        let pos: number;
        let i: number;
        let count: number;

        cursor.seek(start);
        const buffer: Int8Array = new Int8Array(length);
        let flag: boolean;
        cursor.readBytes(buffer);

        if (y >= img.getYSize()) return;

        for (pos = 0; pos < length; pos++) {
            flag = (buffer[pos]! & 0x80) !== 0x00;
            buffer[pos] = VSDK.unsigned8BitInteger2signedByte(VSDK.signedByte2unsignedInteger(buffer[pos]!) & 0x7f);
            count = VSDK.signedByte2unsignedInteger(buffer[pos]!);
            if (flag) {
                // Copy next count bytes
                for (i = 0; i < count; i++) {
                    pos++;
                    if (x >= img.getXSize() || pos >= length) {
                        return;
                    }
                    img.putPixelByte(x, y, buffer[pos]!);
                    x++;
                }
            } else {
                // RLE Processing: next byte count times
                pos++;
                for (i = 0; i < count; i++) {
                    if (x >= img.getXSize() || pos >= length) {
                        return;
                    }
                    img.putPixelByte(x, y, buffer[pos]!);
                    x++;
                }
            }
        }
    }

    public static readImageSGI(fileContent: Uint8Array, sourceName: string): Image | null {
        let img: Image | null = null;

        //- Process RGB image file ----------------------------------------
        const cursor: _SgiByteCursor = new _SgiByteCursor(fileContent);
        const character: Int8Array = new Int8Array(1);
        let irisImageFileMagicNumber: number; // SGI header data
        let storageFormat: number;
        let numberOfChannels: number;
        let xSize: number;
        let ySize: number;
        const imageName: Int8Array = new Int8Array(80);
        let colormapId: number;
        const dummy2: Int8Array = new Int8Array(404);

        try {
            //- Process SGI file header ----------------------------------
            irisImageFileMagicNumber = cursor.readSignedShortBE();
            if (irisImageFileMagicNumber !== 474) {
                throw new Error("Not an SGI image, wrong magic number: " + irisImageFileMagicNumber);
            }
            cursor.readBytes(character);
            storageFormat = VSDK.signedByte2unsignedInteger(character[0]!);
            cursor.readBytes(character);
            // `bytesPerPixelChannel`, read and unused by the Java reader.
            cursor.readSignedShortBE();
            // `numberOfDimensions`, likewise read and unused.
            xSize = cursor.readSignedShortBE();
            ySize = cursor.readSignedShortBE();
            numberOfChannels = cursor.readSignedShortBE();
            // `minimumPixelValue`, `maximumPixelValue` and `dummy1`.
            cursor.readLongBE();
            cursor.readLongBE();
            cursor.readLongBE();
            cursor.readBytes(imageName);
            colormapId = cursor.readLongBE();
            cursor.readBytes(dummy2);

            if (colormapId !== 0) {
                throw new Error("Not implemented SGI colormap: " + colormapId);
            }

            switch (numberOfChannels) {
                case 1: {
                    const p: GrayScalePalette = new GrayScalePalette();
                    img = new IndexedColorImageUncompressed(p);
                    break;
                }
                case 3:
                    img = new RGBImageUncompressed();
                    break;
                case 4:
                    img = new RGBAImageUncompressed();
                    break;
                default:
                    throw new Error("Not supported SGI subformat: unknown number of channels: " + numberOfChannels);
            }
            img.init(xSize, ySize);

            //- Process offset tables (only if it is RLE) ----------------
            const numberOfTables: number = ySize * numberOfChannels;
            const startsTable: Float64Array = new Float64Array(numberOfTables);
            const lengthsTable: Float64Array = new Float64Array(numberOfTables);
            let i: number;

            cursor.seek(512);
            if (storageFormat === 0x01) {
                for (i = 0; i < numberOfTables; i++) {
                    startsTable[i] = cursor.readLongBE();
                }
                for (i = 0; i < numberOfTables; i++) {
                    lengthsTable[i] = cursor.readLongBE();
                }
            } else {
                throw new Error("Not implemented SGI storageFormat: " + storageFormat);
            }

            //- Process image data ---------------------------------------
            let y: number;
            // This works only for grayscale images!
            for (y = 0; y < ySize; y++) {
                ImagePersistenceSGI.processScanLineCase8bpp(
                    cursor,
                    startsTable[y]!,
                    lengthsTable[y]!,
                    img as IndexedColorImageUncompressed,
                    ySize - y - 1,
                );
            }
            //------------------------------------------------------------
        } catch (e) {
            Logger.reportMessage(
                null,
                VSDK.ERROR,
                "ImagePersistenceSGI.readImageSGI",
                "Error reading image from " + sourceName + "\n" + String(e),
            );
        }
        //-----------------------------------------------------------------
        return img;
    }
}

/**
The single file pointer Java's `RandomAccessFile` and its `FileInputStream`
share, over the bytes of the resource. Exported for the reader's own tests;
it is not part of the Java API.
*/
export class _SgiByteCursor {
    private position = 0;

    public constructor(private readonly bytes: Uint8Array) {}

    public seek(position: number): void {
        this.position = position;
    }

    /**
    Java's `PersistenceElement.readBytes(is, buffer)`, which fills the whole
    array or throws. The buffer is an `Int8Array` because Java's is a `byte[]`,
    and the RLE decoder tests the sign bit of what it holds.
    */
    public readBytes(buffer: Int8Array): void {
        if (this.position + buffer.length > this.bytes.length) {
            throw new Error("Unexpected end of SGI image data");
        }
        let i: number;
        for (i = 0; i < buffer.length; i++) {
            buffer[i] = VSDK.unsigned8BitInteger2signedByte(this.bytes[this.position + i]!);
        }
        this.position += buffer.length;
    }

    public readSignedShortBE(): number {
        const value: number = PersistenceElement.byteArray2signedShortBE(this.bytes, this.position);
        this.position += 2;
        return value;
    }

    public readLongBE(): number {
        const value: number = PersistenceElement.byteArray2longBE(this.bytes, this.position);
        this.position += 4;
        return value;
    }
}
