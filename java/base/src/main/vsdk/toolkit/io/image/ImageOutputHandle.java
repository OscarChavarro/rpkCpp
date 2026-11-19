/**
Interface for writing image data in different file formats
*/

package vsdk.toolkit.io.image;

/**
Philippe Bekaert & Jan Prikryl, October 1998 - March 2000
*/

import java.io.OutputStream;

import vsdk.toolkit.common.color.ColorRgb;
import vsdk.toolkit.common.logging.Logger;
import vsdk.toolkit.tonemap.ToneMap;
import vsdk.toolkit.tonemap.ToneMappingContext;

public class ImageOutputHandle {
    protected int width;
    protected int height;

    // Image file output driver name
    public String driverName;

    // Gamma correction factors for red, green and blue  used by default
    public float[] gamma;
    public ToneMappingContext toneMapOptions;

    public ImageOutputHandle() {
        width = 0;
        height = 0;
        driverName = null;
        gamma = new float[3];
        toneMapOptions = null;
    }

    protected void init(String _name, int _width, int _height) {
        driverName = _name;
        width = _width;
        height = _height;
        gamma[0] = 1.0f;
        gamma[1] = 1.0f;
        gamma[2] = 1.0f;
        toneMapOptions = null;
    }

    public void setToneMappingContext(ToneMappingContext inToneMapOptions) {
        toneMapOptions = inToneMapOptions;
    }

    // Writes a scanline of gamma-corrected display RGB pixels
    // returns the number of pixels written
    public int writeDisplayRGB(byte[] x) {
        System.err.printf("%s does not support display RGB output.\n", driverName);
        return 0;
    }

    protected static ColorRgb gammaCorrect(ColorRgb rgb, final float[] gamma) {
        double r = gamma[0] == 1.0f ? rgb.getR() : Math.pow(rgb.getR(), 1.0f / gamma[0]);
        double g = gamma[1] == 1.0f ? rgb.getG() : Math.pow(rgb.getG(), 1.0f / gamma[1]);
        double b = gamma[2] == 1.0f ? rgb.getB() : Math.pow(rgb.getB(), 1.0f / gamma[2]);
        return new ColorRgb(r, g, b);
    }

    public int writeDisplayRGB(float[] rgbFloatArray) {
        byte[] rgb = new byte[3 * width];
        for (int i = 0; i < width; i++) {
            // Convert RGB radiance to display RGB
            ColorRgb displayRgb = new ColorRgb(
                rgbFloatArray[3 * i],
                rgbFloatArray[3 * i + 1],
                rgbFloatArray[3 * i + 2]);
            // Apply gamma correction
            displayRgb = gammaCorrect(displayRgb, gamma);
            // Convert float to byte representation
            rgb[3 * i] = (byte)(int)(displayRgb.getR() * 255.0f);
            rgb[3 * i + 1] = (byte)(int)(displayRgb.getG() * 255.0f);
            rgb[3 * i + 2] = (byte)(int)(displayRgb.getB() * 255.0f);
        }

        // Output display RGB values
        int pixelsWriten = writeDisplayRGB(rgb);

        return pixelsWriten;
    }

    /**
Writes a scanline of raw radiance data
returns the number of pixels written
*/
    public int writeRadianceRGB(ColorRgb[] rgbRadiance) {
        if (toneMapOptions == null) {
            Logger.fatal(-1, "ImageOutputHandle::writeRadianceRGB", "Tone mapping context not set");
        }

        byte[] rgb = new byte[3 * width];
        for (int i = 0; i < width; i++) {
            // Convert RGB radiance to display RGB
            ColorRgb displayRgb = new ColorRgb();
            ToneMap.radianceToRgb(rgbRadiance[i], displayRgb, toneMapOptions);

            // Apply gamma correction
            displayRgb = gammaCorrect(displayRgb, gamma);

            // Convert float to byte representation
            rgb[3 * i] = (byte)(int)(displayRgb.getR() * 255.0f);
            rgb[3 * i + 1] = (byte)(int)(displayRgb.getG() * 255.0f);
            rgb[3 * i + 2] = (byte)(int)(displayRgb.getB() * 255.0f);
        }

        // Output display RGB values
        int pixelsWriten = writeDisplayRGB(rgb);

        return pixelsWriten;
    }

