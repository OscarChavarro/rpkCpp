package vsdk.toolkit.render;

/**
Philippe Bekaert & Jan Prikryl, October 1998 - March 2000
*/

import java.io.OutputStream;

import vsdk.toolkit.common.logging.Logger;
import vsdk.toolkit.io.image.ImagePersistence;
import vsdk.toolkit.media.Image;
import vsdk.toolkit.media.RGBImageFloatUncompressed;

/**
Destination of a rendered image: an open output stream and the file format
written on it, chosen from the extension of the file name. The images are
written by VITRAL's ImagePersistence:
- PPM and PNG: display RGB (tone mapped and gamma corrected by ScreenBuffer)
- PIC: the high dynamic range radiances, in Radiance's RGBE format
The stream belongs to the caller, which closes it when the image is written.
PNG files are written by VITRAL's ImagePersistenceAwt plugin (the awt/ module),
which ImagePersistence loads when it is on the class path.
*/
public final class ImageOutputTarget {
    public enum Format {
        DISPLAY_PPM,
        DISPLAY_PNG,
        RADIANCE_RGBE
    }

    private final OutputStream outputStream;
    private final Format format;
    private final String driverName;

    private ImageOutputTarget(OutputStream inOutputStream, Format inFormat, String inDriverName) {
        outputStream = inOutputStream;
        format = inFormat;
        driverName = inDriverName;
    }

    /**
    Examines the file name extension in order to decide what file format to
    use (PPM when the stream is a pipe, unless the name is of a PNG file).
    Returns null, after reporting it, for an extension of an unsupported
    format or a null stream.
    */
    public static ImageOutputTarget create(String fileName, OutputStream outputStream, int isPipe) {
        if (outputStream == null) {
            return null;
        }

        // Assume PPM format if pipe, unless it is a PNG file (e.g. compressed)
        String fileExtension = imageFileExtension(fileName);
        if (fileExtension != null && fileExtension.regionMatches(true, 0, "png", 0, 3)) {
            return new ImageOutputTarget(outputStream, Format.DISPLAY_PNG, "PNG");
        }
        if (isPipe != 0) {
            fileExtension = "ppm";
        }
        if (fileExtension != null && fileExtension.regionMatches(true, 0, "ppm", 0, 3)) {
            return new ImageOutputTarget(outputStream, Format.DISPLAY_PPM, "PPM");
        }
        if (fileExtension != null && fileExtension.regionMatches(true, 0, "pic", 0, 3)) {
            // Olaf: HDR PIC output
            return new ImageOutputTarget(outputStream, Format.RADIANCE_RGBE, "high dynamic range PIC");
        }
        Logger.reportMessage(
            null,
            Logger.ERROR,
            "ImageOutputTarget.create",
            String.format("Can't save high dynamic range image to a '%s' file, format not supported.", fileExtension));
        return null;
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

    public Format getFormat() {
        return format;
    }

    public String getDriverName() {
        return driverName;
    }

    public boolean writeDisplayImage(Image image) {
        if (format == Format.DISPLAY_PNG) {
            try {
                ImagePersistence.exportPNG(outputStream, image);
            }
            catch (Exception e) {
                return false;
            }
            return true;
        }
        return ImagePersistence.exportPPM(outputStream, image);
    }

    public boolean writeRadianceImage(RGBImageFloatUncompressed image) {
        return ImagePersistence.exportRGBE(outputStream, image);
    }
}
