package vsdk.toolkit.raycasting.stochasticRaytracing;

import java.util.ArrayList;
import java.util.Random;

import vsdk.toolkit.common.color.ColorRgb;
import vsdk.toolkit.common.logging.Logger;
import vsdk.toolkit.common.StratifiedSampling2D;
import vsdk.toolkit.common.linealAlgebra.Numeric;
import vsdk.toolkit.common.linealAlgebra.Vector3D;
import vsdk.toolkit.io.image.ImageOutputHandle;
import vsdk.toolkit.material.BsdfComponent;
import vsdk.toolkit.material.PhongEmittanceDistributionFunction;
import vsdk.toolkit.raycasting.bidirectionalRaytracing.LightList;
import vsdk.toolkit.raycasting.common.PathRayType;
import vsdk.toolkit.raycasting.common.RayTools;
import vsdk.toolkit.raycasting.common.RayTracer;
import vsdk.toolkit.raycasting.common.SimpleRaytracingPathNode;
import vsdk.toolkit.raycasting.photonMap.PhotonMapRadianceMethod;
import vsdk.toolkit.raycasting.raytracing.NextEventSampler;
import vsdk.toolkit.raycasting.raytracing.PixelSampler;
import vsdk.toolkit.raycasting.raytracing.SampleConnectionFlags;
import vsdk.toolkit.raycasting.raytracing.SamplerConfig;
import vsdk.toolkit.raycasting.raytracing.ScreenIterate;
import vsdk.toolkit.render.ScreenBuffer;
import vsdk.toolkit.scene.Background;
import vsdk.toolkit.scene.Camera;
import vsdk.toolkit.scene.RadianceMethod;
import vsdk.toolkit.scene.RadianceMethodAlgorithm;
import vsdk.toolkit.scene.Scene;
import vsdk.toolkit.scene.VoxelGrid;
import vsdk.toolkit.skin.Patch;
import vsdk.toolkit.tonemap.ToneMappingContext;

public final class StochasticRaytracer extends RayTracer {
    private static final float PHOTON_MAP_MIN_DIST = 0.02f;
    private static final float PHOTON_MAP_MIN_DIST2 = PHOTON_MAP_MIN_DIST * PHOTON_MAP_MIN_DIST;
    private static String name = "Stochastic Raytracing & Final Gathers";
    private LightList lightList;
    private StochasticRayTracingState rayTracingState;
    private static Random random48 = new Random();

    public StochasticRaytracer(
        LightList inLightList,
        StochasticRayTracingState inRayTracingState)
    {
        lightList = inLightList;
        rayTracingState = inRayTracingState;
    }

    @Override
    public void defaults() {
        // Defaults are owned by the caller-provided StochasticRayTracingState instance.
    }

    @Override
    public String getName() {
        return name;
    }

    @Override
    public void initialize(ArrayList<Patch> lightPatches) {
    }

    /**
Raytrace the current scene as seen with the current camera. If fp
is not a nullptr pointer, write the ray-traced image to the file
pointed to by 'fp'
*/
    @Override
    public void execute(
        ImageOutputHandle ip,
        Scene scene,
        RadianceMethod radianceMethod,
        ToneMappingContext toneMapOptions,
        vsdk.toolkit.common.RenderOptions renderOptions)
    {
        if ( toneMapOptions == null ) {
            Logger.fatal(-1, "StochasticRaytracer::execute", "Tone mapping context not provided");
        }

        StochasticRaytracingConfiguration config = new StochasticRaytracingConfiguration(
            scene.camera,
            rayTracingState,
            scene.lightSourcePatchList,
            radianceMethod,
            toneMapOptions,
            lightList); // config filled in by constructor
        StochasticRaytracerCallbackData callbackData = new StochasticRaytracerCallbackData();
        callbackData.config = config;
        callbackData.radianceMethod = radianceMethod;
        callbackData.renderOptions = renderOptions;

        // Frame Coherent sampling : init fixed seed
        if ( rayTracingState.doFrameCoherent != 0 ) {
            srand48(rayTracingState.baseSeed);
        }

        if ( rayTracingState.progressiveTracing == 0 ) {
            ScreenIterate.sequential(
                scene.camera,
                scene.voxelGrid,
                scene.background,
                StochasticRaytracer::calcPixel,
                callbackData,
                toneMapOptions);
        } else {
            ScreenIterate.progressive(
                scene.camera,
                scene.voxelGrid,
                scene.background,
                StochasticRaytracer::calcPixel,
                callbackData,
                toneMapOptions);
        }

        config.screen.render();

        if ( ip != null ) {
            config.screen.writeFile(ip);
        }

        rayTracingState.lastScreen = config.screen;
        config.screen = null;
        lightList = config.rayTracingLightList;
        config.release();
    }

