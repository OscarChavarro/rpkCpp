package vsdk.toolkit.raycasting.bidirectionalRaytracing;

import java.util.ArrayList;
import java.util.Locale;

import vsdk.toolkit.common.color.ColorRgb;
import vsdk.toolkit.common.logging.Logger;
import vsdk.toolkit.common.RenderOptions;
import vsdk.toolkit.common.StratifiedSampling2D;
import vsdk.toolkit.common.linealAlgebra.Numeric;
import vsdk.toolkit.common.linealAlgebra.Vector2D;
import vsdk.toolkit.io.image.ImageOutputHandle;
import vsdk.toolkit.material.BsdfComponent;
import vsdk.toolkit.material.PhongEmittanceDistributionFunction;
import vsdk.toolkit.material.XxdfComponentFlag;
import vsdk.toolkit.raycasting.common.BsdfComp;
import vsdk.toolkit.raycasting.common.PathRayType;
import vsdk.toolkit.raycasting.common.RayTools;
import vsdk.toolkit.raycasting.common.RayTracer;
import vsdk.toolkit.raycasting.common.SimpleRaytracingPathNode;
import vsdk.toolkit.raycasting.raytracing.BsdfSampler;
import vsdk.toolkit.raycasting.raytracing.EyeSampler;
import vsdk.toolkit.raycasting.raytracing.PixelSampler;
import vsdk.toolkit.raycasting.raytracing.SampleConnectionFlags;
import vsdk.toolkit.raycasting.raytracing.Sampler;
import vsdk.toolkit.raycasting.raytracing.SamplerConfig;
import vsdk.toolkit.raycasting.raytracing.ScreenIterate;
import vsdk.toolkit.render.ScreenBuffer;
import vsdk.toolkit.scene.Background;
import vsdk.toolkit.scene.Camera;
import vsdk.toolkit.scene.RadianceMethod;
import vsdk.toolkit.scene.Scene;
import vsdk.toolkit.scene.VoxelGrid;
import vsdk.toolkit.skin.Patch;
import vsdk.toolkit.tonemap.ToneMappingContext;

public final class BidirectionalPathRaytracer extends RayTracer {
    private static final int STRINGS_SIZE = 300;
    private static String name = "Bidirectional Path Tracing";
    private BidirectionalPathTracingState bidirectionalPathState;
    private LightList lightList;

    private static BsdfComp copyBsdfComp(BsdfComp source) {
        BsdfComp copy = new BsdfComp();

        if ( source == null ) {
            return copy;
        }

        ColorRgb[] src = source.asArray();
        ColorRgb[] dst = copy.asArray();

        for ( int i = 0; i < src.length && i < dst.length; i++ ) {
            dst[i].set(src[i].r, src[i].g, src[i].b);
        }

        return copy;
    }

    private static void assignBsdfComp(BsdfComp destination, BsdfComp source) {
        if ( destination == null || source == null ) {
            return;
        }

        ColorRgb[] src = source.asArray();
        ColorRgb[] dst = destination.asArray();

        for ( int i = 0; i < src.length && i < dst.length; i++ ) {
            dst[i].set(src[i].r, src[i].g, src[i].b);
        }
    }

    public BidirectionalPathRaytracer(
        BidirectionalPathTracingState inBidirectionalPathState,
        LightList inLightList)
    {
        bidirectionalPathState = inBidirectionalPathState;
        lightList = inLightList;
    }

    @Override
    public void defaults() {
        // Defaults are owned by the caller-provided BidirectionalPathTracingState instance.
    }

    @Override
    public String getName() {
        return name;
    }

    @Override
    public void initialize(ArrayList<Patch> lightPatches) {
        // mainInitApplication the light list
        lightList = new LightList(lightPatches);
    }

