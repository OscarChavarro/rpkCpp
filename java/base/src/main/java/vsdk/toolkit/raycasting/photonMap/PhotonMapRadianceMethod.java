package vsdk.toolkit.raycasting.photonMap;

import vsdk.toolkit.common.Random;

import java.io.OutputStream;
import java.util.ArrayList;
import java.util.Locale;

import vsdk.toolkit.common.color.ColorRgb;
import vsdk.toolkit.common.logging.Logger;
import vsdk.toolkit.material.RenderOptions;
import vsdk.toolkit.common.linealAlgebra.Vector3D;
import vsdk.toolkit.common.statistics.Statistics;
import vsdk.toolkit.material.BsdfComponent;
import vsdk.toolkit.material.PhongBidirectionalScatteringDistributionFunction;
import vsdk.toolkit.raycasting.bidirectionalRaytracing.BiPath;
import vsdk.toolkit.raycasting.bidirectionalRaytracing.LightDirSampler;
import vsdk.toolkit.raycasting.bidirectionalRaytracing.LightList;
import vsdk.toolkit.raycasting.bidirectionalRaytracing.UniformLightSampler;
import vsdk.toolkit.raycasting.common.BsdfComp;
import vsdk.toolkit.raycasting.common.RayTools;
import vsdk.toolkit.raycasting.common.SimpleRaytracingPathNode;
import vsdk.toolkit.raycasting.raytracing.BsdfSampler;
import vsdk.toolkit.raycasting.raytracing.EyeSampler;
import vsdk.toolkit.raycasting.raytracing.SampleConnectionFlags;
import vsdk.toolkit.raycasting.raytracing.Sampler;
import vsdk.toolkit.raycasting.raytracing.SamplerConfig;
import vsdk.toolkit.raycasting.raytracing.SurfaceSampler;
import vsdk.toolkit.render.ScreenBuffer;
import vsdk.toolkit.scene.Background;
import vsdk.toolkit.scene.Camera;
import vsdk.toolkit.scene.RadianceMethod;
import vsdk.toolkit.scene.RadianceMethodAlgorithm;
import vsdk.toolkit.scene.Scene;
import vsdk.toolkit.scene.VoxelGrid;
import vsdk.toolkit.environment.geometry.elements.Element;
import vsdk.toolkit.environment.geometry.elements.Patch;
import vsdk.toolkit.environment.geometry.elements.RayHit;
import vsdk.toolkit.tonemap.ToneMappingContext;

// To adjust photonMapGetRadiance returns
public class PhotonMapRadianceMethod extends RadianceMethod {
    private static final int STRING_LENGTH = 1000;
    private static boolean doingLocalRayCasting = false;

    private final PhotonMapState photonMapState;
    private final PhotonMapConfig photonMapConfig;

    private static void appendStatsText(StringBuilder buffer, int[] offset, String format, Object... args) {
        if ( offset[0] >= STRING_LENGTH - 1 ) {
            return;
        }

        String text;
        try {
            text = String.format(Locale.US, format, args);
        } catch ( Exception e ) {
            text = format;
        }

        int available = STRING_LENGTH - offset[0];
        if ( available <= 0 ) {
            return;
        }

        if ( text.length() >= available ) {
            buffer.append(text, 0, available - 1);
            offset[0] = STRING_LENGTH - 1;
        } else {
            buffer.append(text);
            offset[0] += text.length();
        }
    }

    public PhotonMapRadianceMethod(PhotonMapState inPhotonMapState, PhotonMapConfig inPhotonMapConfig) {
        photonMapState = inPhotonMapState;
        photonMapConfig = inPhotonMapConfig;

        photonMapState.setDefaults();
        className = RadianceMethodAlgorithm.PHOTON_MAP;
    }

    @Override
    public String getRadianceMethodName() {
        return "Photon map";
    }

    @Override
    public void parseOptions(int[] argc, String[] argv) {
    }

    @Override
    public void writeVRML(
        Camera camera,
        OutputStream outputStream,
        RenderOptions renderOptions)
    {
    }

