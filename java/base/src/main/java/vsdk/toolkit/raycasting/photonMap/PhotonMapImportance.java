package vsdk.toolkit.raycasting.photonMap;

import vsdk.toolkit.common.color.ColorRgb;
import vsdk.toolkit.material.BsdfComponent;
import vsdk.toolkit.material.PhongBidirectionalScatteringDistributionFunction;
import vsdk.toolkit.raycasting.common.SimpleRaytracingPathNode;
import vsdk.toolkit.raycasting.raytracing.Sampler;
import vsdk.toolkit.raycasting.raytracing.SamplerConfig;
import vsdk.toolkit.scene.Background;
import vsdk.toolkit.scene.Camera;
import vsdk.toolkit.scene.VoxelGrid;

/**
Importon tracing
*/
public class PhotonMapImportance {
    /**
Store a importon/poton. Some acceptance tests are performed first
**/
    private static boolean hasDiffuseOrGlossy(SimpleRaytracingPathNode node) {
        if ( node.m_hit.getPatch() != null && node.m_hit.getPatch().material != null ) {
            PhongBidirectionalScatteringDistributionFunction bsdf = node.m_hit.getPatch().material.getBsdf();
            return !PhotonMap.zeroAlbedo(bsdf, node.m_hit,
                (byte)(BsdfComponent.BRDF_DIFFUSE_COMPONENT |
                    BsdfComponent.BTDF_DIFFUSE_COMPONENT |
                    BsdfComponent.BRDF_GLOSSY_COMPONENT |
                    BsdfComponent.BTDF_GLOSSY_COMPONENT));
        } else {
            return false;
        }
    }

    private static boolean bounceDiffuseOrGlossy(SimpleRaytracingPathNode node) {
        int mask = BsdfComponent.BRDF_DIFFUSE_COMPONENT |
            BsdfComponent.BTDF_DIFFUSE_COMPONENT |
            BsdfComponent.BRDF_GLOSSY_COMPONENT |
            BsdfComponent.BTDF_GLOSSY_COMPONENT;
        return (node.m_usedComponents & mask) != 0;
    }

    private static boolean doImportanceStore(ImportanceMap map, SimpleRaytracingPathNode node, ColorRgb importance) {
        if ( PhotonMapImportance.hasDiffuseOrGlossy(node) ) {
            float importanceF = importance.average();
            float potentialF = 1.0f;

            // Compute footprint
            float footprintF = 1.0f;

            Importon importon = new Importon(node.m_hit.getPoint(), importanceF, potentialF, footprintF, node.m_inDirF);

            return map.addPhoton(importon, node.m_hit.getNormal(), (short)0);
        } else {
            return false;
        }
    }

    // Returns whether a valid potential path was returned.
    private static boolean
    tracePotentialPath(
        Camera camera,
        VoxelGrid sceneVoxelGrid,
        Background sceneBackground,
        PhotonMapState photonMapState,
        PhotonMapConfig photonMapConfig)
    {
        SimpleRaytracingPathNode path = photonMapConfig.biPath.m_eyePath;
        SamplerConfig scfg = photonMapConfig.eyeConfig;

        // Eye node
        path = scfg.traceNode(camera, sceneVoxelGrid, sceneBackground, path, Math.random(), Math.random(), Sampler.BSDF_ALL_COMPONENTS);
        if ( path == null ) {
            return false;
        }
        photonMapConfig.biPath.m_eyePath = path;  // In case no nodes were present

        ColorRgb accImportance = new ColorRgb();  // Track importance along the ray
        accImportance.setMonochrome(1.0f);

        // Adjust importance for eye ray
        float factor = (float)(path.m_G / path.m_pdfFromPrev);
        accImportance.scale(factor);

        boolean indirectImportance = false; // Can we store in the indirect importance map

        // New node
        path.ensureNext();
        SimpleRaytracingPathNode node = path.next();

        // Keep tracing nodes until sampling fails, store importons along the way

        double x1;
        double x2;

        x1 = Math.random();
        x2 = Math.random();

        byte specFlags = (byte)(BsdfComponent.BRDF_SPECULAR_COMPONENT | BsdfComponent.BTDF_SPECULAR_COMPONENT);

        while ( scfg.traceNode(
            camera,
            sceneVoxelGrid,
            sceneBackground,
            node,
            x1,
            x2,
            indirectImportance ? specFlags : Sampler.BSDF_ALL_COMPONENTS
            ) != null ) {
            // Successful trace
            SimpleRaytracingPathNode prev = node.previous();

            // Determine scatter type
            boolean didDG = PhotonMapImportance.bounceDiffuseOrGlossy(prev);
            boolean tooClose = (node.m_G > photonMapState.gThreshold);

            if ( didDG && !tooClose ) {
                indirectImportance = true;
            }

            // Adjust importance
            accImportance.selfScalarProduct(prev.m_bsdfEval);
            factor = (float)(node.m_G / node.m_pdfFromPrev);
            accImportance.scale(factor);

            // Store in map
            ImportanceMap imap = indirectImportance ? photonMapConfig.importanceMap :
                                photonMapConfig.importanceCMap;
            if ( imap != null ) {
                PhotonMapImportance.doImportanceStore(imap, node, accImportance);
            }

            // New node
            node.ensureNext();
            node = node.next();
            x1 = Math.random();
            x2 = Math.random();
        }

        return true;
    }

    public static void
    tracePotentialPaths(
        Camera camera,
        VoxelGrid sceneVoxelGrid,
        Background sceneBackground,
        int numberOfPaths,
        PhotonMapState photonMapState,
        PhotonMapConfig photonMapConfig)
    {
        // Fill in config structures
        photonMapConfig.eyeConfig.maxDepth = 7; // Maximum of 4 specular bounces
        photonMapConfig.eyeConfig.minDepth = 3;

        for ( int i = 0; i < numberOfPaths; i++ ) {
            PhotonMapImportance.tracePotentialPath(
                camera,
                sceneVoxelGrid,
                sceneBackground,
                photonMapState,
                photonMapConfig);
        }

        photonMapConfig.eyeConfig.maxDepth = 1; // Back to NEE state
        photonMapConfig.eyeConfig.minDepth = 1;
    }
}