    /**
    Raytrace the current scene as seen with the current camera. If fp
    is not a nullptr pointer, write the ray traced image to the file
    pointed to by 'fp'
    */
    @Override
    public void
    execute(
        ImageOutputHandle ip,
        Scene scene,
        RadianceMethod radianceMethod,
        ToneMappingContext toneMapOptions,
        RenderOptions renderOptions)
    {
        // Install the samplers to be used in the state
        BidirectionalPathTracingConfiguration config = new BidirectionalPathTracingConfiguration();
        config.toneMapOptions = toneMapOptions;
        if ( config.toneMapOptions == null ) {
            Logger.fatal(-1, "BidirectionalPathRaytracer::execute", "Tone mapping context not provided");
        }

        // Copy base config (so that rendering is independent of GUI)
        config.baseConfig = new BidirectionalPathRaytracerConfig();
        config.baseConfig.copyFrom(bidirectionalPathState.baseConfig);
        config.baseConfig.totalSamples =
                (long)bidirectionalPathState.baseConfig.samplesPerPixel * scene.camera.xSize * scene.camera.ySize;

        config.dBuffer = null;

        // Eye and light path sampling config
        config.eyeConfig.pointSampler = new EyeSampler();
        config.eyeConfig.dirSampler = new PixelSampler();
        config.eyeConfig.surfaceSampler = new BsdfSampler();
        config.eyeConfig.surfaceSampler.SetComputeFromNextPdf(true);
        config.eyeConfig.surfaceSampler.SetComputeBsdfComponents(bidirectionalPathState.baseConfig.useSpars != 0);

        if ( bidirectionalPathState.baseConfig.sampleImportantLights != 0 ) {
            config.eyeConfig.neSampler = new ImportantLightSampler(lightList);
        } else {
            config.eyeConfig.neSampler = new UniformLightSampler(lightList);
        }

        config.eyeConfig.minDepth = bidirectionalPathState.baseConfig.minimumPathDepth;

        if ( bidirectionalPathState.baseConfig.maximumEyePathDepth < 1 ) {
            System.err.printf("Maximum Eye Path Length too small (<1), using 1\n");
            config.eyeConfig.maxDepth = 1;
        } else {
            config.eyeConfig.maxDepth = bidirectionalPathState.baseConfig.maximumEyePathDepth;
        }

        config.lightConfig.pointSampler = new UniformLightSampler(lightList);
        config.lightConfig.dirSampler = new LightDirSampler();
        config.lightConfig.surfaceSampler = new BsdfSampler();
        config.lightConfig.surfaceSampler.SetComputeFromNextPdf(true);
        config.lightConfig.surfaceSampler.SetComputeBsdfComponents(bidirectionalPathState.baseConfig.useSpars != 0);

        config.lightConfig.minDepth = bidirectionalPathState.baseConfig.minimumPathDepth;
        config.lightConfig.maxDepth = bidirectionalPathState.baseConfig.maximumLightPathDepth;
        config.lightConfig.neSampler = null; // eyeSampler ?

        config.screen = new ScreenBuffer(null, scene.camera, config.toneMapOptions);
        config.screen.setFactor(1.0f); // We're storing plain radiance

        config.eyePath = null;
        config.lightPath = null;

        // SPaR configuration

        LeSpar leSpar = null;
        LDSpar ldSpar = null;

        if ( bidirectionalPathState.baseConfig.useSpars != 0 ) {
            SparConfig sc = config.sparConfig;

            sc.baseConfig = config.baseConfig; // Share base config options

            config.sparList = new SparList();

            leSpar = new LeSpar();
            ldSpar = new LDSpar();

            sc.leSpar = leSpar;
            sc.ldSpar = ldSpar;

            leSpar.init(sc, radianceMethod);
            ldSpar.init(sc, radianceMethod);

            config.sparList.add(leSpar);
            config.sparList.add(ldSpar);
        }

        if ( bidirectionalPathState.saveSubsequentImages != 0 ) {
            doBptAndSubsequentImages(scene.camera, scene.voxelGrid, scene.background, config);
        } else if ( config.baseConfig.doDensityEstimation != 0 ) {
            doBptDensityEstimation(scene.camera, scene.voxelGrid, scene.background, config);
        } else if ( bidirectionalPathState.baseConfig.progressiveTracing == 0 ) {
            ScreenIterate.sequential(
                scene.camera,
                scene.voxelGrid,
                scene.background,
                BidirectionalPathRaytracer::bpCalcPixel,
                config,
                config.toneMapOptions);
        } else {
            ScreenIterate.progressive(
                scene.camera,
                scene.voxelGrid,
                scene.background,
                BidirectionalPathRaytracer::bpCalcPixel,
                config,
                config.toneMapOptions);
        }

        config.screen.render();

        if ( ip != null ) {
            config.screen.writeFile(ip);
        }

        bidirectionalPathState.lastScreen = config.screen;

        if ( bidirectionalPathState.baseConfig.useSpars != 0 ) {
            config.sparList = null;
            leSpar = null;
            ldSpar = null;
        }

        config.eyeConfig.pointSampler = null;
        config.eyeConfig.dirSampler = null;
        config.eyeConfig.surfaceSampler = null;
        config.eyeConfig.neSampler = null;
        config.lightConfig.pointSampler = null;
        config.lightConfig.dirSampler = null;
        config.lightConfig.surfaceSampler = null;

        if ( config.dBuffer != null ) {
            config.dBuffer = null;
        }

        config.baseConfig = null;
    }

    @Override
    public boolean saveImage(ImageOutputHandle imageOutputHandle) {
        if ( imageOutputHandle != null && bidirectionalPathState.lastScreen != null ) {
            bidirectionalPathState.lastScreen.sync();
            bidirectionalPathState.lastScreen.writeFile(imageOutputHandle);
            return true;
        } else {
            return false;
        }
    }

    @Override
    public void terminate() {
        bidirectionalPathState.lastScreen = null;
        if ( lightList != null ) {
            lightList = null;
        }
    }

    private static boolean spikeCheck(ColorRgb color) {
        double colAvg = color.average();

        if ( colAvg > 60000 /* Aaaaaaarrrrggggghh */) {
            System.out.printf("Spike\n");
            return true;
        }

        if ( colAvg < 0 ) {
            System.out.printf("Negative");
            return true;
        }

        return false;
    }

    private static void
    addWithSpikeCheck(
        BidirectionalPathTracingConfiguration config,
        BiPath path,
        int nx,
        int ny,
        float pixX,
        float pixY,
        ColorRgb f)
    {
        addWithSpikeCheck(config, path, nx, ny, pixX, pixY, f, false);
    }

