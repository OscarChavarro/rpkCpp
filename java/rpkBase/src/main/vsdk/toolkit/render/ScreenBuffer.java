package vsdk.toolkit.render;

import java.io.OutputStream;

import vsdk.toolkit.common.color.ColorRgb;
import vsdk.toolkit.common.color.ColorRgbMutable;
import vsdk.toolkit.common.logging.Logger;
import vsdk.toolkit.common.linealAlgebra.Numeric;
import vsdk.toolkit.common.linealAlgebra.Vector2Df;
import vsdk.toolkit.common.linealAlgebra.Vector3Df;
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
           are provided to handle pixel float coordinates,
           pixel numbers and screen boundaries. These are
           derived from the camera variable
*/
public class ScreenBuffer {
    private ColorRgbMutable[] radiance;
    private ColorRgbMutable[] rgbColor;
    private Camera camera;

    private boolean synced;
    private float factor;
    private float addFactor;
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
        target.background = new ColorRgbMutable(source.background.getR(), source.background.getG(), source.background.getB());
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
            radiance = new ColorRgbMutable[camera.xSize * camera.ySize];
            rgbColor = new ColorRgbMutable[camera.xSize * camera.ySize];
            for (int i = 0; i < camera.xSize * camera.ySize; i++) {
                radiance[i] = new ColorRgbMutable();
                rgbColor[i] = new ColorRgbMutable();
            }
        }

        // Clear
        ColorRgbMutable black = new ColorRgbMutable(0.0, 0.0, 0.0);
        for (int i = 0; i < camera.xSize * camera.ySize; i++) {
            radiance[i] = new ColorRgbMutable(0.0, 0.0, 0.0);
            rgbColor[i] = new ColorRgbMutable(black.getR(), black.getG(), black.getB());
        }

        factor = 1.0f;
        addFactor = 1.0f;
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
        factor = 1.0f;
        addFactor = 1.0f;
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
            radiance[i] = new ColorRgbMutable(source.radiance[i].getR(), source.radiance[i].getG(), source.radiance[i].getB());
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
            radiance[i] = new ColorRgbMutable(
                src1.radiance[i].getR() + src2.radiance[i].getR(),
                src1.radiance[i].getG() + src2.radiance[i].getG(),
                src1.radiance[i].getB() + src2.radiance[i].getB());
        }
    }

    public void add(int x, int y, ColorRgbMutable inRadiance) {
        int index = x + (camera.ySize - y - 1) * camera.xSize;

        radiance[index] = new ColorRgbMutable(
            radiance[index].getR() + addFactor * inRadiance.getR(),
            radiance[index].getG() + addFactor * inRadiance.getG(),
            radiance[index].getB() + addFactor * inRadiance.getB());
        synced = false;
    }

    public void set(int x, int y, ColorRgbMutable inRadiance) {
        int index = x + (camera.ySize - y - 1) * camera.xSize;
        radiance[index] = new ColorRgbMutable(
            addFactor * inRadiance.getR(),
            addFactor * inRadiance.getG(),
            addFactor * inRadiance.getB());
        synced = false;
    }

    public ColorRgbMutable get(int x, int y) {
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
        ip.gamma[0] = (float)activeToneMapOptions.gamma.getR(); // For default radiance -> display RGB
        ip.gamma[1] = (float)activeToneMapOptions.gamma.getG();
        ip.gamma[2] = (float)activeToneMapOptions.gamma.getB();
        for (int i = camera.ySize - 1; i >= 0; i--) {
            // Write scan lines
            if (!isRgbImage()) {
                ColorRgb[] scanline = new ColorRgb[camera.xSize];
                int rowStart = i * camera.xSize;
                for (int j = 0; j < camera.xSize; j++) {
                    scanline[j] = radiance[rowStart + j].toImmutable();
                }
                ip.writeRadianceRGB(scanline);
            }
            else {
                float[] rgbFloatArray = new float[3 * camera.xSize];
                int rowStart = i * camera.xSize;
                for (int j = 0; j < camera.xSize; j++) {
                    ColorRgbMutable color = radiance[rowStart + j];
                    int base = 3 * j;
                    rgbFloatArray[base] = (float)color.getR();
                    rgbFloatArray[base + 1] = (float)color.getG();
                    rgbFloatArray[base + 2] = (float)color.getB();
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

        ColorRgbMutable[] scanline = new ColorRgbMutable[camera.xSize];
        int rowStart = y * camera.xSize;
        for (int i = 0; i < camera.xSize; i++) {
            scanline[i] = rgbColor[rowStart + i];
        }
        SoftIds.softRenderPixels(camera.xSize, 1, scanline, requireToneMappingContext());
    }

    public void sync() {
        ColorRgbMutable tmpRad = new ColorRgbMutable();
        ToneMappingContext activeToneMapOptions = requireToneMappingContext();

        for (int i = 0; i < camera.xSize * camera.ySize; i++) {
            tmpRad.scaledCopy(factor, new ColorRgbMutable(radiance[i]));
            if (!isRgbImage()) {
                ToneMap.radianceToRgb(tmpRad, rgbColor[i], activeToneMapOptions);
            }
            else {
                tmpRad.set(rgbColor[i].getR(), rgbColor[i].getG(), rgbColor[i].getB());
            }
        }

        synced = true;
    }

    protected void syncLine(int lineNumber) {
        ColorRgbMutable tmpRad = new ColorRgbMutable();
        ToneMappingContext activeToneMapOptions = requireToneMappingContext();

        for (int i = 0; i < camera.xSize; i++) {
            tmpRad.scaledCopy(factor, new ColorRgbMutable(radiance[lineNumber * camera.xSize + i]));
            if (!isRgbImage()) {
                ToneMap.radianceToRgb(tmpRad, rgbColor[lineNumber * camera.xSize + i], activeToneMapOptions);
            }
            else {
                tmpRad.set(
                    rgbColor[lineNumber * camera.xSize + i].getR(),
                    rgbColor[lineNumber * camera.xSize + i].getG(),
                    rgbColor[lineNumber * camera.xSize + i].getB());
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

    public float getScreenXMin() {
        return -camera.pixelWidth * (float)camera.xSize / 2.0f;
    }

    public float getScreenYMin() {
        return -camera.pixelHeight * (float)camera.ySize / 2.0f;
    }

    public float getPixXSize() {
        return camera.pixelWidth;
    }

    public float getPixYSize() {
        return camera.pixelHeight;
    }

    public Vector2Df getPixelPoint(int nx, int ny, float xOffset, float yOffset) {
        return new Vector2Df(
            getScreenXMin() + ((float)nx + xOffset) * getPixXSize(),
            getScreenYMin() + ((float)ny + yOffset) * getPixYSize());
    }

    public Vector2Df getPixelPoint(int nx, int ny) {
        return getPixelPoint(nx, ny, 0.5f, 0.5f);
    }

    public Vector2Df getPixelCenter(int nx, int ny) {
        return getPixelPoint(nx, ny, 0.5f, 0.5f);
    }

    public int getNx(float x) {
        return (int)Math.floor((x - getScreenXMin()) / getPixXSize());
    }

    public int getNy(float y) {
        return (int)Math.floor((y - getScreenYMin()) / getPixYSize());
    }

    public void getPixel(float x, float y, int[] nx, int[] ny) {
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
    public Vector3Df getPixelVector(int nx, int ny, float xOffset, float yOffset) {
        Vector2Df pix = getPixelPoint(nx, ny, xOffset, yOffset);
        Vector3Df dir = new Vector3Df();
        dir = camera.Z.add(camera.X.multiply(pix.x())).add(camera.Y.multiply(pix.y()));
        return dir;
    }

    public Vector3Df getPixelVector(int nx, int ny) {
        return getPixelVector(nx, ny, 0.5f, 0.5f);
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

    public static float computeFluxToRadFactor(Camera camera, int pixX, int pixY) {
        Vector3Df dir = new Vector3Df();
        double h = camera.pixelWidth;
        double v = camera.pixelHeight;

        double x = -h * camera.xSize / 2.0 + pixX * h;
        double y = -v * camera.ySize / 2.0 + pixY * v;

        double xSample = x + h * 0.5;  // (pixX, pixY) indicate upper left
        double ySample = y + v * 0.5;

        dir = camera.Z.add(camera.X.multiply((float)xSample)).add(camera.Y.multiply((float)ySample));
        double distPixel2 = dir.lengthSquared();
        double distPixel = Math.sqrt(distPixel2);
        dir = dir.multiplyByInverse((float)distPixel, Numeric.EPSILON_FLOAT);

        double factor = 1.0 / (h * v);

        factor *= distPixel2; // r(eye->pixel)^2
        factor /= Math.pow(dir.dotProduct(camera.Z), 2);  // cos^2

        return (float)factor;
    }

    public float getScreenXMax() {
        return camera.pixelWidth * (float)camera.xSize / 2.0f;
    }

    public float getScreenYMax() {
        return camera.pixelHeight * (float)camera.ySize / 2.0f;
    }

    public ColorRgbMutable getBiLinear(float x, float y) {
        int[] nx0v = new int[1];
        int[] ny0v = new int[1];
        int nx1;
        int ny1;
        Vector2Df center;
        ColorRgbMutable color = new ColorRgbMutable();

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

        ColorRgbMutable c0 = get(nx0, ny0); // Separate vars, since interpolation is a macro...
        ColorRgbMutable c1 = get(nx1, ny0); // u = 1
        ColorRgbMutable c2 = get(nx1, ny1); // u = 1, v = 1
        ColorRgbMutable c3 = get(nx0, ny1); // v = 1

        color.interpolateBiLinear(
            new ColorRgbMutable(c0),
            new ColorRgbMutable(c1),
            new ColorRgbMutable(c2),
            new ColorRgbMutable(c3),
            x, y);

        return new ColorRgbMutable(color);
    }

    public void scaleRadiance(float inFactor) {
        for (int i = 0; i < camera.xSize * camera.ySize; i++) {
            radiance[i] = new ColorRgbMutable(
                radiance[i].getR() * inFactor,
                radiance[i].getG() * inFactor,
                radiance[i].getB() * inFactor);
        }

        synced = false;
    }

    public void setAddScaleFactor(float inFactor) {
        addFactor = inFactor;
    }

    public void setFactor(float inFactor) {
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
