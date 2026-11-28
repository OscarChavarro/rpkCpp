package vsdk.toolkit.media;
import java.io.Serial;
import java.nio.ByteBuffer;
import java.util.Arrays;

import vsdk.toolkit.common.VSDK;
import vsdk.toolkit.common.logging.Logger;

/**
Compressed RGBA image storage. Pixel-level access is intentionally unsupported
because the contents remain in GPU-ready compressed blocks.
*/
public class RGBAImageCompressed extends Image implements Cloneable
{
    @SuppressWarnings("FieldNameHidesFieldInSuperclass")
    @Serial private static final long serialVersionUID = 20260430L;

    public static final int COMPRESSION_UNKNOWN = 0;
    public static final int COMPRESSION_DXT1 = 1;
    public static final int COMPRESSION_DXT3 = 3;
    public static final int COMPRESSION_DXT5 = 5;

    private byte data[];
    transient private ByteBuffer directData;
    private int xSize;
    private int ySize;
    private int compressionFormat;
    private int compressedDataSize;

    public RGBAImageCompressed()
    {
        data = null;
        directData = null;
        xSize = 0;
        ySize = 0;
        compressionFormat = COMPRESSION_UNKNOWN;
        compressedDataSize = 0;
    }

    public void dettach()
    {
        data = null;
        directData = null;
    }

    @Override
    public int getSizeInBytes()
    {
        int dataSize = data == null ? 0 : data.length;
        return dataSize + 4*INT_SIZE_IN_BYTES + POINTER_SIZE_IN_BYTES;
    }

    @Override
    public boolean init(int width, int height)
    {
        xSize = width;
        ySize = height;
        compressionFormat = COMPRESSION_UNKNOWN;
        compressedDataSize = 0;
        data = new byte[0];
        directData = ByteBuffer.allocateDirect(0);
        return true;
    }

    @Override
    public boolean initNoFill(int width, int height)
    {
        return init(width, height);
    }

    public boolean initCompressed(
        int width,
        int height,
        int compressionFormat,
        byte compressedData[])
    {
        if ( width <= 0 || height <= 0 || compressedData == null ) {
            data = null;
            directData = null;
            xSize = 0;
            ySize = 0;
            this.compressionFormat = COMPRESSION_UNKNOWN;
            compressedDataSize = 0;
            return false;
        }

        xSize = width;
        ySize = height;
        this.compressionFormat = compressionFormat;
        compressedDataSize = calculateTopLevelDataSize(width, height, compressionFormat);
        if ( compressedData.length < compressedDataSize ) {
            data = null;
            directData = null;
            xSize = 0;
            ySize = 0;
            this.compressionFormat = COMPRESSION_UNKNOWN;
            compressedDataSize = 0;
            return false;
        }
        data = Arrays.copyOf(compressedData, compressedData.length);
        directData = ByteBuffer.allocateDirect(data.length);
        directData.put(data);
        directData.rewind();
        return true;
    }

    public void setRawImage(
        int width,
        int height,
        int compressionFormat,
        byte compressedData[])
    {
        initCompressed(width, height, compressionFormat, compressedData);
    }

    public int getCompressionFormat()
    {
        return compressionFormat;
    }

    public int getCompressedDataSize()
    {
        return compressedDataSize;
    }

    public byte[] getRawImage()
    {
        return data;
    }

    public ByteBuffer getRawImageDirectBuffer()
    {
        directData.rewind();
        return directData;
    }

    @Override
    public int getXSize()
    {
        return xSize;
    }

    @Override
    public int getYSize()
    {
        return ySize;
    }

    public void putPixel(int x, int y, byte r, byte g, byte b)
    {
        reportUnsupportedPixelAccess("putPixel");
    }

    public void putPixel(int x, int y, byte r, byte g, byte b, byte a)
    {
        reportUnsupportedPixelAccess("putPixel");
    }

    public void putPixel(int x, int y, RGBAPixel p)
    {
        reportUnsupportedPixelAccess("putPixel");
    }

    @Override
    public void putPixelRgb(int x, int y, RGBPixel p)
    {
        reportUnsupportedPixelAccess("putPixelRgb");
    }

    public RGBAPixel getPixel(int x, int y)
    {
        reportUnsupportedPixelAccess("getPixel");
        return new RGBAPixel();
    }

    @Override
    public RGBPixel getPixelRgb(int x, int y)
    {
        reportUnsupportedPixelAccess("getPixelRgb");
        return new RGBPixel();
    }

    public void getPixelRgba(int x, int y, RGBAPixel p)
    {
        reportUnsupportedPixelAccess("getPixelRgba");
    }

    @Override
    public void getPixelRgb(int x, int y, RGBPixel p)
    {
        reportUnsupportedPixelAccess("getPixelRgb");
    }

    @Override
    public RGBAImageCompressed clone() throws CloneNotSupportedException
    {
        super.clone();
        RGBAImageCompressed copy = new RGBAImageCompressed();
        copy.initCompressed(xSize, ySize, compressionFormat, data);
        return copy;
    }

    public static int calculateTopLevelDataSize(
        int width,
        int height,
        int compressionFormat)
    {
        int blockSize;

        if ( compressionFormat == COMPRESSION_DXT1 ) {
            blockSize = 8;
        }
        else if ( compressionFormat == COMPRESSION_DXT3 ||
                  compressionFormat == COMPRESSION_DXT5 ) {
            blockSize = 16;
        }
        else {
            return 0;
        }

        int blockWidth = Math.max(1, (width + 3) / 4);
        int blockHeight = Math.max(1, (height + 3) / 4);
        return blockWidth * blockHeight * blockSize;
    }

    private void reportUnsupportedPixelAccess(String method)
    {
        Logger.reportMessage(
            this,
            VSDK.FATAL_ERROR,
            method,
            "Pixel access is not implemented for RGBAImageCompressed.");
    }
}
