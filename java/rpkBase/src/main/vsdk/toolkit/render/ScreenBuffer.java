package vsdk.toolkit.render;

import java.io.OutputStream;

import vsdk.toolkit.common.color.ColorRgb;
import vsdk.toolkit.common.logging.Logger;
import vsdk.toolkit.common.linealAlgebra.Numeric;
import vsdk.toolkit.common.linealAlgebra.Vector2Dd;
import vsdk.toolkit.common.linealAlgebra.Vector3Dd;
import vsdk.toolkit.io.image.ImageOutputHandle;
import vsdk.toolkit.io.wrapper.FileUncompressWrapper;
import vsdk.toolkit.scene.Camera;
import vsdk.toolkit.tonemap.ToneMap;
import vsdk.toolkit.tonemap.ToneMappingContext;

/**
Class for storing pixel radiance/fluxes
and an associated RGB framebuffer

12/10/99 : The screen buffer has now a camera variable that states
           from where the image is made. Several functions
           are provided to handle pixel double coordinates,
           pixel numbers and screen boundaries. These are
           derived from the camera variable
*/
public class ScreenBuffer {
    private ColorRgb[] radiance;
    private ColorRgb[] rgbColor;
    private Camera camera;

    private boolean synced;
    private double factor;
    private double addFactor;
    private boolean rgbImage; // Indicates an RGB image ( = no radiance conversion!)
    private ToneMappingContext toneMapOptions;

    private static Camera copyCamera(Camera source) {
        if (source == null) {
            return null;
        }

        Camera target = new Camera();

        target.eyePosition = source.eyePosition;
        target.lookPosition = source.lookPosition;
        target.upDirection = source.upDirection;
        target.viewDistance = source.viewDistance;
        target.fieldOfVision = source.fieldOfVision;
        target.horizontalFov = source.horizontalFov;
        target.verticalFov = source.verticalFov;
        target.near = source.near;
        target.far = source.far;
        target.xSize = source.xSize;
        target.ySize = source.ySize;
        target.X = source.X;
        target.Y = source.Y;
        target.Z = source.Z;
        target.background = new ColorRgb(source.background.getR(), source.background.getG(), source.background.getB());
        target.changed = source.changed;
        target.pixelWidth = source.pixelWidth;
        target.pixelHeight = source.pixelHeight;
        target.pixelWidthTangent = source.pixelWidthTangent;
        target.pixelHeightTangent = source.pixelHeightTangent;
        for (int i = 0; i < Camera.NUMBER_OF_VIEW_PLANES; i++) {
            target.viewPlanes[i].normal = source.viewPlanes[i].normal;
            target.viewPlanes[i].d = source.viewPlanes[i].d;
        }

        return target;
    }

    private void init(Camera inCamera, Camera defaultCamera) {
        if (inCamera == null) {
            // Use the current camera
            inCamera = defaultCamera;
        }

        if (inCamera == null) {
            Logger.reportMessage(this, Logger.FATAL_ERROR, "ScreenBuffer::init", "Camera not set");
            return;
        }

        if ((radiance != null) && (inCamera.xSize != camera.xSize || inCamera.ySize != camera.ySize)) {
            rgbColor = null;
            radiance = null;
        }

        camera = copyCamera(inCamera);

        if (radiance == null) {
            radiance = new ColorRgb[camera.xSize * camera.ySize];
            rgbColor = new ColorRgb[camera.xSize * camera.ySize];
            for (int i = 0; i < camera.xSize * camera.ySize; i++) {
                radiance[i] = new ColorRgb();
                rgbColor[i] = new ColorRgb();
            }
        }

        // Clear
        ColorRgb black = new ColorRgb(0.0, 0.0, 0.0);
        for (int i = 0; i < camera.xSize * camera.ySize; i++) {
            radiance[i] = new ColorRgb(0.0, 0.0, 0.0);
            rgbColor[i] = new ColorRgb(black.getR(), black.getG(), black.getB());
        }

        factor = 1.0;
        addFactor = 1.0;
        synced = true;
        rgbImage = false;
    }

    /**
Constructor : make an screen buffer from a camera definition
*/
    public ScreenBuffer(Camera camera, Camera defaultCamera, ToneMappingContext inToneMapOptions) {
        radiance = null;
        rgbColor = null;
        this.camera = new Camera();
        toneMapOptions = inToneMapOptions;
        init(camera, defaultCamera);
        synced = false;
        factor = 1.0;
        addFactor = 1.0;
        rgbImage = false;
    }

    public ScreenBuffer(Camera camera, Camera defaultCamera) {
        this(camera, defaultCamera, null);
    }