    /**
For counting how much CPU time was used for the computations
*/
    private void photonMapRadiosityUpdateCpuSecs() {
        long t = System.nanoTime();
        photonMapState.cpuSecs += (float)((double)(t - photonMapState.lastClock) / 1000000000.0);
        photonMapState.lastClock = t;
    }

    @Override
    public Element createPatchData(Patch patch) {
        patch.radianceData = null;
        return patch.radianceData;
    }

    @Override
    public void destroyPatchData(Patch patch) {
        patch.radianceData = null;
    }

    private void photonMapChooseSurfaceSampler(SurfaceSampler[] samplerPtr) {
        if ( samplerPtr[0] != null ) {
            samplerPtr[0] = null;
        }

        if ( photonMapState.usePhotonMapSampler != 0 ) {
            samplerPtr[0] = new PhotonMapSampler();
        } else {
            samplerPtr[0] = new BsdfSampler();
        }
    }

    /**
Initializes the computations for the current scene (if any)
*/
    @Override
    public void initialize(Scene scene, ToneMappingContext toneMapOptions) {
        System.err.printf("Photon map activated\n");

        if ( toneMapOptions == null ) {
            Logger.fatal(-1, "PhotonMapRadianceMethod::initialize", "Tone mapping context not provided");
        }

        photonMapState.lastClock = System.nanoTime();
        photonMapState.cpuSecs = 0.0f;
        photonMapState.gIterationNumber = 0;
        photonMapState.cIterationNumber = 0;
        photonMapState.i_iteration_nr = 0;
        photonMapState.iterationNumber = 0;
        photonMapState.runStopNumber = 0;
        photonMapState.totalGPaths = 0;
        photonMapState.totalCPaths = 0;
        photonMapState.totalIPaths = 0;

        photonMapConfig.screen = new ScreenBuffer(null, scene.camera, toneMapOptions);

        photonMapConfig.lightList = new LightList(scene.lightSourcePatchList);

        // mainInitApplication samplers

        photonMapConfig.lightConfig.releaseVars();
        photonMapConfig.eyeConfig.releaseVars();

        SamplerConfig cfg = photonMapConfig.eyeConfig;

        cfg.pointSampler = new EyeSampler();

        cfg.dirSampler = new ScreenSampler();

        SurfaceSampler[] selected = new SurfaceSampler[] {cfg.surfaceSampler};
        photonMapChooseSurfaceSampler(selected);
        cfg.surfaceSampler = selected[0];
        cfg.surfaceSampler.SetComputeFromNextPdf(false);
        cfg.neSampler = null;

        cfg.minDepth = 1;
        cfg.maxDepth = 1;  // Only eye point needed, for Particle tracing test

        cfg = photonMapConfig.lightConfig;

        cfg.pointSampler = new UniformLightSampler(photonMapConfig.lightList);
        cfg.dirSampler = new LightDirSampler();
        selected = new SurfaceSampler[] {cfg.surfaceSampler};
        photonMapChooseSurfaceSampler(selected);
        cfg.surfaceSampler = selected[0];
        // cfg.surfaceSampler = new PhotonMapSampler; //new BsdfSampler;
        cfg.surfaceSampler.SetComputeFromNextPdf(false);  // Only 1 pdf

        cfg.minDepth = photonMapState.minimumLightPathDepth;
        cfg.maxDepth = photonMapState.maximumLightPathDepth;

        Statistics.instance().rayTracer.rayCount = 0;

        // mainInitApplication the photon map

        photonMapConfig.map = new PhotonMap(
            photonMapState,
            new int[] {photonMapState.reconGPhotons},
            photonMapState.precomputeGIrradiance != 0);

        photonMapConfig.importanceMap = new ImportanceMap(
            photonMapState,
            new int[] {photonMapState.reconIPhotons},
            new float[] {photonMapState.gImpScale});

        photonMapConfig.importanceCMap = new ImportanceMap(
            photonMapState,
            new int[] {photonMapState.reconIPhotons},
            new float[] {photonMapState.cImpScale});

        photonMapConfig.causticMap = new PhotonMap(photonMapState, new int[] {photonMapState.reconCPhotons});
    }

    private static ColorRgb cloneColor(ColorRgb c) {
        return new ColorRgb(c.getR(), c.getG(), c.getB());
    }