    @Override
    public boolean saveImage(ImageOutputHandle imageOutputHandle) {
        if ( imageOutputHandle != null && rayTracingState.lastScreen != null ) {
            rayTracingState.lastScreen.sync();
            rayTracingState.lastScreen.writeFile(imageOutputHandle);
            return true;
        } else {
            return false;
        }
    }

    @Override
    public void terminate() {
        rayTracingState.lastScreen = null;
        lightList = null;
    }

    private static ColorRgb stochasticRaytracerGetScatteredRadiance(
        Camera camera,
        VoxelGrid sceneVoxelGrid,
        Background sceneBackground,
        SimpleRaytracingPathNode thisNode,
        StochasticRaytracingConfiguration config,
        StorageReadout readout,
        RadianceMethod radianceMethod,
        vsdk.toolkit.common.RenderOptions renderOptions)
    {
        int siCurrent; // What scatter block are we handling
        ScatterInfo si;

        SimpleRaytracingPathNode newNode = new SimpleRaytracingPathNode();
        thisNode.attach(newNode);

        ColorRgb result = new ColorRgb();
        result.clear();

        if ( (config.samplerConfig.surfaceSampler == null) ||
            (thisNode.m_depth >= config.samplerConfig.maxDepth) ) {
            // No scattering
            return result;
        }

        if ( (config.siStorage.flags != 0) &&
            (readout == StorageReadout.SCATTER) ) {
            // Do storage components
            si = config.siStorage;
            siCurrent = -1;
        } else {
            // No direct light using storage components
            si = config.siOthers[0];
            siCurrent = 0;
        }

        while ( siCurrent < config.siOthersCount ) {
            int numberOfSamples;

            if ( si.DoneSomePreviousBounce(thisNode) ) {
                numberOfSamples = si.nrSamplesAfter;
            } else {
                numberOfSamples = si.nrSamplesBefore;
            } // First bounce of this kind

            // A small optimisation to prevent sampling surface that
            // don't have this scattering component.

            if ( numberOfSamples > 2 ) {
                // Some bigger value may be more efficient
                ColorRgb albedo = new ColorRgb();
                albedo.clear();
                if ( thisNode.m_useBsdf != null ) {
                    albedo = thisNode.m_useBsdf.splitBsdfScatteredPower(thisNode.m_hit, si.flags);
                }
                if ( albedo.average() < Numeric.EPSILON ) {
                    // Skip, no contribution anyway
                    numberOfSamples = 0;
                }
            }

            // Do we need to compute scattered radiance at all...
            if ((numberOfSamples > 0) && (thisNode.m_depth + 1 < config.samplerConfig.maxDepth) ) {
                double[] x1 = new double[1];
                double[] x2 = new double[1];
                double factor;
                StratifiedSampling2D stratified = new StratifiedSampling2D(numberOfSamples);
                ColorRgb radiance;
                boolean doRR = thisNode.m_depth >= config.samplerConfig.minDepth;

                for ( int i = 0; i < numberOfSamples; i++ ) {
                    stratified.sample(x1, x2);

                    // Surface sampling
                    if ( config.samplerConfig.surfaceSampler.sample(
                            camera,
                            sceneVoxelGrid,
                            sceneBackground,
                            thisNode.previous(),
                            thisNode,
                            newNode, x1[0], x2[0],
                            doRR,
                            si.flags)
                         && ((newNode.m_rayType != PathRayType.ENVIRONMENT) || (config.backgroundIndirect)) ) {
                        if ( newNode.m_rayType != PathRayType.ENVIRONMENT ) {
                            newNode.assignBsdfAndNormal();
                        }

                        // Frame coherent & correlated sampling
                        if ( config.doFrameCoherent != 0 || config.doCorrelatedSampling != 0 ) {
                            config.seedConfig.save(newNode.m_depth);
                        }

                        // Get the incoming radiance
                        if ( siCurrent == -1 ) {
                            // Storage bounce
                            radiance = stochasticRaytracerGetRadiance(
                                camera,
                                sceneVoxelGrid,
                                sceneBackground,
                                newNode,
                                config,
                                StorageReadout.READ_NOW,
                                numberOfSamples,
                                radianceMethod,
                                renderOptions);
                        } else {
                            radiance = stochasticRaytracerGetRadiance(
                                camera,
                                sceneVoxelGrid,
                                sceneBackground,
                                newNode,
                                config,
                                readout,
                                numberOfSamples,
                                radianceMethod,
                                renderOptions);
                        }

                        // Frame coherent & correlated sampling
                        if ( config.doFrameCoherent != 0 || config.doCorrelatedSampling != 0 ) {
                            config.seedConfig.Restore(newNode.m_depth);
                        }

                        // Collect outgoing radiance
                        factor = newNode.m_G / (newNode.m_pdfFromPrev * numberOfSamples);

                        radiance.scalarProductScaled(radiance, (float)factor, thisNode.m_bsdfEval);
                        result.add(radiance, result);
                    }
                }
            }

            // Next scatter info block
            siCurrent++;
            if ( siCurrent < config.siOthersCount ) {
                si = config.siOthers[siCurrent];
            }
        }

        thisNode.setNext(null);
        return result;
    }

