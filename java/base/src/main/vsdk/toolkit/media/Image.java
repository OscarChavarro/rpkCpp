//= References:                                                             =
//= [BLIN1978b] Blinn, James F. "Simulation of wrinkled surfaces", SIGGRAPH =
//=          proceedings, 1978.                                             =

package vsdk.toolkit.media;
import java.io.Serial;

import vsdk.toolkit.common.color.ColorRgb;
import vsdk.toolkit.common.VSDK;

/**
This abstract class establishes the required interface for all Image classes
in the VSDK toolkit, and provides some common utilities for nearest and
bi-linear interpolation evaluation on Rgb space.
*/

public abstract class Image extends MediaEntity
{
    @SuppressWarnings("FieldNameHidesFieldInSuperclass")
    @Serial private static final long serialVersionUID = 20150218L;

    /**
    Given the width and height of the desired new size for this image, this
    method is responsable of allocating the necesary memory to keep such
    an image.
    @param width - desired new width in pixels for the image. Must be greater
    than 0.
    @param height - desired new height in pixels for the image. Must be greater
    than 0.
    @return true if image memory could be allocated, false otherwise
    */
    public abstract boolean init(int width, int height);

    /**
    Given the width and height of the desired new size for this image, this
    method is responsable of allocating the necesary memory to keep such
    an image.
    @param width - desired new width in pixels for the image. Must be greater
    than 0.
    @param height - desired new height in pixels for the image. Must be greater
    than 0.
    @return true if image memory could be allocated, false otherwise
    */
    public abstract boolean initNoFill(int width, int height);

    /**
    Returns current image width in pixels
    @return current image width in pixels
    */
    public abstract int getXSize();

    /**
    Returns current image height in pixels
    @return current image height in pixels
    */
    public abstract int getYSize();

    /**
    Given an image position inside its current boundaries and an RGBPixel,
    this method convert the pixel RGB value to its internal colorspace,
    and updates corresponding internal pixel value.
    @param x - x cooordinate of desired pixel, must be between 0 and image
    width minus 1
    @param y - y cooordinate of desired pixel, must be between 0 and image
    height minus 1
    @param p
    */
    public abstract void putPixelRgb(int x, int y, RGBPixel p);

    /**
    Given an image position inside its current boundaries, an RGBPixel is
    returned from internal color space representation.
    @param x - x cooordinate of desired pixel, must be between 0 and image
    width minus 1
    @param y - y cooordinate of desired pixel, must be between 0 and image
    height minus 1
    @return the RGBPixel corresponding to requested pixel coordinate
    inside the image.
    */
    public abstract RGBPixel getPixelRgb(int x, int y);

    /**
    Given an image position inside its current boundaries, an RGBPixel is
    returned from internal color space representation.
    @param x - x cooordinate of desired pixel, must be between 0 and image
    width minus 1
    @param y - y cooordinate of desired pixel, must be between 0 and image
    height minus 1
    Modifies the RGBPixel corresponding to requested pixel coordinate
    inside the image.
    @param p
    */
    public abstract void getPixelRgb(int x, int y, RGBPixel p);

    /**
    Given an image position inside its current boundaries, an RGBPixel is
    returned from internal color space representation.
    @param x - x cooordinate of desired pixel, must be between 0 and image
    width minus 1
    @param y - y cooordinate of desired pixel, must be between 0 and image
    height minus 1
    @return the RGBPixel corresponding to requested pixel coordinate
    inside the image.
    */
    public byte getPixel8bitGrayScale(int x, int y)
    {
        RGBPixel rgb = new RGBPixel();
        getPixelRgb(x, y, rgb);

        int sum = VSDK.signedByte2unsignedInteger(rgb.r) + VSDK.signedByte2unsignedInteger(rgb.g) + VSDK.signedByte2unsignedInteger(rgb.b);
        return VSDK.unsigned8BitInteger2signedByte(sum / 3);
    }

    /**
    Given a double value inside the integer limits of this image, this
    method returns a rgb color corresponding to the nearest getPixelRgb.

    \todo : implement this method.
    @param x
    @param y
    @return 
    */
    public ColorRgb getColorRgbNearest(double x, double y)
    {
        double u = x - Math.floor(x);
        double v = y - Math.floor(y);

        int i = (int)Math.floor(u * ((double)(getXSize()-1)));
        int j = (int)Math.floor(v * ((double)(getYSize()-1)));

        RGBPixel p = getPixelRgb(i, j);
        return new ColorRgb(
            ((double)VSDK.signedByte2unsignedInteger(p.r)) / 255.0,
            ((double)VSDK.signedByte2unsignedInteger(p.g)) / 255.0,
            ((double)VSDK.signedByte2unsignedInteger(p.b)) / 255.0);
    }

