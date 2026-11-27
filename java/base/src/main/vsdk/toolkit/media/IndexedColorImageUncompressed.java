package vsdk.toolkit.media;
import java.io.Serial;

import vsdk.toolkit.common.VSDK;
import vsdk.toolkit.common.color.ColorRgb;

public class IndexedColorImageUncompressed extends Image
{
    @Serial private static final long serialVersionUID = 20060821L;

    private byte data[];
    private int xSize;
    private int ySize;
    private RGBColorPalette colorTable;

    private ColorRgb staticColor;

    /**
    Check the general signature contract in superclass method
    Image.init.
    If the recieved colorTable is null, this method creates a 256 level
    grayscale palette.
    */
    public IndexedColorImageUncompressed(RGBColorPalette colorTable)
    {
        xSize = 0;
        ySize = 0;
        data = null;
        if ( colorTable != null ) {
            this.colorTable = colorTable;
        }
        else {
            this.colorTable = new RGBColorPalette();
        }
    }

    /**
    Check the general signature contract in superclass method
    Image.init.
    This method creates a 256 level grayscale palette.
    */
    public IndexedColorImageUncompressed()
    {
        xSize = 0;
        ySize = 0;
        data = null;
        this.colorTable = new RGBColorPalette();
    }

    @Override
    public boolean init(int width, int height)
    {
        try {
          data = new byte[width * height];
          for ( int i = 0; i < width*height; i++ ) {
              data[i] = 0;
          }
        }
        catch ( Exception e ) {
          data = null;
          return false;
        }
        xSize = width;
        ySize = height;
        return true;
    }

    @Override
    public boolean initNoFill(int width, int height)
    {
        try {
          data = new byte[width * height];
          for ( int i = 0; i < width*height; i++ ) {
              data[i] = 0;
          }
        }
        catch ( Exception e ) {
          data = null;
          return false;
        }
        xSize = width;
        ySize = height;
        return true;
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

    public byte[] getRawImage()
    {
        return data;
    }

    public void putPixel(int x, int y, byte val)
    {
        int index = xSize*y + x;
        data[index] = val;
    }

    public void putPixel(int x, int y, int val)
    {
        int index = xSize*y + x;
        data[index] = VSDK.unsigned8BitInteger2signedByte(val);
    }

    @Override
    public byte getPixel8bitGrayScale(int x, int y)
    {
        return data[xSize*y + x];
    }

    public int getPixel(int x, int y)
    {
        int index = xSize*y + x;
        return VSDK.signedByte2unsignedInteger(data[index]);
    }

    @Override
    public RGBPixel getPixelRgb(int x, int y)
    {
        int index = xSize*y + x;
        double val;
        val = (double)(VSDK.signedByte2unsignedInteger(data[index])) / 255.0;

        staticColor = colorTable.evalLinear(val);
        RGBPixel p = new RGBPixel();
        p.r = VSDK.unsigned8BitInteger2signedByte((int)(staticColor.r()*255.0));
        p.g = VSDK.unsigned8BitInteger2signedByte((int)(staticColor.g()*255.0));
        p.b = VSDK.unsigned8BitInteger2signedByte((int)(staticColor.b()*255.0));
        return p;
    }

    @Override
    public void getPixelRgb(int x, int y, RGBPixel p)
    {
        int index = xSize*y + x;
        double val;
        val = (double)(VSDK.signedByte2unsignedInteger(data[index])) / 255.0;

        staticColor = colorTable.evalLinear(val);
        p.r = VSDK.unsigned8BitInteger2signedByte((int)(staticColor.r()*255.0));
        p.g = VSDK.unsigned8BitInteger2signedByte((int)(staticColor.g()*255.0));
        p.b = VSDK.unsigned8BitInteger2signedByte((int)(staticColor.b()*255.0));
    }

    public RGBColorPalette getColorTable()
    {
        return colorTable;
    }

    public void setColorTable(RGBColorPalette p)
    {
        colorTable = p;
    }

    @Override
    public void putPixelRgb(int x, int y, RGBPixel p)
    {
        ColorRgb c = new ColorRgb(
            (double)(VSDK.signedByte2unsignedInteger(p.r)) / 255.0,
            (double)(VSDK.signedByte2unsignedInteger(p.g)) / 255.0,
            (double)(VSDK.signedByte2unsignedInteger(p.b)) / 255.0);

        int index = colorTable.selectNearestIndexToRgb(c);
        putPixel(x, y, index);
    }

    public RGBImageUncompressed exportToRgbImage()
    {
        RGBImageUncompressed copy;
        int xxSize = getXSize();
        int yySize = getYSize();
        int x, y;
        int source;
        RGBPixel target = new RGBPixel();

        copy = new RGBImageUncompressed();
        copy.init(xxSize, yySize);
        for ( x = 0; x < xxSize; x++ ) {
            for ( y = 0; y < yySize; y++ ) {
                //source = getPixel(x, y);
                //target.r = target.g = target.b = VSDK.unsigned8BitInteger2signedByte(source);
                getPixelRgb(x, y, target);
                copy.putPixel(x, y, target);
            }
        }
        return copy;
    }

    public RGBAImageUncompressed exportToRgbaImage()
    {
        RGBAImageUncompressed copy;
        int xxSize = getXSize();
        int yySize = getYSize();
        int x, y;
        int source;
        RGBAPixel target = new RGBAPixel();

        copy = new RGBAImageUncompressed();
        copy.init(xxSize, yySize);
        target.a = VSDK.unsigned8BitInteger2signedByte(128);
        for ( x = 0; x < xxSize; x++ ) {
            for ( y = 0; y < yySize; y++ ) {
                source = getPixel(x, y);
                target.r = target.g = target.b = VSDK.unsigned8BitInteger2signedByte(source);
                copy.putPixel(x, y, target);
            }
        }

        return copy;
    }
}