    public boolean isRgbImage() {
        return rgbImage;
    }

    /**
Copy dimensions and contents (radiance only) from source
*/
    public void copy(ScreenBuffer source, Camera defaultCamera) {
        init(source.camera, defaultCamera);
        rgbImage = source.isRgbImage();

        // Now the resolution is ok.
        for (int i = 0; i < camera.xSize * camera.ySize; i++) {
            radiance[i] = new ColorRgb(source.radiance[i].getR(), source.radiance[i].getG(), source.radiance[i].getB());
        }
        synced = false;
    }

    /**
Merge (add) two screen buffers (radiance only) from src1 and src2
*/
    public void merge(ScreenBuffer src1, ScreenBuffer src2, Camera defaultCamera) {
        init(src1.camera, defaultCamera);
        rgbImage = src1.isRgbImage();

        if ((getHRes() != src2.getHRes()) || (getVRes() != src2.getVRes())) {
            Logger.reportMessage(this, Logger.ERROR, "ScreenBuffer::merge", "Incompatible screen buffer sources");
            return;
        }

        int N = getVRes() * getHRes();

        for (int i = 0; i < N; i++) {
            radiance[i] = new ColorRgb(
                src1.radiance[i].getR() + src2.radiance[i].getR(),
                src1.radiance[i].getG() + src2.radiance[i].getG(),
                src1.radiance[i].getB() + src2.radiance[i].getB());
        }
    }

    public void add(int x, int y, ColorRgb inRadiance) {
        int index = x + (camera.ySize - y - 1) * camera.xSize;

        radiance[index] = new ColorRgb(
            radiance[index].getR() + addFactor * inRadiance.getR(),
            radiance[index].getG() + addFactor * inRadiance.getG(),
            radiance[index].getB() + addFactor * inRadiance.getB());
        synced = false;
    }

    public void set(int x, int y, ColorRgb inRadiance) {
        int index = x + (camera.ySize - y - 1) * camera.xSize;
        radiance[index] = new ColorRgb(
            addFactor * inRadiance.getR(),
            addFactor * inRadiance.getG(),
            addFactor * inRadiance.getB());
        synced = false;
    }

    public ColorRgb get(int x, int y) {
        int index = x + (camera.ySize - y - 1) * camera.xSize;

        return radiance[index];
    }

    public void render() {
        if (!synced) {
            sync();
        }

        SoftIds.softRenderPixels(camera.xSize, camera.ySize, rgbColor, requireToneMappingContext());
    }

    public void writeFile(ImageOutputHandle ip) {
        if (ip == null) {
            return;
        }

        if (!synced) {
            sync();
        }

        System.err.printf("Writing %s file ... ", ip.driverName);

        ToneMappingContext activeToneMapOptions = requireToneMappingContext();
        ip.setToneMappingContext(activeToneMapOptions);
        ip.gamma[0] = (double)activeToneMapOptions.gamma.getR(); // For default radiance -> display RGB
        ip.gamma[1] = (double)activeToneMapOptions.gamma.getG();
        ip.gamma[2] = (double)activeToneMapOptions.gamma.getB();
        for (int i = camera.ySize - 1; i >= 0; i--) {
            // Write scan lines
            if (!isRgbImage()) {
                ColorRgb[] scanline = new ColorRgb[camera.xSize];
                int rowStart = i * camera.xSize;
                for (int j = 0; j < camera.xSize; j++) {
                    scanline[j] = radiance[rowStart + j];
                }
                ip.writeRadianceRGB(scanline);
            }
            else {
                double[] rgbFloatArray = new double[3 * camera.xSize];
                int rowStart = i * camera.xSize;
                for (int j = 0; j < camera.xSize; j++) {
                    ColorRgb color = radiance[rowStart + j];
                    int base = 3 * j;
                    rgbFloatArray[base] = (double)color.getR();
                    rgbFloatArray[base + 1] = (double)color.getG();
                    rgbFloatArray[base + 2] = (double)color.getB();
                }
                ip.writeDisplayRGB(rgbFloatArray);
            }
        }

        System.err.printf("done.\n");
    }

    public void renderScanline(int y) {
        y = camera.ySize - y - 1;

        if (!synced) {
            syncLine(y);
        }

        ColorRgb[] scanline = new ColorRgb[camera.xSize];
        int rowStart = y * camera.xSize;
        for (int i = 0; i < camera.xSize; i++) {
            scanline[i] = rgbColor[rowStart + i];
        }
        SoftIds.softRenderPixels(camera.xSize, 1, scanline, requireToneMappingContext());
    }