    /**
Returns file name extension. Understands extra suffixes ".Z", ".gz",
".bz", and ".bz2".
*/
    public static String imageFileExtension(String fileName) {
        int fileNameLength = fileName == null ? 0 : fileName.length();
        if (fileNameLength <= 0) {
            return fileName;
        }

        int extensionDotIndex = fileNameLength - 1;
        while (extensionDotIndex >= 0 && fileName.charAt(extensionDotIndex) != '.') {
            extensionDotIndex--;
        }

        if (extensionDotIndex < 0) {
            return fileName;
        }

        String fileExtension = fileName.substring(extensionDotIndex);
        if (".Z".equals(fileExtension) ||
            ".gz".equals(fileExtension) ||
            ".bz".equals(fileExtension) ||
            ".bz2".equals(fileExtension)) {
            extensionDotIndex--;
            while (extensionDotIndex >= 0 && fileName.charAt(extensionDotIndex) != '.') {
                extensionDotIndex--;
            }
            if (extensionDotIndex < 0) {
                return fileName;
            }
        }

        return fileName.substring(extensionDotIndex + 1);
    }

    /**
Examines filename extension in order to decide what file format to
use to write radiance image
*/
    public static ImageOutputHandle
    createRadianceImageOutputHandle(
        String fileName,
        OutputStream outputStream,
        int isPipe,
        int width,
        int height)
    {
        if (outputStream != null) {
            String fileExtension = isPipe != 0 ? "ppm" : imageFileExtension(fileName);
            // Assume PPM format if pipe
            if (fileExtension != null && fileExtension.regionMatches(true, 0, "ppm", 0, 3)) {
                return new PPMOutputHandle(outputStream, width, height);
            }
            // Olaf: HDR PIC output
            else if (fileExtension != null && fileExtension.regionMatches(true, 0, "pic", 0, 3)) {
                if (isPipe != 0) {
                    Logger.error("createRadianceImageOutputHandle",
                        "Can't write PIC output to a pipe.\n");
                    return null;
                }

                return new PicOutputHandle(fileName, width, height);
            }
            else {
                Logger.error("createRadianceImageOutputHandle",
                    "Can't save high dynamic range image to a '%s' file, format not supported.",
                    fileExtension);
                return null;
            }
        }
        return null;
    }

    /**
Same, but for writing "normal" display RGB images instead radiance image
*/
    public static ImageOutputHandle
    createImageOutputHandle(
        String fileName,
        OutputStream outputStream,
        final int isPipe,
        final int width,
        final int height)
    {
        if (outputStream != null) {
            String fileExtension = isPipe != 0 ? "ppm" : imageFileExtension(fileName);

            if (fileExtension != null && fileExtension.regionMatches(true, 0, "ppm", 0, 3)) {
                return new PPMOutputHandle(outputStream, width, height);
            }
            else {
                Logger.error("createImageOutputHandle",
                    "Can't save display-RGB images to a '%s' file, format not supported.\n",
                    fileExtension);
                return null;
            }
        }
        return null;
    }

    /**
Write a scanline of display RGB, RGB radiance or CIE XYZ radiance data.
3 samples per pixel: RGB order for RGB data and XYZ order for CIE XYZ data
*/
    public static int writeDisplayRGB(ImageOutputHandle img, byte[] data) {
        return img.writeDisplayRGB(data);
    }

    /**
Finish writing the image
*/
    public static void deleteImageOutputHandle(ImageOutputHandle img) {
        if (img == null) {
            return;
        }
        if (img instanceof PicOutputHandle) {
            ((PicOutputHandle)img).closeHandle();
        }
    }

    /**
The following ImageOutputHandle constructors are only needed if you want to specify
yourself what format to use
*/
}