    private static void
    addWithSpikeCheck(
        BidirectionalPathTracingConfiguration config,
        BiPath path,
        int nx,
        int ny,
        float pixX,
        float pixY,
        ColorRgb f,
        boolean radSample)
    {
        if ( config.baseConfig.doDensityEstimation != 0 ) {
            ScreenBuffer rs;
            ScreenBuffer ds;
            DensityBuffer db;
            float baseSize;

            if ( radSample ) {
                rs = config.ref2;
                ds = config.dest2;
                db = config.dBuffer2;
                baseSize = 1.5f;
            } else {
                rs = config.ref;
                ds = config.dest;
                db = config.dBuffer;
                baseSize = 5.0f;
            }

            if ( config.deStoreHits && db != null ) {
                db.add(pixX, pixY, f);
            } else {
                // Now splat directly into the dest screen
                Vector2D center = new Vector2D();
                ColorRgb g = new ColorRgb();

                // Get the center:
                center.x = pixX;
                center.y = pixY;

                // Convert f into DE quantities
                float factor = rs.getPixXSize() * rs.getPixYSize()
                    * (float)config.baseConfig.totalSamples;

                if ( f.average() > Numeric.EPSILON ) {
                    g.scaledCopy(factor, f); // Undo part of flux to rad factor

                    config.kernel.varCover(center, g, rs, ds,
                        (int)config.baseConfig.totalSamples, config.scaleSamples,
                        baseSize);
                }
                return;
            }
        }

        if ( config.baseConfig.eliminateSpikes != 0 ) {
            if ( !spikeCheck(f) ) {
                config.screen.add(nx, ny, f);
            } else {
                // Wanna see the spikes !
                //  config->screen->Add(config->nx, config->ny, f);
            }
        } else {
            config.screen.add(nx, ny, f);
        }
    }

    private static void
    handlePathX0(
        Camera camera,
        Background sceneBackground,
        BidirectionalPathTracingConfiguration config,
        BiPath path)
    {
        PhongEmittanceDistributionFunction endingEdf =
            path.m_eyeEndNode.m_hit.getMaterial() != null
                ? path.m_eyeEndNode.m_hit.getMaterial().getEdf()
                : null;
        boolean endingInEnvironment = path.m_eyeEndNode.m_rayType == PathRayType.ENVIRONMENT;
        boolean hasEnvironmentBackground = endingInEnvironment && sceneBackground != null;
        ColorRgb oldBsdfEval = new ColorRgb();
        ColorRgb f;
        ColorRgb fRad = new ColorRgb();
        BsdfComp oldBsdfComp;
        float factor;
        double pdfLNE;
        double oldPdfLNE;
        double oldPDFLightEval;
        double oldPDFDirEval;
        double oldRRPDFLightEval;
        double oldRRPDFDirEval;
        float[] pdf = new float[] {1.0f};
        float[] weight = new float[] {1.0f};
        SimpleRaytracingPathNode eyePrevNode;
        SimpleRaytracingPathNode eyeEndNode;

        if ( path.m_eyeSize > config.baseConfig.maximumPathDepth ) {
            return;
        }

        if ( endingEdf != null || hasEnvironmentBackground || config.baseConfig.useSpars != 0 ) {
            eyeEndNode = path.m_eyeEndNode;

            // Store the Bsdf and PDF evaluations that will be overwritten
            // by other values needed to compute the contribution

            eyePrevNode = eyeEndNode.previous(); // Always != nullptr

            oldBsdfEval.set(eyeEndNode.m_bsdfEval.r, eyeEndNode.m_bsdfEval.g, eyeEndNode.m_bsdfEval.b);
            oldBsdfComp = copyBsdfComp(eyeEndNode.m_bsdfComp);
            oldPDFLightEval = eyeEndNode.m_pdfFromNext;

            oldPDFDirEval = eyePrevNode.m_pdfFromNext;

            oldRRPDFLightEval = eyeEndNode.m_rrPdfFromNext;
            oldRRPDFDirEval = eyePrevNode.m_rrPdfFromNext;

            oldPdfLNE = path.m_pdfLNE;

            // Fill in new values
            eyeEndNode.m_bsdfComp.Clear();

            if ( endingEdf != null ) {
                // Landed on a light : fill in values for BPT
                if ( endingEdf == null ) {
                    eyeEndNode.m_bsdfEval.clear();
                } else {
                    eyeEndNode.m_bsdfEval = endingEdf.phongEdfEval(
                        eyeEndNode.m_hit,
                        eyeEndNode.m_inDirF,
                        XxdfComponentFlag.DIFFUSE_COMPONENT | XxdfComponentFlag.GLOSSY_COMPONENT | XxdfComponentFlag.SPECULAR_COMPONENT,
                        null);
                }
                eyeEndNode.m_bsdfComp.Fill(eyeEndNode.m_bsdfEval,
                    (byte)BsdfComponent.BRDF_DIFFUSE_COMPONENT);

                if ( config.lightConfig.maxDepth > 0 ) {
                    eyeEndNode.m_pdfFromNext = config.lightConfig.pointSampler.evalPDF(camera, null, eyeEndNode);
                    eyeEndNode.m_rrPdfFromNext = 1.0; // Light point: no Russian R.
                } else {
                    // Impossible to generate light point with a light sub path
                    eyeEndNode.m_pdfFromNext = 0.0;
                    eyeEndNode.m_rrPdfFromNext = 0.0;
                }

                if ( config.lightConfig.maxDepth > 1 ) {
                    eyePrevNode.m_pdfFromNext =
                        config.lightConfig.dirSampler.evalPDF(camera, eyeEndNode, eyePrevNode);
                    eyePrevNode.m_rrPdfFromNext = 1.0;
                } else {
                    eyePrevNode.m_pdfFromNext = 0.0;
                    eyePrevNode.m_rrPdfFromNext = 0.0;
                }

                // Compute
                if ( (config.baseConfig.sampleImportantLights != 0) && (config.lightConfig.maxDepth > 0)
                    && (path.m_eyeSize > 2) ) {
                    pdfLNE = config.eyeConfig.neSampler.evalPDF(camera, eyePrevNode, eyeEndNode);
                } else {
                    pdfLNE = eyeEndNode.m_pdfFromNext; // same sampling as light path
                }

                path.m_pdfLNE = pdfLNE;
            } else if ( hasEnvironmentBackground ) {
                eyeEndNode.m_bsdfEval = Background.backgroundRadiance(
                    sceneBackground,
                    null,
                    eyeEndNode.m_inDirF,
                    null);
                eyeEndNode.m_bsdfComp.Fill(
                    eyeEndNode.m_bsdfEval,
                    (byte)BsdfComponent.BRDF_DIFFUSE_COMPONENT);

                eyeEndNode.m_pdfFromNext = 0.0;
                eyeEndNode.m_rrPdfFromNext = 0.0;

                eyePrevNode.m_pdfFromNext = 0.0;
                eyePrevNode.m_rrPdfFromNext = 0.0;

                path.m_pdfLNE = 0.0;
            }

            // Path radiance evaluation
            path.m_geomConnect = 1.0; // Fake connection for X_0 paths

            if ( config.baseConfig.useSpars != 0 ) {
                config.sparList.handlePath(config.sparConfig, path, fRad, f = new ColorRgb());
                factor = 1.0f; // pdf and weight already taken into account
            } else {
                f = path.evalRadiance();
                factor = path.evalPdfAndWeight(config.baseConfig, pdf, weight);
            }

            factor *= (float)config.fluxToRadFactor / (float)config.baseConfig.samplesPerPixel;

            if ( config.baseConfig.useSpars != 0 ) {
                fRad.scale(factor);
                addWithSpikeCheck(config, path, config.nx, config.ny,
                    config.xSample, config.ySample, fRad, true);
                f.scale(factor);
                addWithSpikeCheck(config, path, config.nx, config.ny,
                    config.xSample, config.ySample, f, false);
            } else {
                f.scale(factor);
                addWithSpikeCheck(config, path, config.nx, config.ny,
                    config.xSample, config.ySample, f);
            }

            // Restore the Brdf and PDF evaluations
            path.m_pdfLNE = oldPdfLNE;
            eyeEndNode.m_bsdfEval.set(oldBsdfEval.r, oldBsdfEval.g, oldBsdfEval.b);
            assignBsdfComp(eyeEndNode.m_bsdfComp, oldBsdfComp);
            eyeEndNode.m_pdfFromNext = oldPDFLightEval;
            eyePrevNode.m_pdfFromNext = oldPDFDirEval;
            eyeEndNode.m_rrPdfFromNext = oldRRPDFLightEval;
            eyePrevNode.m_rrPdfFromNext = oldRRPDFDirEval;
        }
    }