    public void sync() {
        ColorRgb tmpRad = new ColorRgb();
        ToneMappingContext activeToneMapOptions = requireToneMappingContext();

        for (int i = 0; i < camera.xSize * camera.ySize; i++) {
            tmpRad = (new ColorRgb(radiance[i])).multiply(factor);
            if (!isRgbImage()) {
                rgbColor[i] = ToneMap.radianceToRgb(tmpRad, activeToneMapOptions);
            }
            else {
                tmpRad = new ColorRgb(rgbColor[i].getR(), rgbColor[i].getG(), rgbColor[i].getB());
            }
        }

        synced = true;
    }

    protected void syncLine(int lineNumber) {
        ColorRgb tmpRad = new ColorRgb();
        ToneMappingContext activeToneMapOptions = requireToneMappingContext();

        for (int i = 0; i < camera.xSize; i++) {
            tmpRad = (new ColorRgb(radiance[lineNumber * camera.xSize + i])).multiply(factor);
            if (!isRgbImage()) {
                rgbColor[lineNumber * camera.xSize + i] = ToneMap.radianceToRgb(tmpRad, activeToneMapOptions);
            }
            else {
                tmpRad = new ColorRgb(rgbColor[lineNumber * camera.xSize + i].getR(), rgbColor[lineNumber * camera.xSize + i].getG(), rgbColor[lineNumber * camera.xSize + i].getB());
            }
        }
    }

    protected ToneMappingContext requireToneMappingContext() {
        if (toneMapOptions == null) {
            Logger.reportMessage(this, Logger.FATAL_ERROR, "ScreenBuffer::requireToneMappingContext", "Tone mapping context not set");
        }
        return toneMapOptions;
    }

    public void setToneMappingContext(ToneMappingContext inToneMapOptions) {
        toneMapOptions = inToneMapOptions;
    }

    public double getScreenXMin() {
        return -camera.pixelWidth * (double)camera.xSize / 2.0;
    }

    public double getScreenYMin() {
        return -camera.pixelHeight * (double)camera.ySize / 2.0;
    }

    public double getPixXSize() {
        return camera.pixelWidth;
    }

    public double getPixYSize() {
        return camera.pixelHeight;
    }

    public Vector2Dd getPixelPoint(int nx, int ny, double xOffset, double yOffset) {
        return new Vector2Dd(
            getScreenXMin() + ((double)nx + xOffset) * getPixXSize(),
            getScreenYMin() + ((double)ny + yOffset) * getPixYSize());
    }

    public Vector2Dd getPixelPoint(int nx, int ny) {
        return getPixelPoint(nx, ny, 0.5, 0.5);
    }

    public Vector2Dd getPixelCenter(int nx, int ny) {
        return getPixelPoint(nx, ny, 0.5, 0.5);
    }

    public int getNx(double x) {
        return (int)Math.floor((x - getScreenXMin()) / getPixXSize());
    }

    public int getNy(double y) {
        return (int)Math.floor((y - getScreenYMin()) / getPixYSize());
    }

    public void getPixel(double x, double y, int[] nx, int[] ny) {
        if (nx != null && nx.length > 0) {
            nx[0] = getNx(x);
        }
        if (ny != null && ny.length > 0) {
            ny[0] = getNy(y);
        }
    }

    /**
Un-normalized vector pointing from the eye point to the
point with given fractional pixel coordinates
*/
    public Vector3Dd getPixelVector(int nx, int ny, double xOffset, double yOffset) {
        Vector2Dd pix = getPixelPoint(nx, ny, xOffset, yOffset);
        Vector3Dd dir = new Vector3Dd();
        dir = camera.Z.add(camera.X.multiply(pix.x())).add(camera.Y.multiply(pix.y()));
        return dir;
    }

    public Vector3Dd getPixelVector(int nx, int ny) {
        return getPixelVector(nx, ny, 0.5, 0.5);
    }

    /**
Screen resolution
*/
    public int getHRes() {
        return camera.xSize;
    }

    public int getVRes() {
        return camera.ySize;
    }

    public static double computeFluxToRadFactor(Camera camera, int pixX, int pixY) {
        Vector3Dd dir = new Vector3Dd();
        double h = camera.pixelWidth;
        double v = camera.pixelHeight;

        double x = -h * camera.xSize / 2.0 + pixX * h;
        double y = -v * camera.ySize / 2.0 + pixY * v;

        double xSample = x + h * 0.5;  // (pixX, pixY) indicate upper left
        double ySample = y + v * 0.5;

        dir = camera.Z.add(camera.X.multiply((double)xSample)).add(camera.Y.multiply((double)ySample));
        double distPixel2 = dir.lengthSquared();
        double distPixel = Math.sqrt(distPixel2);
        dir = dir.multiplyByInverse((double)distPixel, Numeric.EPSILON);

        double factor = 1.0 / (h * v);

        factor *= distPixel2; // r(eye->pixel)^2
        factor /= Math.pow(dir.dotProduct(camera.Z), 2);  // cos^2

        return (double)factor;
    }

