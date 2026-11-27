//= References:                                                             =
//= [BLIN1978b] Blinn, James F. "Simulation of wrinkled surfaces", SIGGRAPH =
//=          proceedings, 1978.                                             =

package vsdk.toolkit.media;
import java.io.Serial;

import java.util.ArrayList;

import vsdk.toolkit.common.VSDK;
import vsdk.toolkit.common.linealAlgebra.Vector3Dd;

public class NormalMap extends MediaEntity
{
    @Serial private static final long serialVersionUID = 20061220L;

    private int xSize;
    private int ySize;
    private ArrayList<Vector3Dd> data;
    // Scale used when converting a bump height map into derivative-like normals.
    // Stored to allow later reconstruction of Fu/Fv-style terms from the sampled
    // vector field.
    private Vector3Dd bumpMapScale;

    public NormalMap()
    {
        xSize = 0;
        ySize = 0;
        data = null;
        bumpMapScale = new Vector3Dd(1, 1, 1);
    }

    public boolean init(int width, int height)
    {
        try {
            data = new ArrayList<Vector3Dd>();
            for ( int i = 0; i < width*height; i++ ) {
                data.add(new Vector3Dd());
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

    public int getXSize()
    {
        return xSize;
    }

    public int getYSize()
    {
        return ySize;
    }

    public Vector3Dd getBumpMapScale()
    {
        return Vector3Dd.copyOf(bumpMapScale);
    }

    public void putNormal(int i, int j, Vector3Dd n)
    {
        if ( i < 0 || j < 0 || i >= xSize || j >= ySize ) return;
        int index = j * xSize + i;
        data.set(index, Vector3Dd.copyOf(n));
    }

    public Vector3Dd getNormal(int u, int v)
    {
        if ( u < 0 || v < 0 || u >= xSize || v >= ySize ) return null;
        int index = v * xSize + u;
        return Vector3Dd.copyOf(data.get(index));
    }

    /**
    Provide a bilinear interpolation scheme as proposed in [BLIN1978b].
    */
    public Vector3Dd getNormal(double x, double y)
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

        //-----------------------------------------------------------------
        Vector3Dd F00, F10, F01, F11, FU0, FU1, FVAL;

        F00 = getNormal(i0, j0);
        F01 = getNormal(i0, j1);
        F10 = getNormal(i1, j0);
        F11 = getNormal(i1, j1);

        FU0 = F00.add(F10.subtract(F00).multiply(du));
        FU1 = F01.add(F11.subtract(F01).multiply(du));
        FVAL = FU0.add(FU1.subtract(FU0).multiply(dv));
        return FVAL;
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
    Warning: This method converts double values to signed bytes. However,
    it is interesting to note that java language aparently makes a weird
    conversion from double to int when casting directly (sometimes
    produces negative integers from positive floats). Note that this
    method checkes this explicity.  Is there a better/clearer way to
    assure the correctness of this conversion?
    */
    public RGBImageUncompressed exportToRgbImage()
    {
        RGBImageUncompressed output = new RGBImageUncompressed();

        if ( !output.init(xSize, ySize) ) {
            return null;
        }

        int x, y;
        Vector3Dd n;
        byte r, g, b;
        int rr, gg, bb;

        for ( y = 0; y < ySize; y++ ) {
            for ( x = 0; x < xSize; x++ ) {
                n = getNormal(x, y);
                n = n.normalized();
                Vector3Dd mapped = new Vector3Dd((n.x()+1)/2, (n.y()+1)/2, (n.z()+1)/2);

                rr = (int)(mapped.x() * 255.0);
                gg = (int)(mapped.y() * 255.0);
                bb = (int)(mapped.z() * 255.0);
                if ( rr < 0 ) rr += 256;
                if ( gg < 0 ) gg += 256;
                if ( bb < 0 ) bb += 256;
                r = VSDK.unsigned8BitInteger2signedByte(rr);
                g = VSDK.unsigned8BitInteger2signedByte(gg);
                b = VSDK.unsigned8BitInteger2signedByte(bb);

                output.putPixel(x, y, r, g, b);
            }
        }
        return output;
    }

    /**
    Warning: This method converts double values to signed bytes. However,
    it is interesting to note that java language aparently makes a weird
    conversion from double to int when casting directly (sometimes
    produces negative integers from positive floats). Note that this
    method checkes this explicity.  Is there a better/clearer way to
    assure the correctness of this conversion?
    */
    public RGBAImageUncompressed exportToRgbaImage()
    {
        RGBAImageUncompressed output = new RGBAImageUncompressed();

        if ( !output.init(xSize, ySize) ) {
            return null;
        }

        int x, y;
        Vector3Dd n;
        byte r, g, b, a;
        int rr, gg, bb;

        a = VSDK.unsigned8BitInteger2signedByte(255);

        for ( y = 0; y < ySize; y++ ) {
            for ( x = 0; x < xSize; x++ ) {
                n = getNormal(x, y);
                n = n.normalized();
                Vector3Dd mapped = new Vector3Dd((n.x()+1)/2, (n.y()+1)/2, (n.z()+1)/2);

                rr = (int)(mapped.x() * 255.0);
                gg = (int)(mapped.y() * 255.0);
                bb = (int)(mapped.z() * 255.0);
                if ( rr < 0 ) rr += 256;
                if ( gg < 0 ) gg += 256;
                if ( bb < 0 ) bb += 256;
                r = VSDK.unsigned8BitInteger2signedByte(rr);
                g = VSDK.unsigned8BitInteger2signedByte(gg);
                b = VSDK.unsigned8BitInteger2signedByte(bb);

                output.putPixel(x, y, r, g, b, a);
            }
        }
        return output;
    }

    /**
    Similar to exportToRgbImage, but each pixel is equivalent to a magnitude
    of displacement from <0, 0, 1> normal
    */
    public RGBImageUncompressed exportToRgbImageGradient()
    {
        RGBImageUncompressed output = new RGBImageUncompressed();

        if ( !output.init(xSize, ySize) ) {
            return null;
        }

        int x, y;
        Vector3Dd n;
        int val;
        byte col;
        Vector3Dd k = new Vector3Dd(0, 0, 1);

        for ( y = 0; y < ySize; y++ ) {
            for ( x = 0; x < xSize; x++ ) {
                n = getNormal(x, y);
                n = n.normalized();

                val = (int)((1.0-k.dotProduct(n)) * 255.0);
                col = VSDK.unsigned8BitInteger2signedByte(val);
                output.putPixel(x, y, col, col, col);
            }
        }
        return output;
    }

    /**
    Similar to exportToRgbaImage, but each pixel is equivalent to a magnitude
    of displacement from <0, 0, 1> normal
    */
    public RGBAImageUncompressed exportToRgbaImageGradient()
    {
        RGBAImageUncompressed output = new RGBAImageUncompressed();

        if ( !output.init(xSize, ySize) ) {
            return null;
        }

        int x, y;
        Vector3Dd n;
        int val;
        byte col;
        Vector3Dd k = new Vector3Dd(0, 0, 1);

        for ( y = 0; y < ySize; y++ ) {
            for ( x = 0; x < xSize; x++ ) {
                n = getNormal(x, y);
                n = n.normalized();

                val = (int)((1.0-k.dotProduct(n)) * 255.0);
                col = VSDK.unsigned8BitInteger2signedByte(val);
                output.putPixel(x, y, col, col, col, 
                      VSDK.unsigned8BitInteger2signedByte(128));
/*
                if ( val > 250 ) {
                    output.putPixel(x, y, (byte)0, (byte)0, (byte)0, (byte)0);
                }
                else {
                    output.putPixel(x, y, (byte)0, (byte)0, (byte)0, (byte)255);                }
*/
            }
        }
        return output;
    }

    public Vector3Dd importBumpMap(IndexedColorImageUncompressed inBumpmap, Vector3Dd inScale)
    {
        //-------------------------------------------------------------------
        int xxSize = inBumpmap.getXSize();
        int yySize = inBumpmap.getYSize();
        Vector3Dd scale = inScale;

        //- 1. If the given scale vector is invalid, create a default one ----
        if( scale.x() < VSDK.EPSILON || scale.y() < VSDK.EPSILON ||
            scale.z() < VSDK.EPSILON ) {
            double val = ((double)xxSize) / ((double)yySize);
            if( val < 1.0 ) {
                scale = new Vector3Dd(1.0, 1.0 / val, 1.0);
            }
            else {
                scale = new Vector3Dd(val, 1.0, 1.0);
            }
        }
        bumpMapScale = Vector3Dd.copyOf(scale);
        init(xxSize, yySize);

        //- 2. Calculate the partial derivatives inside the image ----------
        Vector3Dd normal;
        int a, b, c, d;
        int u, v;

        for( u = 1; u < xxSize - 1; u++ ) {
            for( v = 1; v < yySize - 1; v++ ) {
                a = inBumpmap.getPixel(u+1, v);
                b = inBumpmap.getPixel(u-1, v);
                c = inBumpmap.getPixel(u, v+1);
                d = inBumpmap.getPixel(u, v-1);

                Vector3Dd df_du = new Vector3Dd(2, 0, ((double)(a - b)) / 255.0);
                Vector3Dd df_dv = new Vector3Dd(0, 2, ((double)(d - c)) / 255.0);

                normal = df_du.crossProduct(df_dv);

                // Scale the `normal` vector relative to the `inOutScale` vector
                normal = new Vector3Dd(
                    normal.x() * scale.x(),
                    normal.y() * scale.y(),
                    normal.z() * scale.z()
                ).normalized();

                putNormal(u, v, normal);
            }
        }

        //- 3. Copy the derivatives to the image borders -------------------
        // \todo : check why are two pixels down and left needed!
        for( u = 0; u < xxSize; u++ ) {
            putNormal(u, 0, getNormal(u, 1));
            putNormal(u, yySize-2, getNormal(u, yySize-3));
            putNormal(u, yySize-1, getNormal(u, yySize-2));
        }
        for( v = 0; v < yySize; v++ ) {
            putNormal(1, v, getNormal(2, v));
            putNormal(0, v, getNormal(1, v));
            putNormal(xxSize-1, v, getNormal(xxSize-2, v));
        }
        return scale;
    }
}
