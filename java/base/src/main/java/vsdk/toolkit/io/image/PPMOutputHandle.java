package vsdk.toolkit.io.image;

import java.io.OutputStream;
import java.nio.charset.StandardCharsets;
import java.util.Locale;
import vsdk.toolkit.io.PersistenceElement;

public final class PPMOutputHandle extends ImageOutputHandle {
    private OutputStream outputStream;

    public PPMOutputHandle(OutputStream _outputStream, int w, int h) {
        init("PPM", w, h);
        outputStream = _outputStream;

        if (outputStream != null) {
            String header = String.format(Locale.US, "P6\n%d %d\n255\n", width, height);
            byte[] headerBytes = header.getBytes(StandardCharsets.US_ASCII);
            if (headerBytes.length > 0) {
                PersistenceElement.writeBytes(outputStream, headerBytes, headerBytes.length);
            }
        }
    }

    @Override
    public int writeDisplayRGB(byte[] rgb) {
        if (outputStream != null) {
            PersistenceElement.writeBytes(outputStream, rgb, 3 * width);
            return width;
        }
        return 0;
    }
}
