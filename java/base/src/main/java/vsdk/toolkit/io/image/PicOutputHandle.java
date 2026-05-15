package vsdk.toolkit.io.image;

import java.io.File;
import java.io.FileOutputStream;
import java.io.OutputStream;
import java.nio.charset.StandardCharsets;
import java.time.LocalDate;
import java.time.format.DateTimeFormatter;
import java.util.Locale;

import vsdk.toolkit.common.color.ColorRgb;
import vsdk.toolkit.io.PersistenceElement;

/**
High dynamic range PIC output handle.

Olaf Appeltants, March 2000
*/
public final class PicOutputHandle extends ImageOutputHandle {
    private OutputStream outputStream;

    private static String formatToString(String format, Object... arguments) {
        if (format == null) {
            return "";
        }
        try {
            return String.format(Locale.US, format, arguments);
        }
        catch (Exception ignored) {
            return "";
        }
    }

    private static void writeFormatted(OutputStream outputStream, String format, Object... arguments) {
        if (outputStream == null || format == null) {
            return;
        }

        String text = formatToString(format, arguments);
        if (text.isEmpty()) {
            return;
        }

        byte[] bytes = text.getBytes(StandardCharsets.US_ASCII);
        PersistenceElement.writeBytes(outputStream, bytes, bytes.length);
    }

    public PicOutputHandle(String filename, int w, int h) {
        init("high dynamic range PIC", w, h);

        File file = new File(filename);
        if (!file.canWrite() || file.isDirectory()) {
            outputStream = null;
            System.err.printf("Can't open PIC output");
            return;
        }

        try {
            outputStream = new FileOutputStream(filename);
        }
        catch (Exception ignored) {
            outputStream = null;
            System.err.printf("Can't open PIC output");
            return;
        }

        writeHeader();
    }

    void closeHandle() {
        if (outputStream != null) {
            try {
                outputStream.close();
            }
            catch (Exception ignored) {
            }
        }
        outputStream = null;
    }

    /**
Writes scanline of high-dynamic range radiance data in RGB format
*/
    @Override
    public int writeRadianceRGB(ColorRgb[] rgbRadiance) {
        int result = 0;

        if (outputStream != null) {
            result = DkColor.writeScan(rgbRadiance, width, outputStream);
        }

        if (result != 0) {
            return width;
        }
        else {
            // We don't know how many pixels were actually written
            return 0;
        }
    }

    private void writeHeader() {
        // Simple RADIANCE header
        writeFormatted(outputStream, "#?RADIANCE\n");
        String compileDate = LocalDate.now().format(DateTimeFormatter.ofPattern("MMM dd yyyy", Locale.US));
        writeFormatted(outputStream, "#RPK PicOutputHandler (compiled %s)\n", compileDate);
        writeFormatted(outputStream, "FORMAT=32-bit_rle_rgbe\n");
        writeFormatted(outputStream, "\n");
        writeFormatted(outputStream, "-Y %d +X %d\n", height, width);
    }
}