    private static ColorRgb
    computeNeFluxEstimate(
        Camera camera,
        BidirectionalPathTracingConfiguration config,
        BiPath path,
        float[] pPdf,
        float[] pWeight,
        ColorRgb fRad)
    {
        SimpleRaytracingPathNode eyePrevNode;
        SimpleRaytracingPathNode lightPrevNode;
        ColorRgb oldBsdfL = new ColorRgb();
        ColorRgb oldBsdfE = new ColorRgb();
        BsdfComp oldBsdfCompL;
        BsdfComp oldBsdfCompE;
        double oldPdfL;
        double oldPdfE;
        double oldRRPdfL;
        double oldRRPdfE;
        double oldPdfLP = 0.0;
        double oldPdfEP = 0.0;
        double oldRRPdfLP = 0.0;
        double oldRRPdfEP = 0.0;
        ColorRgb f;
        SimpleRaytracingPathNode eyeEndNode;
        SimpleRaytracingPathNode lightEndNode;

        // Store PDF and BSDF evaluations that will be overwritten
        eyeEndNode = path.m_eyeEndNode;
        lightEndNode = path.m_lightEndNode;
        eyePrevNode = eyeEndNode.previous();
        lightPrevNode = lightEndNode.previous();

        oldBsdfL.set(lightEndNode.m_bsdfEval.r, lightEndNode.m_bsdfEval.g, lightEndNode.m_bsdfEval.b);
        oldBsdfCompL = copyBsdfComp(lightEndNode.m_bsdfComp);

        oldBsdfE.set(eyeEndNode.m_bsdfEval.r, eyeEndNode.m_bsdfEval.g, eyeEndNode.m_bsdfEval.b);
        oldBsdfCompE = copyBsdfComp(eyeEndNode.m_bsdfComp);

        oldPdfL = lightEndNode.m_pdfFromNext;

        oldRRPdfL = lightEndNode.m_rrPdfFromNext;

        if ( lightPrevNode != null ) {
            oldPdfLP = lightPrevNode.m_pdfFromNext;
            oldRRPdfLP = lightPrevNode.m_rrPdfFromNext;
        }

        oldPdfE = eyeEndNode.m_pdfFromNext;
        oldRRPdfE = eyeEndNode.m_rrPdfFromNext;

        if ( eyePrevNode != null ) {
            oldPdfEP = eyePrevNode.m_pdfFromNext;
            oldRRPdfEP = eyePrevNode.m_rrPdfFromNext;
        }

        // Connect the sub-paths
        path.m_geomConnect =
            SamplerConfig.pathNodeConnect(camera, eyeEndNode, lightEndNode,
                config.eyeConfig, config.lightConfig,
                SampleConnectionFlags.CONNECT_EL | SampleConnectionFlags.CONNECT_LE | SampleConnectionFlags.FILL_OTHER_PDF,
                Sampler.BSDF_ALL_COMPONENTS, Sampler.BSDF_ALL_COMPONENTS, path.m_dirEL);

        path.m_dirLE.scaledCopy(-1.0f, path.m_dirEL);

        // Evaluate radiance and pdf and weight
        if ( config.baseConfig.useSpars != 0 ) {
            ColorRgb localFRad = fRad != null ? fRad : new ColorRgb();
            f = new ColorRgb();
            config.sparList.handlePath(config.sparConfig, path, localFRad, f);
            if ( fRad == null ) {
                localFRad.clear();
            }
        } else {
            f = path.evalRadiance();

            float factor = path.evalPdfAndWeight(config.baseConfig, pPdf, pWeight);
            f.scale(factor); // Flux estimate
        }

        // Restore old values
        lightEndNode.m_bsdfEval.set(oldBsdfL.r, oldBsdfL.g, oldBsdfL.b);
        assignBsdfComp(lightEndNode.m_bsdfComp, oldBsdfCompL);

        eyeEndNode.m_bsdfEval.set(oldBsdfE.r, oldBsdfE.g, oldBsdfE.b);
        assignBsdfComp(eyeEndNode.m_bsdfComp, oldBsdfCompE);

        lightEndNode.m_pdfFromNext = oldPdfL;
        lightEndNode.m_rrPdfFromNext = oldRRPdfL;

        if ( lightPrevNode != null ) {
            lightPrevNode.m_pdfFromNext = oldPdfLP;
            lightPrevNode.m_rrPdfFromNext = oldRRPdfLP;
        }

        eyeEndNode.m_pdfFromNext = oldPdfE;
        eyeEndNode.m_rrPdfFromNext = oldRRPdfE;

        if ( eyePrevNode != null ) {
            eyePrevNode.m_pdfFromNext = oldPdfEP;
            eyePrevNode.m_rrPdfFromNext = oldRRPdfEP;
        }

        return f;
    }