    private static ColorRgb srGetDirectRadiance(
        Camera camera,
        VoxelGrid sceneVoxelGrid,
        Background sceneBackground,
        SimpleRaytracingPathNode prevNode,
        StochasticRaytracingConfiguration config,
        StorageReadout readout)
    {
        ColorRgb result = new ColorRgb();
        ColorRgb radiance = new ColorRgb();
        result.clear();
        Vector3D dirEL = new Vector3D();

        if ( readout == StorageReadout.READ_NOW && config.radMode == RayTracingRadMode.STORED_PHOTON_MAP ) {
            // We're reading out D|G, specular not with direct light
            return result;
        }

        NextEventSampler nes = config.samplerConfig.neSampler;
        int bsdfAll = BsdfComponent.BRDF_DIFFUSE_COMPONENT
            | BsdfComponent.BRDF_GLOSSY_COMPONENT
            | BsdfComponent.BRDF_SPECULAR_COMPONENT
            | BsdfComponent.BTDF_DIFFUSE_COMPONENT
            | BsdfComponent.BTDF_GLOSSY_COMPONENT
            | BsdfComponent.BTDF_SPECULAR_COMPONENT;
        int bsdfSpec = BsdfComponent.BRDF_SPECULAR_COMPONENT | BsdfComponent.BTDF_SPECULAR_COMPONENT;

        // Check if N.E.E. can give a contribution. I.e. not inside
        // a medium or just about to leave to vacuum
        if ( (nes != null) &&
            (config.nextEventSamples > 0) &&
            (prevNode.m_depth + 1 < config.samplerConfig.maxDepth) ) {
            SimpleRaytracingPathNode lightNode = new SimpleRaytracingPathNode();
            double[] x1 = new double[1];
            double[] x2 = new double[1];
            double geom;
            double weight;
            double cl;
            double cr;
            double factor;
            double nrs;
            boolean lightsToDo = true;

            if ( config.lightMode == RayTracingLightMode.ALL_LIGHTS ) {
                lightsToDo = nes.ActivateFirstUnit();
            }

            while ( lightsToDo ) {
                StratifiedSampling2D stratified = new StratifiedSampling2D(config.nextEventSamples);

                for ( int i = 0; i < config.nextEventSamples; i++ ) {
                    // Light sampling
                    stratified.sample(x1, x2);

                    if ( config.samplerConfig.neSampler.sample(
                        camera,
                        sceneVoxelGrid,
                        sceneBackground,
                        prevNode.previous(),
                        prevNode,
                        lightNode,
                        x1[0],
                        x2[0],
                        true,
                        (byte)bsdfAll)
                        && ( RayTools.pathNodesVisible(sceneVoxelGrid, prevNode, lightNode) ) ) {
                        // Now connect for all applicable scatter-info's
                        int siCurrent;
                        ScatterInfo si;

                        if ( (config.siStorage.flags != 0) && (readout == StorageReadout.SCATTER) ) {
                            // Do storage components
                            si = config.siStorage;
                            siCurrent = -1;
                        } else {
                            // No direct light using storage components
                            si = config.siOthers[0];
                            siCurrent = 0;
                        }

                        while ( siCurrent < config.siOthersCount ) {
                            boolean doSi = true;

                            if ( ((config.reflectionSampling == RayTracingSamplingMode.PHOTON_MAP_SAMPLING)
                                || (config.reflectionSampling == RayTracingSamplingMode.CLASSICAL_SAMPLING))
                                && ( (si.flags & bsdfSpec) != 0 ) ) {
                                // Perfect mirror reflection, no n.e.e.
                                doSi = false;
                            }

                            if ( doSi ) {
                                // Connect using correct flags
                                geom = SamplerConfig.pathNodeConnect(
                                    camera,
                                    prevNode,
                                    lightNode,
                                    config.samplerConfig,
                                    null, // No light config
                                    SampleConnectionFlags.CONNECT_EL,
                                    si.flags,
                                    (byte)bsdfAll,
                                    dirEL);

                                // Contribution of this sample (with Multiple Imp. S.)

                                if ( config.reflectionSampling == RayTracingSamplingMode.CLASSICAL_SAMPLING ) {
                                    weight = 1.0;
                                } else {
                                    // N direct * pdf  for the n.e.e.
                                    cl = SimpleRaytracingPathNode.multipleImportanceSampling(config.nextEventSamples * lightNode.m_pdfFromPrev);

                                    // N scatter * pdf  for possible scattering
                                    if ( si.DoneSomePreviousBounce(prevNode) ) {
                                        nrs = si.nrSamplesAfter;
                                    } else {
                                        nrs = si.nrSamplesBefore;
                                    }

                                    cr = SimpleRaytracingPathNode.multipleImportanceSampling(nrs * lightNode.m_pdfFromNext);

                                    // Are we deep enough to do russian roulette
                                    if ( lightNode.m_depth >= config.samplerConfig.minDepth ) {
                                        cr *= SimpleRaytracingPathNode.multipleImportanceSampling(lightNode.m_rrPdfFromNext);
                                    }

                                    weight = cl / (cl + cr);
                                }

                                factor = weight * geom / (lightNode.m_pdfFromPrev *
                                                          config.nextEventSamples);
                                radiance.scalarProductScaled(prevNode.m_bsdfEval, (float)factor, lightNode.m_bsdfEval);

                                // Collect outgoing radiance
                                result.add(result, radiance);
                            } // if not photon map or no caustic path

                            // Next scatter info block
                            siCurrent++;
                            if ( siCurrent < config.siOthersCount ) {
                                si = config.siOthers[siCurrent];
                            }
                        }
                    }
                }

                if ( config.lightMode == RayTracingLightMode.ALL_LIGHTS ) {
                    lightsToDo = nes.ActivateNextUnit();
                } else {
                    lightsToDo = false;
                }
            }
        }
        return result;
    }

