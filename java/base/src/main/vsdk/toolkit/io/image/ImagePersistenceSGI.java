//= References:                                                             =
//= [wHAEB2006] Haeberli, Paul. "The SGI Image File Format", version 1.00,  =
//=     available at http://local.wasp.uwa.edu.au/~pbourke/dataformats/     =
//=     sgirgb/sgiversion.html, accessed december 19 2006.                  =

package vsdk.toolkit.io.image;

import java.io.FileInputStream;
import java.io.RandomAccessFile;

import vsdk.toolkit.common.VSDK;
import vsdk.toolkit.common.logging.Logger;
import vsdk.toolkit.media.Image;
import vsdk.toolkit.media.RGBImageUncompressed;
import vsdk.toolkit.media.RGBAImageUncompressed;
import vsdk.toolkit.media.IndexedColorImageUncompressed;
import vsdk.toolkit.media.GrayScalePalette;
import vsdk.toolkit.io.PersistenceElement;

/**
This is a factory class intended to create Image entities from binary image
files in SGI format. It is supposed to implement a persistence schema for
the SGI image file format, as described in [wHAEB2006].

\todo  Not all subformats are supported, and no writting operations are
      implemented yet.
*/
public class ImagePersistenceSGI extends PersistenceElement
{
    private static
    void processScanLineCase8bpp(RandomAccessFile fd, FileInputStream is,
                                 long start, long length, 
                                 IndexedColorImageUncompressed img, int y) throws Exception
    {
        int x = 0;
        int pos;
        int i;
        int count;

        fd.seek(start);
        byte buffer[] = new byte[(int)length];
        boolean flag;
        readBytes(is, buffer);

        if ( y >= img.getYSize() ) return;

        for ( pos = 0; pos < length; pos++ ) {
            flag = ((buffer[pos] & 0x80) != 0x00)?true:false;
            //buffer[pos] <<= 1;
            //buffer[pos] >>= 1;
            buffer[pos] = VSDK.unsigned8BitInteger2signedByte(VSDK.signedByte2unsignedInteger(buffer[pos]) & 0x7F);
            count = VSDK.signedByte2unsignedInteger(buffer[pos]);
            if ( flag ) {
                // Copy next count bytes
                for ( i = 0; i < count; i++ ) {
                    pos++;
                    if ( x >= img.getXSize() || pos >= length ) {
                        return;
                    }
                    img.putPixel(x, y, buffer[pos]);
                    x++;
                }
            }
            else {
                // RLE Processing: next byte count times
                pos++;
                for ( i = 0; i < count; i++ ) {
                    if ( x >= img.getXSize() || pos >= length ) {
                        return;
                    }
                    img.putPixel(x, y, buffer[pos]);
                    x++;
                }
            }
        }
    }

    public static
    Image readImageSGI(String filename)
    {
        Image img = null;

        //- Process RGB image file ----------------------------------------
        FileInputStream is;                             // File input helpers
        byte character[] = new byte[1];
        int irisImageFileMagicNumber;               // SGI header data
        int storageFormat;
        int bytesPerPixelChannel;
        int numberOfDimensions;
        int xSize;
        int ySize;
        int numberOfChannels;
        long minimumPixelValue;
        long maximumPixelValue;
        long dummy1;
        byte imageName[] = new byte[80];
        long colormapId;
        byte dummy2[] = new byte[404];

        try {
            RandomAccessFile fd = new RandomAccessFile(filename, "r");
            is = new FileInputStream(fd.getFD());

            //- Process SGI file header ----------------------------------
            irisImageFileMagicNumber = readSignedShortBE(is);
            if ( irisImageFileMagicNumber != 474 ) {
                throw new Exception("Not an SGI image, " +
                    "wrong magic number: " + irisImageFileMagicNumber);
            }
            readBytes(is, character);
            storageFormat = VSDK.signedByte2unsignedInteger(character[0]);
            readBytes(is, character);
            bytesPerPixelChannel = VSDK.signedByte2unsignedInteger(character[0]);
            numberOfDimensions = readSignedShortBE(is);
            xSize = readSignedShortBE(is);
            ySize = readSignedShortBE(is);
            numberOfChannels = readSignedShortBE(is);
            minimumPixelValue = readLongBE(is);
            maximumPixelValue = readLongBE(is);
            dummy1 = readLongBE(is);
            readBytes(is, imageName);
            colormapId = readLongBE(is);
            readBytes(is, dummy2);

            if ( colormapId != 0 ) {
                throw new Exception("Not implemented SGI colormap: " +
                    colormapId);
            }

            switch ( numberOfChannels ) {
              case 1:
                GrayScalePalette p = new GrayScalePalette();
                img = new IndexedColorImageUncompressed(p);
                break;
              case 3:
                img = new RGBImageUncompressed();
                break;
              case 4:
                img = new RGBAImageUncompressed();
                break;
              default:
                throw new Exception("Not supported SGI subformat: " +
                    "unknown number of channels: " + numberOfChannels);
            }
            img.init(xSize, ySize);

            //- Process offset tables (only if it is RLE) ----------------
            int numberOfTables = ySize * numberOfChannels;
            long startsTable[] = new long[numberOfTables];
            long lengthsTable[] = new long[numberOfTables];
            int i;

            fd.seek(512);
            if ( storageFormat == 0x01 ) {
                for ( i = 0; i < numberOfTables; i++ ) {
                    startsTable[i] = readLongBE(is);
                }
                for ( i = 0; i < numberOfTables; i++ ) {
                    lengthsTable[i] = readLongBE(is);
                }
            }
            else {
                throw new Exception("Not implemented SGI storageFormat: " +
                    storageFormat);
            }

            //- Process image data ---------------------------------------
            int y;
            // This works only for grayscale images!
            for ( y = 0; y < ySize; y++ ) {
                processScanLineCase8bpp(fd, is, 
                                        startsTable[y], lengthsTable[y], 
                                        (IndexedColorImageUncompressed)img, ySize-y-1);
            }
            //------------------------------------------------------------
            is.close();
            fd.close();
          }
          catch ( Exception e ) {
            Logger.reportMessage(null, VSDK.ERROR, 
                "ImagePersistenceSGI.readImageSGI",
                               "Error reading image from " + filename + "\n" + e);
        }
        //-----------------------------------------------------------------
        return img;
    }

}