    /**
    handlePathXx : handle a path with eyeSize >= 2 and
    light size >= 1
    */
    private static void
    handlePathXx(
        Camera camera,
        VoxelGrid sceneWorldVoxelGrid,
        Background sceneBackground,
        BidirectionalPathTracingConfiguration config,
        BiPath path)
    {
        ColorRgb f;
        ColorRgb fRad = new ColorRgb();
        double oldPdfLNE = 0.0;
        float[] pdf = new float[1];
        float[] weight = new float[1];
        boolean doLNE;
        SimpleRaytracingPathNode newLightNode = new SimpleRaytracingPathNode();
        SimpleRaytracingPathNode oldLightPath = null;
        SimpleRaytracingPathNode oldLightEndNode = null;
        int oldLightSize = 0;

        if ( (path.m_eyeSize + path.m_lightSize) > config.baseConfig.maximumPathDepth ) {
            return;
        }

        // Check if we need to sample another light point with
        // importance sampling.

        doLNE = (path.m_lightSize == 1) && config.baseConfig.sampleImportantLights != 0;

        if ( doLNE ) {
            // Replace the current light path with new importance sampled path
            oldLightPath = path.m_lightPath;
            oldLightSize = path.m_lightSize;
            oldLightEndNode = path.m_lightEndNode;

            path.m_lightPath = newLightNode;

            newLightNode.m_pdfFromPrev = 0.0;
            newLightNode.m_pdfFromNext = 0.0;

            if ( !config.eyeConfig.neSampler.sample(
                camera,
                sceneWorldVoxelGrid,
                sceneBackground,
                null,
                path.m_eyeEndNode,
                newLightNode,
                Math.random(),
                Math.random()) ) {
                // No light point sampled, no contribution possible

                path.m_lightPath = oldLightPath;
                return;
            }

            // Compute the normal sampling pdf for the new path/point
            oldPdfLNE = path.m_pdfLNE;
            path.m_pdfLNE = newLightNode.m_pdfFromPrev;
            newLightNode.m_pdfFromPrev = config.lightConfig.pointSampler.evalPDF(camera, null, newLightNode);

            path.m_lightEndNode = newLightNode;
        }

        if ( RayTools.pathNodesVisible(sceneWorldVoxelGrid, path.m_eyeEndNode, path.m_lightEndNode) ) {
            f = computeNeFluxEstimate(camera, config, path, pdf, weight, fRad);

            float factor = (float)config.fluxToRadFactor / (float)config.baseConfig.samplesPerPixel;
            f.scale(factor);
            addWithSpikeCheck(config, path, config.nx, config.ny,
                config.xSample, config.ySample, f);

            if ( config.baseConfig.useSpars != 0 ) {
                fRad.scale(factor);
                addWithSpikeCheck(config, path, config.nx, config.ny,
                    config.xSample, config.ySample, fRad, true);
            }
        }

        if ( doLNE ) {
            // Restore the current light path
            path.m_lightPath = oldLightPath;
            path.m_lightSize = oldLightSize;
            path.m_lightEndNode = oldLightEndNode;
            path.m_pdfLNE = oldPdfLNE;
        }
    }

