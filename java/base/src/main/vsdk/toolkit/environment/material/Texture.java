package vsdk.toolkit.environment.material;

import vsdk.toolkit.common.color.ColorRgb;

/**
Color texture of a PhongMaterial: `channels` bytes per pixel (1: gray, 3:
RGB, 4: RGBA), rows from the bottom up. Immutable once built; the data is
copied by the constructor.
*/
public final class Texture {
    private final int width;
    private final int height;
    private final int channels;
    private final byte[] data; // First bytes correspond to bottom-left pixel (as in OpenGL)

    public Texture()
    {
        width = 0;
        height = 0;
        channels = 0;
        data = null;
    }

    /** @param inData unsigned bytes (cpp `unsigned char`), copied */
    public Texture(int inWidth, int inHeight, int inChannels, byte[] inData)
    {
        width = inWidth;
        height = inHeight;
        channels = inChannels;
        final long byteCount = (long)width * (long)height * (long)channels;
        if ( byteCount <= 0 || inData == null ) {
            data = null;
            return;
        }
        data = new byte[(int)byteCount];
        System.arraycopy(inData, 0, data, 0, (int)byteCount);
    }

    public int getWidth()
    {
        return width;
    }

    public int getHeight()
    {
        return height;
    }

    public int getChannels()
    {
        return channels;
    }

    public byte[] getData()
    {
        return data;
    }

    private double channelValue(int pixelIndex, int channel)
    {
        return (double)(data[pixelIndex + channel] & 0xFF) / 255.0;
    }

    public ColorRgb evaluateColor(double u, double v)
    {
        double u1 = u - Math.floor(u);
        double u0 = 1.0 - u1;
        double v1 = v - Math.floor(v);
        double v0 = 1.0 - v1;
        int i = (int)(u1 * width);
        int i1 = i + 1;
        int j = (int)(v1 * height);
        int j1 = j + 1;
        if ( i < 0 ) {
            i = 0;
        }
        if ( i >= width ) {
            i = width - 1;
        }
        if ( j < 0 ) {
            j = 0;
        }
        if ( j >= height ) {
            j = height - 1;
        }
        if ( i1 >= width ) {
            i1 -= width;
        }
        if ( j1 >= height ) {
            j1 -= height;
        }

        if ( data == null ) {
            return new ColorRgb(0.0, 0.0, 0.0);
        }

        final int pixelIndex00 = (j * width + i) * channels;
        final int pixelIndex01 = (j1 * width + i) * channels;
        final int pixelIndex10 = (j * width + i1) * channels;
        final int pixelIndex11 = (j1 * width + i1) * channels;

        double r00 = 0.0;
        double g00 = 0.0;
        double b00 = 0.0;
        double r10 = 0.0;
        double g10 = 0.0;
        double b10 = 0.0;
        double r01 = 0.0;
        double g01 = 0.0;
        double b01 = 0.0;
        double r11 = 0.0;
        double g11 = 0.0;
        double b11 = 0.0;

        switch ( channels ) {
          case 1:
            r00 = g00 = b00 = channelValue(pixelIndex00, 0);
            r10 = g10 = b10 = channelValue(pixelIndex10, 0);
            r01 = g01 = b01 = channelValue(pixelIndex01, 0);
            r11 = g11 = b11 = channelValue(pixelIndex11, 0);
            break;
          case 3:
          case 4:
            r00 = channelValue(pixelIndex00, 0);
            g00 = channelValue(pixelIndex00, 1);
            b00 = channelValue(pixelIndex00, 2);
            r10 = channelValue(pixelIndex10, 0);
            g10 = channelValue(pixelIndex10, 1);
            b10 = channelValue(pixelIndex10, 2);
            r01 = channelValue(pixelIndex01, 0);
            g01 = channelValue(pixelIndex01, 1);
            b01 = channelValue(pixelIndex01, 2);
            r11 = channelValue(pixelIndex11, 0);
            g11 = channelValue(pixelIndex11, 1);
            b11 = channelValue(pixelIndex11, 2);
            break;
          default:
            break;
        }

        final double r = 0.25 * (u0 * v0 * r00 + u1 * v0 * r10 + u0 * v1 * r01 + u1 * v1 * r11);
        final double g = 0.25 * (u0 * v0 * g00 + u1 * v0 * g10 + u0 * v1 * g01 + u1 * v1 * g11);
        final double b = 0.25 * (u0 * v0 * b00 + u1 * v0 * b10 + u0 * v1 * b01 + u1 * v1 * b11);
        return new ColorRgb(r, g, b);
    }
}
