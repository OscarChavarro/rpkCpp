package vsdk.toolkit.render;

import java.io.OutputStream;

import vsdk.toolkit.common.color.ColorRgb;
import vsdk.toolkit.common.Error;
import vsdk.toolkit.common.linealAlgebra.Numeric;
import vsdk.toolkit.common.linealAlgebra.Vector2D;
import vsdk.toolkit.common.linealAlgebra.Vector3D;
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
    private ColorRgb[] radiance;
    private ColorRgb[] rgbColor;
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

        target.eyePosition.copy(source.eyePosition);
        target.lookPosition.copy(source.lookPosition);
        target.upDirection.copy(source.upDirection);
        target.viewDistance = source.viewDistance;
        target.fieldOfVision = source.fieldOfVision;
        target.horizontalFov = source.horizontalFov;
        target.verticalFov = source.verticalFov;
        target.near = source.near;
        target.far = source.far;
        target.xSize = source.xSize;
        target.ySize = source.ySize;
        target.X.copy(source.X);
        target.Y.copy(source.Y);
        target.Z.copy(source.Z);
        target.background.set(source.background.r, source.background.g, source.background.b);
        target.changed = source.changed;
        target.pixelWidth = source.pixelWidth;
        target.pixelHeight = source.pixelHeight;
        target.pixelWidthTangent = source.pixelWidthTangent;
        target.pixelHeightTangent = source.pixelHeightTangent;
        for (int i = 0; i < Camera.NUMBER_OF_VIEW_PLANES; i++) {
            target.viewPlanes[i].normal.copy(source.viewPlanes[i].normal);
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
            Error.fatal(-1, "ScreenBuffer::init", "Camera not set");
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
                radiance[i].clear();
                rgbColor[i].clear();
            }
        }

        // Clear
        ColorRgb black = new ColorRgb(0.0, 0.0, 0.0);
        for (int i = 0; i < camera.xSize * camera.ySize; i++) {
            radiance[i].setMonochrome(0.0f);
            rgbColor[i].set(black.r, black.g, black.b);
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
            radiance[i].set(source.radiance[i].r, source.radiance[i].g, source.radiance[i].b);
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
            Error.error("ScreenBuffer::merge", "Incompatible screen buffer sources");
            return;
        }

        int N = getVRes() * getHRes();

        for (int i = 0; i < N; i++) {
            radiance[i].add(src1.radiance[i], src2.radiance[i]);
        }
    }

    public void add(int x, int y, ColorRgb inRadiance) {
        int index = x + (camera.ySize - y - 1) * camera.xSize;

        radiance[index].addScaled(radiance[index], addFactor, inRadiance);
        synced = false;
    }

    public void set(int x, int y, ColorRgb inRadiance) {
        int index = x + (camera.ySize - y - 1) * camera.xSize;
        radiance[index].scaledCopy(addFactor, inRadiance);
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
        ip.gamma[0] = activeToneMapOptions.gamma.r; // For default radiance -> display RGB
        ip.gamma[1] = activeToneMapOptions.gamma.g;
        ip.gamma[2] = activeToneMapOptions.gamma.b;
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
                float[] rgbFloatArray = new float[3 * camera.xSize];
                int rowStart = i * camera.xSize;
                for (int j = 0; j < camera.xSize; j++) {
                    ColorRgb color = radiance[rowStart + j];
                    int base = 3 * j;
                    rgbFloatArray[base] = color.r;
                    rgbFloatArray[base + 1] = color.g;
                    rgbFloatArray[base + 2] = color.b;
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
            tmpRad.scaledCopy(factor, radiance[i]);
            if (!isRgbImage()) {
                ToneMap.radianceToRgb(tmpRad, rgbColor[i], activeToneMapOptions);
            }
            else {
                tmpRad.set(rgbColor[i].r, rgbColor[i].g, rgbColor[i].b);
            }
        }

        synced = true;
    }

    protected void syncLine(int lineNumber) {
        ColorRgb tmpRad = new ColorRgb();
        ToneMappingContext activeToneMapOptions = requireToneMappingContext();

        for (int i = 0; i < camera.xSize; i++) {
            tmpRad.scaledCopy(factor, radiance[lineNumber * camera.xSize + i]);
            if (!isRgbImage()) {
                ToneMap.radianceToRgb(tmpRad, rgbColor[lineNumber * camera.xSize + i], activeToneMapOptions);
            }
            else {
                tmpRad = rgbColor[lineNumber * camera.xSize + i];
            }
        }
    }

    protected ToneMappingContext requireToneMappingContext() {
        if (toneMapOptions == null) {
            Error.fatal(-1, "ScreenBuffer::requireToneMappingContext", "Tone mapping context not set");
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

    public Vector2D getPixelPoint(int nx, int ny, float xOffset, float yOffset) {
        return new Vector2D(
            getScreenXMin() + ((float)nx + xOffset) * getPixXSize(),
            getScreenYMin() + ((float)ny + yOffset) * getPixYSize());
    }

    public Vector2D getPixelPoint(int nx, int ny) {
        return getPixelPoint(nx, ny, 0.5f, 0.5f);
    }

    public Vector2D getPixelCenter(int nx, int ny) {
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
    public Vector3D getPixelVector(int nx, int ny, float xOffset, float yOffset) {
        Vector2D pix = getPixelPoint(nx, ny, xOffset, yOffset);
        Vector3D dir = new Vector3D();
        dir.combine3(camera.Z, pix.x, camera.X, pix.y, camera.Y);
        return dir;
    }

    public Vector3D getPixelVector(int nx, int ny) {
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
        Vector3D dir = new Vector3D();
        double h = camera.pixelWidth;
        double v = camera.pixelHeight;

        double x = -h * camera.xSize / 2.0 + pixX * h;
        double y = -v * camera.ySize / 2.0 + pixY * v;

        double xSample = x + h * 0.5;  // (pixX, pixY) indicate upper left
        double ySample = y + v * 0.5;

        dir.combine3(camera.Z, (float)xSample, camera.X, (float)ySample, camera.Y);
        double distPixel2 = dir.norm2();
        double distPixel = Math.sqrt(distPixel2);
        dir.inverseScaledCopy((float)distPixel, dir, Numeric.EPSILON_FLOAT);

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

    public ColorRgb getBiLinear(float x, float y) {
        int[] nx0v = new int[1];
        int[] ny0v = new int[1];
        int nx1;
        int ny1;
        Vector2D center;
        ColorRgb color = new ColorRgb();

        getPixel(x, y, nx0v, ny0v);
        int nx0 = nx0v[0];
        int ny0 = ny0v[0];
        center = getPixelCenter(nx0, ny0);

        x = (x - center.x) / getPixXSize();
        y = (y - center.y) / getPixYSize();

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

        color.interpolateBiLinear(c0, c1, c2, c3, x, y);

        return color;
    }

    public void scaleRadiance(float inFactor) {
        for (int i = 0; i < camera.xSize * camera.ySize; i++) {
            radiance[i].scale(inFactor);
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