    private static BsdfComp cloneBsdfComp(BsdfComp comp) {
        BsdfComp copy = new BsdfComp();
        for ( int i = 0; i < copy.comp.length; i++ ) {
            copy.comp[i].set(comp.comp[i].getR(), comp.comp[i].getG(), comp.comp[i].getB());
        }
        return copy;
    }

    /**
Adapted from bi-directional path, this is a bit overkill for here
*/
    private ColorRgb
    photonMapDoComputePixelFluxEstimate(
        Camera camera,
        PhotonMapConfig config,
        RadianceMethod radianceMethod)
    {
        BiPath bp = config.biPath;
        SimpleRaytracingPathNode eyePrevNode;
        SimpleRaytracingPathNode lightPrevNode;
        ColorRgb oldBsdfL;
        ColorRgb oldBsdfE;
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
        eyeEndNode = bp.m_eyeEndNode;
        lightEndNode = bp.m_lightEndNode;
        eyePrevNode = eyeEndNode.previous();
        lightPrevNode = lightEndNode.previous();

        oldBsdfL = cloneColor(lightEndNode.m_bsdfEval);
        oldBsdfCompL = cloneBsdfComp(lightEndNode.m_bsdfComp);

        oldBsdfE = cloneColor(eyeEndNode.m_bsdfEval);
        oldBsdfCompE = cloneBsdfComp(eyeEndNode.m_bsdfComp);

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
        bp.m_geomConnect =
            SamplerConfig.pathNodeConnect(camera, eyeEndNode, lightEndNode,
                config.eyeConfig, config.lightConfig,
                SampleConnectionFlags.CONNECT_EL | SampleConnectionFlags.CONNECT_LE,
                Sampler.BSDF_ALL_COMPONENTS, Sampler.BSDF_ALL_COMPONENTS, bp.m_dirEL);

        bp.m_dirLE.scaledCopy(-1.0f, bp.m_dirEL);

        // Evaluate radiance and probabilityDensityFunction and weight
        f = bp.evalRadiance();

        float factor = 1.0f / (float)bp.evalPdfAcc();

        f.scale(factor); // Flux estimate

        // Restore old values
        lightEndNode.m_bsdfEval.set(oldBsdfL.getR(), oldBsdfL.getG(), oldBsdfL.getB());
        lightEndNode.m_bsdfComp = oldBsdfCompL;

        eyeEndNode.m_bsdfEval.set(oldBsdfE.getR(), oldBsdfE.getG(), oldBsdfE.getB());
        eyeEndNode.m_bsdfComp = oldBsdfCompE;

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
Test next event estimator to the screen. The result is standard
particle tracing, although constructing global & caustic together
does not give correct display
*/
    private void
    photonMapDoScreenNEE(
        Camera camera,
        VoxelGrid sceneWorldVoxelGrid,
        PhotonMapConfig config,
        RadianceMethod radianceMethod)
    {
        int[] nx = new int[1];
        int[] ny = new int[1];
        float[] pixX = new float[1];
        float[] pixY = new float[1];
        ColorRgb f;
        BiPath bp = config.biPath;

        if ( config.currentMap == config.importanceMap ) {
            return;
        }

        // First we need to determine if the lightEndNode can be seen from
        // the camera. At the same time the pixel hit is computed
        if ( RayTools.eyeNodeVisible(
            camera,
            sceneWorldVoxelGrid,
            bp.m_eyeEndNode,
            bp.m_lightEndNode,
            pixX,
            pixY) ) {
            // Visible !
            f = photonMapDoComputePixelFluxEstimate(camera, config, radianceMethod);

            config.screen.getPixel(pixX[0], pixY[0], nx, ny);

            float factor;

            if ( config.currentMap == config.map ) {
                factor = (ScreenBuffer.computeFluxToRadFactor(camera, nx[0], ny[0])
                      / (float)photonMapState.totalGPaths);
            } else {
                factor = (ScreenBuffer.computeFluxToRadFactor(camera, nx[0], ny[0])
                      / (float)photonMapState.totalCPaths);
            }

            f.scale(factor);

            config.screen.add(nx[0], ny[0], f);
        }
    }


    /**
Store a photon. Some acceptance tests are performed first
*/
    private boolean
    photonMapDoPhotonStore(
        Camera camera,
        SimpleRaytracingPathNode node,
        ColorRgb power)
    {
        if ( node.m_hit.getPatch() != null && node.m_hit.getPatch().material != null ) {
            // Only add photons on surfaces with a certain reflection
            // coefficient

            PhongBidirectionalScatteringDistributionFunction bsdf = node.m_hit.getPatch().material.getBsdf();

            if ( !PhotonMap.zeroAlbedo(bsdf, node.m_hit,
                    (byte)(BsdfComponent.BRDF_DIFFUSE_COMPONENT |
                        BsdfComponent.BTDF_DIFFUSE_COMPONENT |
                        BsdfComponent.BRDF_GLOSSY_COMPONENT |
                        BsdfComponent.BTDF_GLOSSY_COMPONENT)) ) {
                Photon photon = new Photon(node.m_hit.getPoint(), power, node.m_inDirF);

                // Determine photon flags
                short flags = 0;

                if ( node.m_depth == 1 ) {
                    // Direct light photon
                    flags |= PhotonFlags.DIRECT_LIGHT_PHOTON;
                }

                if ( photonMapState.densityControl == PhotonMapDensityControlOption.NO_DENSITY_CONTROL ) {
                    return photonMapConfig.currentMap.addPhoton(photon, node.m_hit.getNormal(), flags);
                } else {
                    float reqDensity;
                    if ( photonMapState.densityControl == PhotonMapDensityControlOption.CONSTANT_RD ) {
                        reqDensity = photonMapState.constantRD;
                    } else {
                        reqDensity = photonMapConfig.currentImpMap.getRequiredDensity(
                            camera,
                            node.m_hit.getPoint(),
                            node.m_hit.getNormal());
                    }

                    return photonMapConfig.currentMap.DC_AddPhoton(photon, node.m_hit, reqDensity, flags);
                }
            }
        }
        return false;
    }

    /**
Handle one path : store at all end positions and for testing, connect to the eye
*/
    private void
    photonMapHandlePath(
        Camera camera,
        VoxelGrid sceneWorldVoxelGrid,
        PhotonMapConfig config,
        RadianceMethod radianceMethod)
    {
        boolean lDone;
        BiPath bp = config.biPath;
        ColorRgb accPower = new ColorRgb();
        float factor;

        // Iterate over all light nodes
        bp.m_lightSize = 1;
        SimpleRaytracingPathNode currentNode = bp.m_lightPath;

        bp.m_eyeSize = 1;
        bp.m_eyeEndNode = bp.m_eyePath;
        bp.m_geomConnect = 1.0; // No connection yet

        lDone = false;
        accPower.setMonochrome(1.0f);

        while ( !lDone ) {
            // Adjust accPower
            factor = (float)(currentNode.m_G / currentNode.m_pdfFromPrev);
            accPower.scale(factor);

            // Store photon, but not emitted light
            if ( config.currentMap == config.map ) {
                // Store
                if ( bp.m_lightSize > 1 && photonMapDoPhotonStore(camera, currentNode, accPower) ) {
                    // Screen next event estimation for testing
                    bp.m_lightEndNode = currentNode;
                    photonMapDoScreenNEE(camera, sceneWorldVoxelGrid, config, radianceMethod);
                }
            } else {
                // Caustic map...
                // Store
                if ( bp.m_lightSize > 2 && photonMapDoPhotonStore(camera, currentNode, accPower) ) {
                    // Screen next event estimation for testing

                    bp.m_lightEndNode = currentNode;
                    photonMapDoScreenNEE(camera, sceneWorldVoxelGrid, config, radianceMethod);
                }
            }

            // Account for bsdf, node that for the first node, this accounts
            // for the emitted radiance.
            if ( !currentNode.ends() ) {
                accPower.selfScalarProduct(currentNode.m_bsdfEval);

                currentNode = currentNode.next();
                bp.m_lightSize++;
            } else {
                lDone = true;
            }
        }
    }

    private void
    photonMapTracePath(
        Camera camera,
        VoxelGrid sceneVoxelGrid,
        Background sceneBackground,
        PhotonMapConfig config,
        byte bsdfFlags) {
        config.biPath.m_eyePath = config.eyeConfig.tracePath(camera, sceneVoxelGrid, sceneBackground, config.biPath.m_eyePath);

        // Use qmc for light sampling
        SimpleRaytracingPathNode path = config.biPath.m_lightPath;

        // First node
        double x1 = Random.drand48(); // nrs[0] * RECIP
        double x2 = Random.drand48(); // nrs[1] * RECIP

        path = config.lightConfig.traceNode(camera, sceneVoxelGrid, sceneBackground, path, x1, x2, bsdfFlags);
        if ( path == null ) {
            return;
        }

        config.biPath.m_lightPath = path;  // In case no nodes were present

        path.ensureNext();

        // Second node
        SimpleRaytracingPathNode node = path.next();
        x1 = Random.drand48(); // nrs[2] * RECIP
        x2 = Random.drand48(); // nrs[3] * RECIP // 4D Niederreiter...

        if ( config.lightConfig.traceNode(camera, sceneVoxelGrid, sceneBackground, node, x1, x2, bsdfFlags) != null ) {
            // Successful trace
            node.ensureNext();
            config.lightConfig.tracePath(camera, sceneVoxelGrid, sceneBackground, node.next(), bsdfFlags);
        }
    }

    private void
    photonMapTracePaths(
        Camera camera,
        VoxelGrid sceneWorldVoxelGrid,
        Background sceneBackground,
        PhotonMapConfig config,
        int numberOfPaths,
        byte bsdfFlags,
        RadianceMethod radianceMethod)
    {
        // Fill in config structures
        for ( int i = 0; i < numberOfPaths; i++ ) {
            photonMapTracePath(camera, sceneWorldVoxelGrid, sceneBackground, config, bsdfFlags);
            photonMapHandlePath(camera, sceneWorldVoxelGrid, config, radianceMethod);
        }
    }

    private void
    photonMapBRRealIteration(
        Camera camera,
        VoxelGrid sceneWorldVoxelGrid,
        Background sceneBackground,
        RadianceMethod radianceMethod)
    {
        photonMapState.iterationNumber++;

        System.err.printf("PhotonMapRadianceMethod Iteration %d\n", photonMapState.iterationNumber);

        if ( (photonMapState.iterationNumber > 1) && (photonMapState.doGlobalMap != 0 || photonMapState.doCausticMap != 0) ) {
            float scaleFactor = ((float)photonMapState.iterationNumber - 1.0f) / (float)photonMapState.iterationNumber;
            photonMapConfig.screen.scaleRadiance(scaleFactor);
        }

        if ( photonMapState.densityControl == PhotonMapDensityControlOption.IMPORTANCE_RD
          && photonMapState.doImportanceMap != 0 ) {
            photonMapState.i_iteration_nr++;
            photonMapConfig.currentMap = photonMapConfig.importanceMap;
            photonMapState.totalIPaths = photonMapState.i_iteration_nr * photonMapState.iPathsPerIteration;
            photonMapConfig.currentMap.setTotalPaths(photonMapState.totalIPaths);
            photonMapConfig.importanceCMap.setTotalPaths(photonMapState.totalIPaths);

            PhotonMapImportance.tracePotentialPaths(
                camera,
                sceneWorldVoxelGrid,
                sceneBackground,
                (int)photonMapState.iPathsPerIteration,
                photonMapState,
                photonMapConfig);

            System.err.printf("Total potential paths : %d, Total rays %d\n",
                photonMapState.totalIPaths,
                Statistics.instance().rayTracer.rayCount);
        }

        // Global map
        if ( photonMapState.doGlobalMap != 0 ) {
            photonMapState.gIterationNumber++;
            photonMapConfig.currentMap = photonMapConfig.map;
            photonMapState.totalGPaths = photonMapState.gIterationNumber * photonMapState.gPathsPerIteration;
            photonMapConfig.currentMap.setTotalPaths(photonMapState.totalGPaths);

            // Set correct importance map: indirect importance
            photonMapConfig.currentImpMap = photonMapConfig.importanceMap;

            photonMapTracePaths(
                camera,
                sceneWorldVoxelGrid,
                sceneBackground,
                photonMapConfig,
                (int)photonMapState.gPathsPerIteration,
                Sampler.BSDF_ALL_COMPONENTS,
                radianceMethod);

            System.err.printf("Global map: ");
            photonMapConfig.map.printStats(System.err);
        }

        // Caustic map
        if ( photonMapState.doCausticMap != 0 ) {
            photonMapState.cIterationNumber++;
            photonMapConfig.currentMap = photonMapConfig.causticMap;
            photonMapState.totalCPaths = photonMapState.cIterationNumber * photonMapState.cPathsPerIteration;
            photonMapConfig.currentMap.setTotalPaths(photonMapState.totalCPaths);

            // Set correct importance map: direct importance
            photonMapConfig.currentImpMap = photonMapConfig.importanceCMap;

            photonMapTracePaths(
                camera,
                sceneWorldVoxelGrid,
                sceneBackground,
                photonMapConfig,
                (int)photonMapState.cPathsPerIteration,
                (byte)(BsdfComponent.BRDF_SPECULAR_COMPONENT | BsdfComponent.BTDF_SPECULAR_COMPONENT),
                radianceMethod);

            System.err.printf("Caustic map: ");
            photonMapConfig.causticMap.printStats(System.err);
        }
    }

    /**
Performs one step of the radiance computations. The goal most often is
to fill in a RGB color for display of each patch and/or vertex. These
colors are used for hardware rendering if the default hardware rendering
method is not updated in this file
*/
    @Override
    public boolean doStep(Scene scene, RenderOptions renderOptions) {
        photonMapState.lastClock = System.nanoTime();

        photonMapBRRealIteration(scene.camera, scene.voxelGrid, scene.background, this);
        photonMapRadiosityUpdateCpuSecs();

        photonMapState.runStopNumber++;

        return false; // Done. Return false if you want the computations to continue
    }

    /**
Undoes the effect of mainInitApplication() and all side-effects of Step()
*/
    @Override
    public void terminate(ArrayList<Patch> scenePatches) {
        photonMapConfig.screen = null;

        photonMapConfig.lightConfig.releaseVars();
        photonMapConfig.eyeConfig.releaseVars();

        photonMapConfig.map = null;
        photonMapConfig.importanceMap = null;
        photonMapConfig.importanceCMap = null;
        photonMapConfig.causticMap = null;
        photonMapConfig.lightList = null;
    }

    /**
Returns the radiance emitted in the node related direction
*/
    public ColorRgb getNodeGRadiance(SimpleRaytracingPathNode node) {
        photonMapConfig.map.doBalancing(photonMapState.balanceKDTree != 0);
        return photonMapConfig.map.reconstruct(node.m_hit, node.m_inDirF,
            node.m_useBsdf,
            node.m_inBsdf, node.m_outBsdf);
    }

    /**
Returns the radiance emitted in the node related direction
*/
    public ColorRgb getNodeCRadiance(SimpleRaytracingPathNode node) {
        photonMapConfig.causticMap.doBalancing(photonMapState.balanceKDTree != 0);

        return photonMapConfig.causticMap.reconstruct(node.m_hit, node.m_inDirF,
            node.m_useBsdf,
            node.m_inBsdf, node.m_outBsdf);
    }

    @Override
    public ColorRgb
    getRadiance(
        Camera camera,
        Patch patch,
        double u,
        double v,
        Vector3D dir,
        RenderOptions renderOptions)
    {
        RayHit hit = new RayHit();
        Vector3D point = new Vector3D();
        PhongBidirectionalScatteringDistributionFunction bsdf = patch.material.getBsdf();
        ColorRgb radiance = new ColorRgb();
        float density;

        patch.pointBarycentricMapping(u, v, point);
        hit.init(patch, point, patch.normal, patch.material);
        Vector3D normal = hit.getNormal();
        hit.shadingNormal(normal);
        hit.setNormal(normal);

        if ( PhotonMap.zeroAlbedo(bsdf, hit,
            (byte)(BsdfComponent.BRDF_DIFFUSE_COMPONENT |
                BsdfComponent.BTDF_DIFFUSE_COMPONENT |
                BsdfComponent.BRDF_GLOSSY_COMPONENT |
                BsdfComponent.BTDF_GLOSSY_COMPONENT)) ) {
            radiance.clear();
            return radiance;
        }

        RadiosityReturnOption radiosityReturn = RadiosityReturnOption.GLOBAL_RADIANCE;

        if ( doingLocalRayCasting ) {
            radiosityReturn = photonMapState.radianceReturn;
        }

        switch ( radiosityReturn ) {
            case GLOBAL_DENSITY:
                radiance = photonMapConfig.map.getDensityColor(hit);
                break;
            case CAUSTIC_DENSITY:
                radiance = photonMapConfig.causticMap.getDensityColor(hit);
                break;
            case IMPORTANCE_C_DENSITY:
                radiance = photonMapConfig.importanceCMap.getDensityColor(hit);
                break;
            case IMPORTANCE_G_DENSITY:
                radiance = photonMapConfig.importanceMap.getDensityColor(hit);
                break;
            case REC_C_DENSITY:
                {
                    Vector3D nn = hit.getNormal();
                    photonMapConfig.importanceCMap.doBalancing(photonMapState.balanceKDTree != 0);
                    density = photonMapConfig.importanceCMap.getRequiredDensity(
                        camera, hit.getPoint(), nn);
                    hit.setNormal(nn);
                    radiance = PhotonMap.getFalseColor(density, photonMapState);
                }
                break;
            case REC_G_DENSITY:
                photonMapConfig.importanceMap.doBalancing(photonMapState.balanceKDTree != 0);
                density = photonMapConfig.importanceMap.getRequiredDensity(
                    camera, hit.getPoint(), hit.getNormal());
                radiance = PhotonMap.getFalseColor(density, photonMapState);
                break;
            case GLOBAL_RADIANCE:
                radiance = photonMapConfig.map.reconstruct(
                    hit, dir, bsdf, null, bsdf);
                break;
            case CAUSTIC_RADIANCE:
                radiance = photonMapConfig.causticMap.reconstruct(
                    hit, dir, bsdf, null, bsdf);
                break;
            default:
                radiance.clear();
                Logger.error("photonMapGetRadiance", "Unknown radiance return");
        }

        return radiance;
    }

    @Override
    public String getStats() {
        StringBuilder stats = new StringBuilder(STRING_LENGTH);
        int[] statsOffset = new int[] {0};

        appendStatsText(stats, statsOffset, "Photon map Statistics:\n\n");
        appendStatsText(stats, statsOffset, "Ray count %d\n", Statistics.instance().rayTracer.rayCount);
        appendStatsText(stats, statsOffset, "Time %g\n", photonMapState.cpuSecs);

        if ( photonMapConfig.map != null ) {
            appendStatsText(stats, statsOffset, "Global Map: ");
            photonMapConfig.map.getStats(stats, STRING_LENGTH);
            statsOffset[0] = Math.min(stats.length(), STRING_LENGTH - 1);
            appendStatsText(stats, statsOffset, "\n");
        }
        if ( photonMapConfig.causticMap != null ) {
            appendStatsText(stats, statsOffset, "Caustic Map: ");
            photonMapConfig.causticMap.getStats(stats, STRING_LENGTH);
            statsOffset[0] = Math.min(stats.length(), STRING_LENGTH - 1);
            appendStatsText(stats, statsOffset, "\n");
        }
        if ( photonMapConfig.importanceMap != null ) {
            appendStatsText(stats, statsOffset, "Global Importance Map: ");
            photonMapConfig.importanceMap.getStats(stats, STRING_LENGTH);
            statsOffset[0] = Math.min(stats.length(), STRING_LENGTH - 1);
            appendStatsText(stats, statsOffset, "\n");
        }
        if ( photonMapConfig.importanceCMap != null ) {
            appendStatsText(stats, statsOffset, "Caustic Importance Map: ");
            photonMapConfig.importanceCMap.getStats(stats, STRING_LENGTH);
            statsOffset[0] = Math.min(stats.length(), STRING_LENGTH - 1);
            appendStatsText(stats, statsOffset, "\n");
        }

        if ( stats.length() >= STRING_LENGTH ) {
            return stats.substring(0, STRING_LENGTH - 1);
        }
        return stats.toString();
    }
}