    private static void
    handlePath1X(
        Camera camera,
        VoxelGrid sceneVoxelGrid,
        BidirectionalPathTracingConfiguration config,
        BiPath path)
    {
        double oldPdfLNE;

        if ( (path.m_eyeSize + path.m_lightSize) > config.baseConfig.maximumPathDepth ) {
            return;
        }

        // We don't do light importance sampling for particle tracing paths
        oldPdfLNE = path.m_pdfLNE;
        path.m_pdfLNE = config.lightPath.m_pdfFromPrev;

        // First we need to determine if the lightEndNode can be seen from
        // the camera. At the same time the pixel hit is computed
        float[] pixX = new float[1];
        float[] pixY = new float[1];
        if ( RayTools.eyeNodeVisible(
            camera,
            sceneVoxelGrid,
            path.m_eyeEndNode,
            path.m_lightEndNode,
            pixX,
            pixY) ) {
            int[] nx = new int[1];
            int[] ny = new int[1];
            ColorRgb f;
            ColorRgb fRad = new ColorRgb();
            float[] pdf = new float[1];
            float[] weight = new float[1];

            // Visible !
            f = computeNeFluxEstimate(camera, config, path, pdf, weight, fRad);

            config.screen.getPixel(pixX[0], pixY[0], nx, ny);

            float factor = ScreenBuffer.computeFluxToRadFactor(camera, nx[0], ny[0]) / (float)config.baseConfig.totalSamples;
            f.scale(factor);

            addWithSpikeCheck(config, path, nx[0], ny[0], pixX[0], pixY[0], f);

            if ( config.baseConfig.useSpars != 0 ) {
                fRad.scale(factor);
                addWithSpikeCheck(config, path, nx[0], ny[0], pixX[0], pixY[0], fRad, true);
            }
        }

        // Restore
        path.m_pdfLNE = oldPdfLNE;
    }

    private static void
    bpCombinePaths(
        Camera camera,
        VoxelGrid sceneVoxelGrid,
        Background sceneBackground,
        BidirectionalPathTracingConfiguration config)
    {
        int eyeSize;
        int lightSize;
        boolean eyeSubPathDone;
        boolean lightSubPathDone;
        SimpleRaytracingPathNode eyeEndNode;
        SimpleRaytracingPathNode lightEndNode;
        SimpleRaytracingPathNode eyePath;
        SimpleRaytracingPathNode lightPath;
        BiPath path = new BiPath();

        eyePath = config.eyePath;
        lightPath = config.lightPath;

        // Compute pdfLNE for the second point in the light path
        if ( config.lightPath != null ) {
            // Single light point get importance sampled, so compute
            // the pdf for it, in order to get correct weights.
            if ( config.baseConfig.sampleImportantLights != 0 && config.lightPath.next() != null ) {
                config.pdfLNE =
                    config.eyeConfig.neSampler.evalPDF(camera, lightPath.next(), lightPath);
            } else {
                config.pdfLNE = lightPath.m_pdfFromPrev;
            }
        } else {
            config.pdfLNE = 0.0;
        }

        path.m_eyePath = eyePath;
        path.m_lightPath = lightPath;
        path.m_pdfLNE = config.pdfLNE;

        // Loop over all possible paths that can be constructed from
        // the eye and light path
        eyeSubPathDone = false;
        eyeSize = 1;  // Always include the eye point!
        eyeEndNode = eyePath;

        while ( !eyeSubPathDone ) {
            // Handle direct hit on light (lightSize = 0)

            if ( eyeSize > 1 ) {
                path.m_eyeSize = eyeSize;
                path.m_eyeEndNode = eyeEndNode;
                path.m_lightSize = 0;
                path.m_lightEndNode = null;

                handlePathX0(camera, sceneBackground, config, path);
            }

            // Handle lightSize > 0 (with N.E.E.)
            lightSubPathDone = lightPath == null;
            lightSize = 1; // lightSize == 0 handled in X_0
            lightEndNode = lightPath;

            while ( !lightSubPathDone ) {
                // Compute N.E.

                if ( eyeSize > 1 ) {
                    // printf("Handle : E %i L %i\n", eyeSize, lightSize);

                    path.m_eyeSize = eyeSize;
                    path.m_eyeEndNode = eyeEndNode;
                    path.m_lightSize = lightSize;
                    path.m_lightEndNode = lightEndNode;
                    handlePathXx(camera, sceneVoxelGrid, sceneBackground, config, path);
                } else {
                    path.m_eyeSize = eyeSize;
                    path.m_eyeEndNode = eyeEndNode;
                    path.m_lightSize = lightSize;
                    path.m_lightEndNode = lightEndNode;
                    handlePath1X(camera, sceneVoxelGrid, config, path);
                }

                if ( lightEndNode.ends() ) {
                    lightSubPathDone = true;
                } else {
                    lightSize++;
                    lightEndNode = lightEndNode.next();
                }
            }

            if ( eyeEndNode.ends() ) {
                eyeSubPathDone = true;
            } else {
                eyeSize++;
                eyeEndNode = eyeEndNode.next();
            }
        }
    }

