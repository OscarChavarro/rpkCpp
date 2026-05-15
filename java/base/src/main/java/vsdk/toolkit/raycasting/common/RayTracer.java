package vsdk.toolkit.raycasting.common;

import java.io.OutputStream;
import java.util.ArrayList;

import vsdk.toolkit.material.RenderOptions;
import vsdk.toolkit.io.image.ImageOutputHandle;
import vsdk.toolkit.scene.RadianceMethod;
import vsdk.toolkit.scene.Scene;
import vsdk.toolkit.environment.geometry.elements.Patch;
import vsdk.toolkit.tonemap.ToneMappingContext;

/**
TODO: This should be converted on to the Raytracer interface for inheriting the current four
ray-tracers: RayMatter, RayCaster, BidirectionalPathRaytracer and StochasticRaytracer.
*/
public abstract class RayTracer {
    public abstract void defaults();
    public abstract String getName();

    // Initializes the current scene for raytracing computations.
    // Called when a new scene is loaded or when selecting a particular
    // raytracing algorithm
    public abstract void initialize(ArrayList<Patch> lightPatches);

    // Raytrace the current scene as seen with the current camera. If 'ip'
    // is not a nullptr pointer, write the ray-traced image using the image output
    // handle pointed by 'ip'
    public abstract void
    execute(
        ImageOutputHandle ip,
        Scene scene,
        RadianceMethod radianceMethod,
        ToneMappingContext toneMapOptions,
        RenderOptions renderOptions);

    // Saves last ray-traced image in the file describe dby the image output handle
    public abstract boolean saveImage(ImageOutputHandle imageOutputHandle);

    // Terminate raytracing computations
    public abstract void terminate();

    /**
Initializes an ImageOutputHandle taking into account the image filename extension,
and performs raytracing
*/
    public static void
    rayTrace(
        String fileName,
        OutputStream stream,
        int isPipe,
        RayTracer rayTracer,
        Scene scene,
        RadianceMethod radianceMethod,
        ToneMappingContext toneMapOptions,
        RenderOptions renderOptions)
    {
        ImageOutputHandle img = null;

        if (stream != null) {
            img = ImageOutputHandle.createRadianceImageOutputHandle(
                fileName,
                stream,
                isPipe,
                scene.camera.xSize,
                scene.camera.ySize);
            if (img == null) {
                return;
            }
        }

        if (rayTracer != null) {
            rayTracer.execute(img, scene, radianceMethod, toneMapOptions, renderOptions);
        }

        if (img != null) {
            ImageOutputHandle.deleteImageOutputHandle(img);
        }
    }
}
