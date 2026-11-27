package vsdk.toolkit.raycasting.photonMap;

import vsdk.toolkit.common.Random;

import vsdk.toolkit.common.color.ColorRgb;
import vsdk.toolkit.material.BsdfComponent;
import vsdk.toolkit.material.PhongBidirectionalScatteringDistributionFunction;
import vsdk.toolkit.raycasting.common.SimpleRaytracingPathNode;
import vsdk.toolkit.raycasting.raytracing.Sampler;
import vsdk.toolkit.raycasting.raytracing.SamplerConfig;
import vsdk.toolkit.scene.Background;
import vsdk.toolkit.scene.Camera;
import vsdk.toolkit.scene.SceneQuery;
import vsdk.toolkit.scene.SceneQueryWorkspace;

/**
Importon tracing
*/
public class PhotonMapImportance {
    /**
Store a importon/poton. Some acceptance tests are performed first
**/
    private static boolean hasDiffuseOrGlossy(SimpleRaytracingPathNode node) {
        if ( node.m_hit.getPatch() != null && node.m_hit.getPatch().getMaterial() != null ) {
            PhongBidirectionalScatteringDistributionFunction bsdf = node.m_hit.getPatch().getMaterial().getBsdf();
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
            double importanceF = (double)importance.average();
            double potentialF = 1.0;

            // Compute footprint
            double footprintF = 1.0;

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
        SceneQuery sceneQuery,
        SceneQueryWorkspace sceneSceneQueryWorkspace,
        Background sceneBackground,
        PhotonMapState photonMapState,
        PhotonMapConfig photonMapConfig)
    {
        SimpleRaytracingPathNode path = photonMapConfig.biPath.m_eyePath;
        SamplerConfig scfg = photonMapConfig.eyeConfig;

        // Eye node
        // The C++ port passes drand48() twice as arguments and gcc evaluates
        // function arguments right to left: first draw is x2, second is x1.
        double eyeX2 = Random.drand48();
        double eyeX1 = Random.drand48();
        path = scfg.traceNode(camera, sceneQuery, sceneSceneQueryWorkspace, sceneBackground, path, eyeX1, eyeX2, Sampler.BSDF_ALL_COMPONENTS);
        if ( path == null ) {
            return false;
        }
        photonMapConfig.biPath.m_eyePath = path;  // In case no nodes were present

        ColorRgb accImportance = new ColorRgb();  // Track importance along the ray
        accImportance = new ColorRgb(1.0, 1.0, 1.0);

        // Adjust importance for eye ray
        double factor = (double)(path.m_G / path.m_pdfFromPrev);
        accImportance = accImportance.multiply(factor);

        boolean indirectImportance = false; // Can we store in the indirect importance map

        // New node
        path.ensureNext();
        SimpleRaytracingPathNode node = path.next();

        // Keep tracing nodes until sampling fails, store importons along the way

        double x1;
        double x2;

        x1 = Random.drand48();
        x2 = Random.drand48();

        byte specFlags = (byte)(BsdfComponent.BRDF_SPECULAR_COMPONENT | BsdfComponent.BTDF_SPECULAR_COMPONENT);

        while ( scfg.traceNode(
            camera,
            sceneQuery,
            sceneSceneQueryWorkspace,
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
            accImportance = accImportance.multiply(prev.m_bsdfEval);
            factor = (double)(node.m_G / node.m_pdfFromPrev);
            accImportance = accImportance.multiply(factor);

            // Store in map
            ImportanceMap imap = indirectImportance ? photonMapConfig.importanceMap :
                                photonMapConfig.importanceCMap;
            if ( imap != null ) {
                PhotonMapImportance.doImportanceStore(imap, node, accImportance);
            }

            // New node
            node.ensureNext();
            node = node.next();
            x1 = Random.drand48();
            x2 = Random.drand48();
        }

        return true;
    }

    public static void
    tracePotentialPaths(
        Camera camera,
        SceneQuery sceneQuery,
        SceneQueryWorkspace sceneSceneQueryWorkspace,
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
                sceneQuery,
                sceneSceneQueryWorkspace,
                sceneBackground,
                photonMapState,
                photonMapConfig);
        }

        photonMapConfig.eyeConfig.maxDepth = 1; // Back to NEE state
        photonMapConfig.eyeConfig.minDepth = 1;
    }
}