    private static ColorRgb
    bpCalcPixel(
        Camera camera,
        VoxelGrid sceneVoxelGrid,
        Background sceneBackground,
        int nx,
        int ny,
        Object data)
    {
        BidirectionalPathTracingConfiguration config = (BidirectionalPathTracingConfiguration)data;
        double[] x1 = new double[1];
        double[] x2 = new double[1];
        ColorRgb result = new ColorRgb();
        StratifiedSampling2D stratifiedSampling2D = new StratifiedSampling2D(config.baseConfig.samplesPerPixel);
        SimpleRaytracingPathNode pixNode;
        SimpleRaytracingPathNode nextNode;

        result.clear();

        // We sample the eye here since it's always the same point
        if ( config.eyePath == null ) {
            config.eyePath = new SimpleRaytracingPathNode();
        }

        config.eyeConfig.pointSampler.sample(camera, sceneVoxelGrid, sceneBackground, null, null, config.eyePath, 0, 0);
        ((PixelSampler)config.eyeConfig.dirSampler).SetPixel(camera, nx, ny, null);

        // Provide a node for the pixel sampling
        pixNode = config.eyePath.next();

        if ( pixNode == null ) {
            pixNode = new SimpleRaytracingPathNode();
            config.eyePath.attach(pixNode);
        }

        nextNode = pixNode.next();
        if ( nextNode == null ) {
            nextNode = new SimpleRaytracingPathNode();
            pixNode.attach(nextNode);
        }

        config.nx = nx;
        config.ny = ny;
        config.fluxToRadFactor = ScreenBuffer.computeFluxToRadFactor(camera, nx, ny);

        for ( int i = 0; i < config.baseConfig.samplesPerPixel; i++ ) {
            if ( config.eyeConfig.maxDepth > 1 ) {
                // Generate an eye path
                stratifiedSampling2D.sample(x1, x2);

                config.eyePath.m_rayType = PathRayType.STARTS;

                Vector2D tmpVec2D = config.screen.getPixelCenter((int)x1[0], (int)x2[0]);
                config.xSample = tmpVec2D.x; // pix_x + (camera.pixelWidth * x1)
                config.ySample = tmpVec2D.y; // pix_y + (camera.pixelHeight * x2)

                if ( config.eyeConfig.dirSampler.sample(camera, sceneVoxelGrid, sceneBackground, null, config.eyePath, pixNode, x1[0], x2[0]) ) {
                    pixNode.assignBsdfAndNormal();
                    config.eyeConfig.tracePath(camera, sceneVoxelGrid, sceneBackground, nextNode);
                }
            } else {
                config.eyePath.m_rayType = PathRayType.STOPS;
            }

            // Generate a light path
            if ( config.lightConfig.maxDepth > 0 ) {
                config.lightPath = config.lightConfig.tracePath(camera, sceneVoxelGrid, sceneBackground, config.lightPath);
            } else {
                // Normally this is already so, so no delete necessary ?!
                config.lightPath = null;
            }

            // Connect all endpoints and compute contribution
            bpCombinePaths(camera, sceneVoxelGrid, sceneBackground, config);
        }

        // Radiance contributions are added to the screen buffer directly
        ColorRgb src;
        if ( config.baseConfig.doDensityEstimation != 0 ) {
            if ( config.dBuffer != null ) {
                src = config.screen.get(nx, ny);
            } else {
                src = config.dest.get(nx, ny);
            }
        } else {
            src = config.screen.get(nx, ny);
        }

        result.set(src.r, src.g, src.b);
        return result;
    }

    private void
    doBptAndSubsequentImages(
        Camera camera,
        VoxelGrid sceneVoxelGrid,
        Background sceneBackground,
        BidirectionalPathTracingConfiguration config)
    {
        int maxSamples;
        int nrIterations;
        String format1;
        String format2;

        // Do some trick to render several images, with different
        // number of samples per pixel.

        // Get the highest power of two < number of samples
        nrIterations = (int)(Math.log((double)bidirectionalPathState.baseConfig.samplesPerPixel) /
            Math.log(2.0));
        maxSamples = (int)Math.pow(2.0, nrIterations);

        nrIterations += 1; // First two are 1 and 1

        System.out.printf("nrIter %d, maxSamples %d, origSamples %d\n", nrIterations,
           maxSamples, bidirectionalPathState.baseConfig.samplesPerPixel);

        System.out.printf("Base name '%s'\n", bidirectionalPathState.baseFilename);

        // Numbers are placed after the last point. If
        // no point is given, both ppm.gz and tif images
        // are saved.
        String baseFilename = bidirectionalPathState.baseFilename != null ? bidirectionalPathState.baseFilename : "";
        int lastOcc = baseFilename.lastIndexOf('.');

        if ( lastOcc < 0 ) {
            format1 = baseFilename + "%d.tif";
            format2 = baseFilename + "%d.ppm.gz";
        } else {
            StringBuilder sb = new StringBuilder();
            sb.append(baseFilename, 0, lastOcc);
            sb.append("%d");
            sb.append(baseFilename.substring(lastOcc));
            format1 = sb.toString();
            format2 = "";
        }

        System.out.printf("Format 1 '%s'\n", format1);
        System.out.printf("Format 2 '%s'\n", format2);

        // Now trace the screen several times, each time
        // with twice the number of samples as the previous
        // time. The screen buffer is scaled by 0.5 each iteration

        int currentSamples = 1;
        int totalSamples = 1;

        for ( int i = 0; i < nrIterations; i++ ) {
            // Halve the screen contributions
            if ( i > 0 ) {
                config.screen.scaleRadiance(0.5f);
                config.screen.setAddScaleFactor(0.5f);
            }

            config.baseConfig.samplesPerPixel = currentSamples;
            config.baseConfig.totalSamples = (long)currentSamples * camera.xSize * camera.ySize;

            ScreenIterate.sequential(
                camera,
                sceneVoxelGrid,
                sceneBackground,
                BidirectionalPathRaytracer::bpCalcPixel,
                config,
                config.toneMapOptions);

            config.screen.render();

            // Save images
            if ( !format1.isEmpty() ) {
                String filename = String.format(Locale.US, format1, totalSamples);
                config.screen.writeFile(filename);
            }
            if ( !format2.isEmpty() ) {
                String filename = String.format(Locale.US, format2, totalSamples);
                config.screen.writeFile(filename);
            }

            if ( i > 0 ) {
                currentSamples *= 2;
            }

            totalSamples += currentSamples;
        }
    }