    private static ColorRgb stochasticRaytracerGetRadiance(
        Camera camera,
        VoxelGrid sceneVoxelGrid,
        Background sceneBackground,
        SimpleRaytracingPathNode thisNode,
        StochasticRaytracingConfiguration config,
        StorageReadout readout,
        int usedScatterSamples,
        RadianceMethod radianceMethod,
        vsdk.toolkit.common.RenderOptions renderOptions)
    {
        ColorRgb result = new ColorRgb();
        ColorRgb radiance = new ColorRgb();
        int edfFlags = 0x01 | 0x02 | 0x04;
        int bsdfSpec = BsdfComponent.BRDF_SPECULAR_COMPONENT | BsdfComponent.BTDF_SPECULAR_COMPONENT;

        // Handle background
        if ( thisNode.m_rayType == PathRayType.ENVIRONMENT ) {
            // Check for  weighting
            double weight = 1;
            double cr;
            double cl;
            boolean doWeight = true;

            if ( thisNode.m_depth <= 1 ) {   // do not weight direct light
                doWeight = false;
            }

            if ( config.reflectionSampling == RayTracingSamplingMode.CLASSICAL_SAMPLING ) {
                doWeight = false;
            }

            if ( !config.backgroundSampling ) {
                doWeight = false;
            }

            if ( doWeight ) {
                cl = config.nextEventSamples *
                     config.samplerConfig.neSampler.evalPDF(camera, thisNode.previous(), thisNode);
                cl = SimpleRaytracingPathNode.multipleImportanceSampling(cl);
                cr = usedScatterSamples * thisNode.m_pdfFromPrev;
                cr = SimpleRaytracingPathNode.multipleImportanceSampling(cr);
                weight = cr / (cr + cl);
            }

            Vector3D position = thisNode.previous().m_hit.getPoint();
            result = Background.backgroundRadiance(sceneBackground, position, thisNode.m_inDirF, null);

            result.scale((float)weight);
        } else {
            // Handle non-background
            PhongEmittanceDistributionFunction thisEdf = thisNode.m_hit.getMaterial().getEdf();

            result.clear();

            // Stored radiance
            if ( (readout == StorageReadout.READ_NOW) && (config.siStorage.flags != 0) ) {
                // Add the stored radiance being emitted from the patch
                if ( radianceMethod.className == RadianceMethodAlgorithm.PHOTON_MAP ) {
                    PhotonMapRadianceMethod photonMapMethod = (PhotonMapRadianceMethod)radianceMethod;
                    if ( config.radMode == RayTracingRadMode.STORED_PHOTON_MAP ) {
                        // Check if the distance to the previous point is big enough
                        // otherwise we need more scattering...
                        float dist2 = thisNode.m_hit.getPoint().distance2(thisNode.previous().m_hit.getPoint());

                        if ( dist2 > PHOTON_MAP_MIN_DIST2 ) {
                            radiance = photonMapMethod.getNodeGRadiance(thisNode);
                            // This does not include Le (self emitted light)
                        } else {
                            radiance.clear();
                            readout = StorageReadout.SCATTER; // This ensures extra scattering, direct light and c-map
                        }
                    } else {
                        radiance = photonMapMethod.getNodeGRadiance(thisNode);
                        // This does not include Le (self emitted light)
                    }
                } else {
                    // Other radiosity method
                    double[] u = new double[1];
                    double[] v = new double[1];

                    // (u, v) coordinates of intersection point
                    Vector3D position = thisNode.m_hit.getPoint();
                    thisNode.m_hit.getPatch().uv(position, u, v);

                    radiance = radianceMethod.getRadiance(
                        camera, thisNode.m_hit.getPatch(), u[0], v[0], thisNode.m_inDirF, renderOptions);

                    // This includes Le diffuse, subtraction first and handle total emitted later (possibly weighted)
                    ColorRgb diffEmit;

                    if ( thisEdf == null ) {
                        diffEmit = new ColorRgb();
                        diffEmit.clear();
                    } else {
                        diffEmit = thisEdf.phongEdfEval(
                            thisNode.m_hit, thisNode.m_inDirF, BsdfComponent.BRDF_DIFFUSE_COMPONENT, null);
                    }

                    radiance.subtract(radiance, diffEmit);
                }

                result.add(result, radiance);

            } // Done: Stored radiance, no self emitted light included!

            // Stored caustic maps
            if ( (config.radMode == RayTracingRadMode.STORED_PHOTON_MAP) && readout == StorageReadout.SCATTER ) {
                PhotonMapRadianceMethod photonMapMethod = (PhotonMapRadianceMethod)radianceMethod;
                radiance = photonMapMethod.getNodeCRadiance(thisNode);
                result.add(result, radiance);
            }

            radiance = srGetDirectRadiance(camera, sceneVoxelGrid, sceneBackground, thisNode, config, readout);
            result.add(result, radiance);

            // Scattered light
            radiance = stochasticRaytracerGetScatteredRadiance(
                camera,
                sceneVoxelGrid,
                sceneBackground,
                thisNode,
                config,
                readout,
                radianceMethod,
                renderOptions);
            result.add(result, radiance);

            // Emitted Light
            if ( config.radMode == RayTracingRadMode.STORED_PHOTON_MAP
                && radianceMethod.className == RadianceMethodAlgorithm.PHOTON_MAP
                && (readout == StorageReadout.READ_NOW)
                && !(config.siStorage.DoneThisBounce(thisNode.previous())) ) {
                // Check if Le would contribute to a caustic
                // Caustic contribution: (E...(D|G)...?L) with ? some specular bounce
                edfFlags = 0;
            }

            if ( (thisEdf != null) && (edfFlags != 0) ) {
                double weight;
                double cr;
                double cl;
                ColorRgb col;
                boolean doWeight = true;

                if ( thisNode.m_depth <= 1 ) {
                    doWeight = false;
                }

                if ( config.reflectionSampling == RayTracingSamplingMode.CLASSICAL_SAMPLING ) {
                    doWeight = false;
                }

                if ( config.reflectionSampling == RayTracingSamplingMode.PHOTON_MAP_SAMPLING
                  && thisNode.m_depth > 1
                  && ( (thisNode.previous().m_usedComponents & bsdfSpec) != 0 ) ) {
                    // Perfect Specular scatter, no weighting
                    doWeight = false;
                }

                if ( doWeight ) {
                    cl = config.nextEventSamples *
                         config.samplerConfig.neSampler.evalPDF(camera, thisNode.previous(), thisNode);
                    cl = SimpleRaytracingPathNode.multipleImportanceSampling(cl);
                    cr = usedScatterSamples * thisNode.m_pdfFromPrev;
                    cr = SimpleRaytracingPathNode.multipleImportanceSampling(cr);

                    weight = cr / (cr + cl);
                } else {
                    // We do not do N.E.E. from the eye !
                    weight = 1;
                }

                if ( thisEdf == null ) {
                    col = new ColorRgb();
                    col.clear();
                } else {
                    col = thisEdf.phongEdfEval(thisNode.m_hit, thisNode.m_inDirF, edfFlags, null);
                }

                result.addScaled(result, (float)weight, col);
            }
        }

        return result;
    }