    public double getScreenXMax() {
        return camera.pixelWidth * (double)camera.xSize / 2.0;
    }

    public double getScreenYMax() {
        return camera.pixelHeight * (double)camera.ySize / 2.0;
    }

    public ColorRgb getBiLinear(double x, double y) {
        int[] nx0v = new int[1];
        int[] ny0v = new int[1];
        int nx1;
        int ny1;
        Vector2Dd center;
        ColorRgb color = new ColorRgb();

        getPixel(x, y, nx0v, ny0v);
        int nx0 = nx0v[0];
        int ny0 = ny0v[0];
        center = getPixelCenter(nx0, ny0);

        x = (x - center.x()) / getPixXSize();
        y = (y - center.y()) / getPixYSize();

        if (x < 0) {
            // Point on left side of pixel center
            x = -x;
            nx1 = Math.max(nx0 - 1, 0);
        }
        else {
            nx1 = Math.min(getHRes(), nx0 + 1);
        }

        if (y < 0) {
            y = -y;
            ny1 = Math.max(ny0 - 1, 0);
        }
        else {
            ny1 = Math.min(getVRes(), ny0 + 1);
        }

        // u = 0 for nx0 and u = 1 for nx1, x in-between. Not that
        // nx0 and nx1 may be the same (at border of image). Same for ny

        ColorRgb c0 = get(nx0, ny0); // Separate vars, since interpolation is a macro...
        ColorRgb c1 = get(nx1, ny0); // u = 1
        ColorRgb c2 = get(nx1, ny1); // u = 1, v = 1
        ColorRgb c3 = get(nx0, ny1); // v = 1

        {
            final double cc = x * y;
            final double bb = x - cc;
            final double dd = y - cc;
            color = new ColorRgb(
                new ColorRgb(c0).getR() + bb * (new ColorRgb(c1).getR() - new ColorRgb(c0).getR()) + cc * (new ColorRgb(c2).getR() - new ColorRgb(c0).getR()) + dd * (new ColorRgb(c3).getR() - new ColorRgb(c0).getR()),
                new ColorRgb(c0).getG() + bb * (new ColorRgb(c1).getG() - new ColorRgb(c0).getG()) + cc * (new ColorRgb(c2).getG() - new ColorRgb(c0).getG()) + dd * (new ColorRgb(c3).getG() - new ColorRgb(c0).getG()),
                new ColorRgb(c0).getB() + bb * (new ColorRgb(c1).getB() - new ColorRgb(c0).getB()) + cc * (new ColorRgb(c2).getB() - new ColorRgb(c0).getB()) + dd * (new ColorRgb(c3).getB() - new ColorRgb(c0).getB()));
        }

        return new ColorRgb(color);
    }

    public void scaleRadiance(double inFactor) {
        for (int i = 0; i < camera.xSize * camera.ySize; i++) {
            radiance[i] = new ColorRgb(
                radiance[i].getR() * inFactor,
                radiance[i].getG() * inFactor,
                radiance[i].getB() * inFactor);
        }

        synced = false;
    }

    public void setAddScaleFactor(double inFactor) {
        addFactor = inFactor;
    }

    public void setFactor(double inFactor) {
        factor = inFactor;
    }

    public void setRgbImage(boolean isRGB) {
        rgbImage = isRGB;
    }

    public void writeFile(String fileName, OutputStream outputStream, int isPipe) {
        if (outputStream == null) {
            return;
        }

        ImageOutputHandle ip = ImageOutputHandle.createRadianceImageOutputHandle(
            fileName,
            outputStream,
            isPipe,
            camera.xSize,
            camera.ySize);

        writeFile(ip);
        if (ip != null) {
            ImageOutputHandle.deleteImageOutputHandle(ip);
        }
    }

    public void writeFile(String fileName) {
        int[] isPipe = new int[] {0};
        OutputStream outputStream = FileUncompressWrapper.openOutputStreamCompressWrapper(fileName, isPipe);
        if (outputStream == null) {
            return;
        }

        writeFile(fileName, outputStream, isPipe[0]);
        FileUncompressWrapper.closeOutputStream(outputStream);
    }
}