    /**
    Given a double value inside the integer limits of this image, this
    method returns a rgb color corresponding to a bi-linear interpolation
    of the 4 neighboring pixels of the float position.

    Current implementation is based on bilinear interpolation algorithm
    proposed for the bumpmap equivalent in [BLIN1978b].
    @param x
    @param y
    @return 
    */
    public ColorRgb getColorRgbBiLinear(double x, double y)
    {
        //-----------------------------------------------------------------
        double u = x - Math.floor(x);
        double v = y - Math.floor(y);
        int width = getXSize();
        int height = getYSize();
        double U = u * ((double)width);
        double V = v * ((double)height);
        int i0 = positiveMod((int)Math.floor(U), width);
        int j0 = positiveMod((int)Math.floor(V), height);
        int i1 = positiveMod(i0 + 1, width);
        int j1 = positiveMod(j0 + 1, height);
        double du = U - (double)i0;
        double dv = V - (double)j0;
        RGBPixel p;

        //-----------------------------------------------------------------
        p = getPixelRgb(i0, j0);
        ColorRgb F00 = new ColorRgb(
            ((double)VSDK.signedByte2unsignedInteger(p.r)) / 255.0,
            ((double)VSDK.signedByte2unsignedInteger(p.g)) / 255.0,
            ((double)VSDK.signedByte2unsignedInteger(p.b)) / 255.0);

        p = getPixelRgb(i1, j0);
        ColorRgb F10 = new ColorRgb(
            ((double)VSDK.signedByte2unsignedInteger(p.r)) / 255.0,
            ((double)VSDK.signedByte2unsignedInteger(p.g)) / 255.0,
            ((double)VSDK.signedByte2unsignedInteger(p.b)) / 255.0);

        p = getPixelRgb(i0, j1);
        ColorRgb F01 = new ColorRgb(
            ((double)VSDK.signedByte2unsignedInteger(p.r)) / 255.0,
            ((double)VSDK.signedByte2unsignedInteger(p.g)) / 255.0,
            ((double)VSDK.signedByte2unsignedInteger(p.b)) / 255.0);

        p = getPixelRgb(i1, j1);
        ColorRgb F11 = new ColorRgb(
            ((double)VSDK.signedByte2unsignedInteger(p.r)) / 255.0,
            ((double)VSDK.signedByte2unsignedInteger(p.g)) / 255.0,
            ((double)VSDK.signedByte2unsignedInteger(p.b)) / 255.0);

        //-----------------------------------------------------------------
        ColorRgb FU0 = new ColorRgb(
            F00.r() + du * (F10.r()-F00.r()),
            F00.g() + du * (F10.g()-F00.g()),
            F00.b() + du * (F10.b()-F00.b()));

        ColorRgb FU1 = new ColorRgb(
            F01.r() + du * (F11.r()-F01.r()),
            F01.g() + du * (F11.g()-F01.g()),
            F01.b() + du * (F11.b()-F01.b()));

        return new ColorRgb(
            FU0.r() + dv * (FU1.r()-FU0.r()),
            FU0.g() + dv * (FU1.g()-FU0.g()),
            FU0.b() + dv * (FU1.b()-FU0.b()));
    }

    private static int positiveMod(int value, int modulus)
    {
        int result = value % modulus;
        if ( result < 0 ) {
            result += modulus;
        }
        return result;
    }

    /**
    This method creates a checker board like visual test pattern with centered
    and colored crossing lines. This is provided as a quick image builder in
    RGB space to test image algorithm.
    */
    public void createTestPattern()
    {
        int i;
        int j;
        RGBPixel p = new RGBPixel();

        for ( i = 0; i < getXSize(); i++ ) {
            for ( j = 0; j < getYSize(); j++ ) {
                if ( ((i % 2 != 0) && (j % 2 == 0)) || 
                     ((j % 2 != 0) && (i % 2 == 0)) ) {
                    p.r = (byte)255;
                    p.g = (byte)255;
                    p.b = (byte)255;
                  }
                  else {
                    p.r = 0;
                    p.g = 0;
                    p.b = 0;
                }
                if ( j == getYSize()/2 ) {
                    p.r = (byte)255;
                    p.g = 0;
                    p.b = 0;
                }
                if ( i == getXSize()/2) {
                    p.r = 0;
                    p.g = (byte)255;
                    p.b = 0;
                }
                putPixelRgb(i, j, p);
            }
        }
    }

    /**
    Just to do not the inheritance chain.
    @return
    @throws CloneNotSupportedException 
    */
    @Override
    public Object clone() throws CloneNotSupportedException
    {
        super.clone();
        return null;
    }

}