    private static ColorRgb calcPixel(
        Camera camera,
        VoxelGrid sceneVoxelGrid,
        Background sceneBackground,
        int nx,
        int ny,
        Object data)
    {
        StochasticRaytracerCallbackData callbackData = (StochasticRaytracerCallbackData)data;
        StochasticRaytracingConfiguration config = callbackData.config;
        RadianceMethod radianceMethod = callbackData.radianceMethod;
        vsdk.toolkit.common.RenderOptions renderOptions = callbackData.renderOptions;
        SimpleRaytracingPathNode eyeNode = new SimpleRaytracingPathNode();
        SimpleRaytracingPathNode pixelNode = new SimpleRaytracingPathNode();
        double[] x1 = new double[1];
        double[] x2 = new double[1];
        ColorRgb col;
        ColorRgb result = new ColorRgb();
        StratifiedSampling2D stratified = new StratifiedSampling2D(config.samplesPerPixel);

        result.clear();

        // Frame coherent & correlated sampling
        if ( config.doFrameCoherent != 0 || config.doCorrelatedSampling != 0 ) {
            if ( config.doCorrelatedSampling != 0 ) {
                // Correlated : start each pixel with same seed
                srand48(config.baseSeed);
            }
            drand48(); // (randomize seed, gives new seed for uncorrelated sampling)
            config.seedConfig.save(0);
        }

        // Calc pixel data

        // Sample eye node
        config.samplerConfig.pointSampler.sample(camera, sceneVoxelGrid, sceneBackground, null, null, eyeNode, 0, 0);
        ((PixelSampler)config.samplerConfig.dirSampler).SetPixel(camera, nx, ny, null);

        eyeNode.attach(pixelNode);

        // Stratified sampling of the pixel
        for ( int i = 0; i < config.samplesPerPixel; i++ ) {
            stratified.sample(x1, x2);

            if ( config.samplerConfig.dirSampler.sample(camera, sceneVoxelGrid, sceneBackground, null, eyeNode, pixelNode, x1[0], x2[0])
                 && ((pixelNode.m_rayType != PathRayType.ENVIRONMENT) || (config.backgroundDirect)) ) {
                pixelNode.assignBsdfAndNormal();

                // Frame coherent & correlated sampling
                if ( config.doFrameCoherent != 0 || config.doCorrelatedSampling != 0 ) {
                    config.seedConfig.save(pixelNode.m_depth);
                }

                col = stochasticRaytracerGetRadiance(
                    camera,
                    sceneVoxelGrid,
                    sceneBackground,
                    pixelNode,
                    config,
                    config.initialReadout,
                    config.samplesPerPixel,
                    radianceMethod,
                    renderOptions);

                // Frame coherent & correlated sampling
                if ( config.doFrameCoherent != 0 || config.doCorrelatedSampling != 0 ) {
                    config.seedConfig.Restore(pixelNode.m_depth);
                }

                // col represents the radiance reflected towards the eye
                // in the pixel sampled point.

                // Account for pixel sampling
                col.scale((float)(pixelNode.m_G / pixelNode.m_pdfFromPrev));
                result.add(result, col);
            }
        }

        // We have now the FLUX for the pixel (x N), convert it to radiance
        double factor = (ScreenBuffer.computeFluxToRadFactor(camera, nx, ny) / (float)config.samplesPerPixel);

        result.scale((float)factor);
        config.screen.add(nx, ny, result);

        // Frame coherent & correlated sampling
        if ( config.doFrameCoherent != 0 || config.doCorrelatedSampling != 0 ) {
            config.seedConfig.Restore(0);
        }

        return result;
    }

    private static void srand48(long seed) {
        random48 = new Random(seed);
    }

    private static double drand48() {
        return random48.nextDouble();
    }
}
