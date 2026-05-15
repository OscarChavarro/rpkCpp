package vsdk.toolkit.material;

// First bytes correspond to bottom-left pixel (as in OpenGL)

import java.util.Arrays;
import vsdk.toolkit.common.color.ColorRgb;

public class Texture {
    private int width;
    private int height;
    private int channels;
    private byte[] data;

    private static void setMonochrome(ColorRgb rgb, float val) {
        rgb.set(val, val, val);
    }

    public Texture() {
        width = 0;
        height = 0;
        channels = 0;
        data = null;
    }

    public Texture(int inWidth, int inHeight, int inChannels, byte[] inData) {
        width = inWidth;
        height = inHeight;
        channels = inChannels;
        data = null;

        long byteCount = (long)width * (long)height * (long)channels;
        if (byteCount <= 0 || inData == null) {
            return;
        }
        data = Arrays.copyOf(inData, (int)byteCount);
    }

    public int getWidth() {
        return width;
    }

    public int getHeight() {
        return height;
    }

    public int getChannels() {
        return channels;
    }

    public byte[] getData() {
        return data;
    }

    public ColorRgb evaluateColor(float u, float v) {
        ColorRgb rgb = new ColorRgb();
        rgb.clear();

        if (data == null || width <= 0 || height <= 0 || channels <= 0) {
            return rgb;
        }

        double u1 = u - Math.floor(u);
        double u0 = 1.0 - u1;
        double v1 = v - Math.floor(v);
        double v0 = 1.0 - v1;

        int i = (int)(u1 * width);
        int i1 = i + 1;
        int j = (int)(v1 * height);
        int j1 = j + 1;

        if (i < 0) {
            i = 0;
        }
        if (i >= width) {
            i = width - 1;
        }
        if (j < 0) {
            j = 0;
        }
        if (j >= height) {
            j = height - 1;
        }
        if (i1 >= width) {
            i1 -= width;
        }
        if (j1 >= height) {
            j1 -= height;
        }

        int pixelIndex00 = (j * width + i) * channels;
        int pixelIndex01 = (j1 * width + i) * channels;
        int pixelIndex10 = (j * width + i1) * channels;
        int pixelIndex11 = (j1 * width + i1) * channels;

        ColorRgb rgb00 = new ColorRgb();
        ColorRgb rgb10 = new ColorRgb();
        ColorRgb rgb01 = new ColorRgb();
        ColorRgb rgb11 = new ColorRgb();

        switch (channels) {
            case 1:
                setMonochrome(rgb00, channelValue(pixelIndex00, 0));
                setMonochrome(rgb10, channelValue(pixelIndex10, 0));
                setMonochrome(rgb01, channelValue(pixelIndex01, 0));
                setMonochrome(rgb11, channelValue(pixelIndex11, 0));
                break;
            case 3:
            case 4:
                rgb00.set(channelValue(pixelIndex00, 0), channelValue(pixelIndex00, 1), channelValue(pixelIndex00, 2));
                rgb10.set(channelValue(pixelIndex10, 0), channelValue(pixelIndex10, 1), channelValue(pixelIndex10, 2));
                rgb01.set(channelValue(pixelIndex01, 0), channelValue(pixelIndex01, 1), channelValue(pixelIndex01, 2));
                rgb11.set(channelValue(pixelIndex11, 0), channelValue(pixelIndex11, 1), channelValue(pixelIndex11, 2));
                break;
            default:
                break;
        }

        rgb.set(
            (float)(0.25 * (u0 * v0 * rgb00.r + u1 * v0 * rgb10.r + u0 * v1 * rgb01.r + u1 * v1 * rgb11.r)),
            (float)(0.25 * (u0 * v0 * rgb00.g + u1 * v0 * rgb10.g + u0 * v1 * rgb01.g + u1 * v1 * rgb11.g)),
            (float)(0.25 * (u0 * v0 * rgb00.b + u1 * v0 * rgb10.b + u0 * v1 * rgb01.b + u1 * v1 * rgb11.b)));

        return rgb;
    }

    private float channelValue(int pixelIndex, int channel) {
        int value = data[pixelIndex + channel] & 0xFF;
        return (float)value / 255.0f;
    }
}