    private void
    doBptDensityEstimation(
        Camera camera,
        VoxelGrid sceneVoxelGrid,
        Background sceneBackground,
        BidirectionalPathTracingConfiguration config)
    {
        String fileName = "";

        // mainInitApplication the screens, one reference one destination
        config.ref = new ScreenBuffer(null, camera, config.toneMapOptions);
        config.dest = new ScreenBuffer(null, camera, config.toneMapOptions);

        config.ref.setFactor(1.0f);
        config.dest.setFactor(1.0f);

        config.dBuffer = new DensityBuffer(config.ref, config.baseConfig);

        if ( config.baseConfig.useSpars != 0 ) {
            config.ref2 = new ScreenBuffer(null, camera, config.toneMapOptions);
            config.dest2 = new ScreenBuffer(null, camera, config.toneMapOptions);

            config.ref2.setFactor(1.0f);
            config.dest2.setFactor(1.0f);

            config.dBuffer2 = new DensityBuffer(config.ref2, config.baseConfig);
        } else {
            config.ref2 = null;
            config.dest2 = null;
            config.dBuffer2 = null;
        }

        // First make a run with D.E. to build ref
        config.baseConfig.samplesPerPixel = 1;
        config.baseConfig.totalSamples = (long)(config.baseConfig.samplesPerPixel *
            config.ref.getHRes() * config.ref.getVRes());

        config.deStoreHits = true;

        // Do the run
        ScreenIterate.sequential(
            camera,
            sceneVoxelGrid,
            sceneBackground,
            BidirectionalPathRaytracer::bpCalcPixel,
            config,
            config.toneMapOptions);

        // Now we have a noisy screen in dest and hits in double buffer

        // Density estimation
        config.dBuffer.reconstruct(); // Estimates ref with fixed kernel width
        config.ref.render();

        if ( config.dBuffer2 != null ) {
            config.dBuffer2.reconstruct(); // Estimates ref with fixed kernel width
            config.ref2.render();
        }

        // Now reconstruct the hits using variable kernels
        config.dBuffer.reconstructVariable(config.dest, 5.0f);
        config.dest.render();

        if ( config.dBuffer2 != null ) {
            config.dBuffer2.reconstructVariable(config.dest2, 1.5f);
            config.dest2.render();
        }

        config.dBuffer = null; // Not needed anymore

        if ( config.dBuffer2 != null ) {
            config.dBuffer2 = null; // Not needed anymore
        }

        // We have a better approximation in dest based on some samples per pixel BPT
        // We will now do subsequent BPT passes, adding the hits directly to dest
        config.deStoreHits = false;

        int numberOfIterations =
            (int)Math.floor(
                Math.log((double)bidirectionalPathState.baseConfig.samplesPerPixel) /
                Math.log(2.0));
        int maxSamples = (int)Math.pow(2.0, numberOfIterations);

        System.out.printf("Doing %d iterations, thus %d samples per pixel\n", numberOfIterations, maxSamples);

        int oldSPP = config.baseConfig.samplesPerPixel; // These were stored
        int newSPP = oldSPP;
        int oldTotalSPP = oldSPP;
        int newTotalSPP = oldTotalSPP + newSPP;

        for ( int i = 1; i <= numberOfIterations; i++ ) {
            System.out.printf("Doing run with %d samples, %d samples already done\n", newSPP, oldTotalSPP);

            // copy dest to ref

            config.ref.copy(config.dest, camera);

            if ( config.ref2 != null ) {
                config.ref2.copy(config.dest2, camera);
            }

            // Rescale dest: nOld / nNew
            config.dest.scaleRadiance((float)oldTotalSPP / (float)newTotalSPP);

            // Set scale factor for added radiance in this run
            config.dest.setAddScaleFactor((float)newSPP / (float)newTotalSPP);

            if ( config.dest2 != null ) {
                // Rescale dest: nOld / nNew
                config.dest2.scaleRadiance((float)oldTotalSPP / (float)newTotalSPP);

                // Set scale factor for added radiance in this run
                config.dest2.setAddScaleFactor((float)newSPP / (float)newTotalSPP);
            }

            // Set current vars
            config.baseConfig.samplesPerPixel = newSPP;
            config.baseConfig.totalSamples = (long)(config.baseConfig.samplesPerPixel *
                config.ref.getHRes() * config.ref.getVRes());

            config.scaleSamples = newTotalSPP; // Base kernel size on total #s/pix

            // Iterate screen : nNew - nOld, using an appropriate scale factor

            ScreenIterate.sequential(
                camera,
                sceneVoxelGrid,
                sceneBackground,
                BidirectionalPathRaytracer::bpCalcPixel,
                config,
                config.toneMapOptions);

            // Render screen & write

            config.dest.render();
            fileName = String.format(Locale.US, "deScreen%d.ppm.gz", newTotalSPP);

            if ( config.dest2 != null ) {
                config.dest2.render();
                fileName = String.format(Locale.US, "de2Screen%d.ppm.gz", newTotalSPP);

                // Merge two images (just add!) into screen
                config.screen.merge(config.dest, config.dest2, camera);
                config.screen.render();
                fileName = String.format(Locale.US, "deMRGScreen%d.ppm.gz", newTotalSPP);
            } else {
                config.screen.copy(config.dest, camera);
            }

            // Update vars
            newSPP = newSPP * 2;
            oldTotalSPP = newTotalSPP;
            newTotalSPP = oldTotalSPP + newSPP;
        }

        config.dest = null;
        config.ref = null;
        if ( config.ref2 != null ) {
            config.ref2 = null;
        }
        if ( config.dest2 != null ) {
            config.dest2 = null;
        }

        if ( fileName.length() > STRINGS_SIZE ) {
            fileName = fileName.substring(0, STRINGS_SIZE);
        }
    }
}
