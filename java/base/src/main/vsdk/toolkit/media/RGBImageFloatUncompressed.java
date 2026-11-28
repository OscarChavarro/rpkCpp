package vsdk.toolkit.media;

import java.io.Serial;

import vsdk.toolkit.common.color.ColorRgb;

/**
Current class is a specific low level implementation of an uncompressed high
dynamic range RGB image: each pixel keeps its red, green and blue components
as unbounded double precision floating point values (e.g. radiance values
computed by a global illumination renderer), ordered in a sequential array
of RGB triplets, row by row from the upper left pixel, and left to right on
each row.

As an `Image`, the RGBPixel based methods map the range [0, 1] of the
components to the byte range [0, 255]: values outside [0, 1] are clamped
when reading, so this image can be given to the 8 bit exporters (PPM, PNG,
JPEG) of `ImagePersistence`. The full range of the components is kept by
`putPixel` / `getPixel` and by the high dynamic range exporters (RGBE).
*/
public class RGBImageFloatUncompressed extends Image
{
    @SuppressWarnings("FieldNameHidesFieldInSuperclass")
    @Serial private static final long serialVersionUID = 20261128L;

    private static final int COMPONENTS_PER_PIXEL = 3;

    private double[] data;
    private int xSize;
    private int ySize;

    public RGBImageFloatUncompressed()
    {
        data = null;
        xSize = 0;
        ySize = 0;
    }

    public int getSizeInBytes()
    {
        return xSize * ySize * COMPONENTS_PER_PIXEL * 8
            + 2 * INT_SIZE_IN_BYTES + POINTER_SIZE_IN_BYTES;
    }

    /**
    Allocates the image with all of its components set to zero
    */
    @Override
    public boolean init(int width, int height)
    {
        if ( !initNoFill(width, height) ) {
            return false;
        }
        for ( int i = 0; i < data.length; i++ ) {
            data[i] = 0.0;
        }
        return true;
    }

    @Override
    public boolean initNoFill(int width, int height)
    {
        if ( width <= 0 || height <= 0 ) {
            return false;
        }
        if ( data != null && width == xSize && height == ySize ) {
            return true;
        }

        try {
            data = new double[width * height * COMPONENTS_PER_PIXEL];
        }
        catch ( OutOfMemoryError e ) {
            data = null;
            xSize = 0;
            ySize = 0;
            return false;
        }
        xSize = width;
        ySize = height;
        return true;
    }

    private int pixelBaseIndex(int x, int y)
    {
        return (y * xSize + x) * COMPONENTS_PER_PIXEL;
    }

    private static byte componentToByte(double value)
    {
        if ( !(value > 0.0) ) {
            return 0;
        }
        if ( value >= 1.0 ) {
            return (byte)255;
        }
        return (byte)(int)(value * 255.0);
    }

    public void putPixel(int x, int y, double r, double g, double b)
    {
        int index = pixelBaseIndex(x, y);
        data[index] = r;
        data[index + 1] = g;
        data[index + 2] = b;
    }

    public void putPixel(int x, int y, ColorRgb color)
    {
        putPixel(x, y, color.getR(), color.getG(), color.getB());
    }

    public ColorRgb getPixel(int x, int y)
    {
        int index = pixelBaseIndex(x, y);
        return new ColorRgb(data[index], data[index + 1], data[index + 2]);
    }

    @Override
    public void putPixelRgb(int x, int y, RGBPixel p)
    {
        putPixel(
            x,
            y,
            (double)(p.r & 0xFF) / 255.0,
            (double)(p.g & 0xFF) / 255.0,
            (double)(p.b & 0xFF) / 255.0);
    }

    @Override
    public RGBPixel getPixelRgb(int x, int y)
    {
        RGBPixel p = new RGBPixel();
        getPixelRgb(x, y, p);
        return p;
    }

    @Override
    public void getPixelRgb(int x, int y, RGBPixel p)
    {
        int index = pixelBaseIndex(x, y);
        p.r = componentToByte(data[index]);
        p.g = componentToByte(data[index + 1]);
        p.b = componentToByte(data[index + 2]);
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
}
