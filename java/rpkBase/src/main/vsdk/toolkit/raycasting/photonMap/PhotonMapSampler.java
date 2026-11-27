/**
A sampler specifically designed for use with photon maps.
Specular materials are treated as Fresnel reflectors/refractors.

NO DIFFUSE OR GLOSSY TRANSMITTING SURFACES SUPPORTED YET!
*/

package vsdk.toolkit.raycasting.photonMap;

import vsdk.toolkit.common.VSDK;
import vsdk.toolkit.common.color.ColorRgb;
import vsdk.toolkit.common.logging.Logger;
import vsdk.toolkit.common.linealAlgebra.CoordinateSystem;
import vsdk.toolkit.common.linealAlgebra.Vector3Dd;
import vsdk.toolkit.material.BsdfComponent;
import vsdk.toolkit.material.PhongBidirectionalScatteringDistributionFunction;
import vsdk.toolkit.material.RefractionIndex;
import vsdk.toolkit.material.Xxdf;
import vsdk.toolkit.raycasting.common.PathRayType;
import vsdk.toolkit.raycasting.common.SimpleRaytracingPathNode;
import vsdk.toolkit.raycasting.raytracing.BsdfSampler;
import vsdk.toolkit.scene.Background;
import vsdk.toolkit.scene.Camera;
import vsdk.toolkit.scene.SceneQuery;
import vsdk.toolkit.scene.SceneQueryWorkspace;
import vsdk.toolkit.environment.geometry.elements.RpkRayHit;

/**
This is a hack to get fresnel factors for perfect specular reflection and refraction
*/
public class PhotonMapSampler extends BsdfSampler {
    private PhotonMap m_photonMap; // To be used for importance sampling

    public PhotonMapSampler() {
        m_photonMap = null;
    }

    /**
Returns true a component was chosen, false if absorbed
*/
    private static boolean
    chooseComponent(
        byte flags1,
        byte flags2,
        PhongBidirectionalScatteringDistributionFunction bsdf,
        RpkRayHit hit,
        boolean doRR,
        double[] x,
        double[] probabilityDensityFunction,
        boolean[] chose1)
    {
        ColorRgb color = new ColorRgb();
        double power1;
        double power2;
        double totalPower;

        // Choose between flags1 or flags2 scattering
        color = new ColorRgb(0.0, 0.0, 0.0);

        if ( bsdf != null ) {
            color = new ColorRgb(bsdf.splitBsdfScatteredPower(hit.shadingContext(), flags1 & 0xFF));
        }
        power1 = (double)color.average();

        color = new ColorRgb(0.0, 0.0, 0.0);
        if ( bsdf != null ) {
            color = new ColorRgb(bsdf.splitBsdfScatteredPower(hit.shadingContext(), flags2 & 0xFF));
        }
        power2 = (double)color.average();

        totalPower = power1 + power2;

        if ( totalPower < VSDK.EPSILON ) {
            // No significant scattering
            return false;
        }

        // Account for russian roulette
        if ( !doRR ) {
            power1 /= totalPower;
            power2 /= totalPower;
            totalPower = 1.0;
        }

        // Use x for scattering choice
        if ( x[0] < power1 ) {
            chose1[0] = true;
            probabilityDensityFunction[0] = power1;
            x[0] = x[0] / power1;
        } else if ( x[0] < totalPower ) {
            chose1[0] = false;
            probabilityDensityFunction[0] = power2;
            x[0] = (x[0] - power1) / power2;
        } else {
            // Absorbed
            return false;
        }

        return true;
    }

    // Sample : newNode gets filled, others may change
    // Return true if the node was filled in, false if path Ends
    // When path ends (absorption) the type of thisNode is adjusted to 'Ends'
    @Override
    public boolean
    sample(
        Camera camera,
        SceneQuery sceneQuery,
        SceneQueryWorkspace sceneSceneQueryWorkspace,
        Background sceneBackground,
        SimpleRaytracingPathNode prevNode,
        SimpleRaytracingPathNode thisNode,
        SimpleRaytracingPathNode newNode,
        double x1,
        double x2,
        boolean doRR,
        byte flags)
    {
        PhongBidirectionalScatteringDistributionFunction bsdf = thisNode.m_useBsdf;
        boolean[] sChosen = new boolean[1];
        double[] pdfChoice = new double[1];
        byte sFlagMask;

        final byte sFLAGS = (byte)(BsdfComponent.BRDF_SPECULAR_COMPONENT | BsdfComponent.BTDF_SPECULAR_COMPONENT);
        final byte gdFLAGS = (byte)(BsdfComponent.BRDF_GLOSSY_COMPONENT | BsdfComponent.BRDF_DIFFUSE_COMPONENT);

        // Choose between S or D|G scattering
        // Hack to get separate specular en fresnel ok...
        if ( flags == (byte)BsdfComponent.BRDF_SPECULAR_COMPONENT ) {
            // Specular transmission can result in reflection
            sFlagMask = sFLAGS;
        } else {
            sFlagMask = flags;
        }

        double[] x2a = new double[] {x2};
        if ( !chooseComponent(
            (byte)(sFLAGS & sFlagMask),
            (byte)(gdFLAGS & flags),
            bsdf,
            thisNode.m_hit,
            doRR,
            x2a,
            pdfChoice,
            sChosen) ) {
            return false; // Absorbed
        }

        // Get a sampled direction
        boolean ok;

        if ( sChosen[0] ) {
            ok = fresnelSample(sceneQuery, sceneSceneQueryWorkspace, sceneBackground, prevNode, thisNode, newNode, x2a[0], flags);
        } else {
            flags = (byte)(gdFLAGS & flags);
            ok = gdSample(camera, sceneQuery, sceneSceneQueryWorkspace, sceneBackground, prevNode, thisNode, newNode, x1, x2a[0], flags);
        }

        if ( ok ) {
            // Adjust probabilityDensityFunction with s vs gd choice
            newNode.m_pdfFromPrev *= pdfChoice[0];

            // Component propagation
            newNode.m_accUsedComponents = (byte)(thisNode.m_accUsedComponents | thisNode.m_usedComponents);
        }

        return ok;
    }

    /** The Fresnel sampler works as follows:
1. Index of refractions are taken
2. Reflectance and Transmittance values are taken. Normally one of the two
   would be zero.
2b. Complex index of refraction, converted into geometric iof
3. Perfect reflected and refracted (if necessary) directions are computed
4. cosines and fresnel formulas are computed
5. reflection or refraction is chosen
6. fresnel reflection/refraction multiplied by appropriate scattering powers
7. node filled in.
*/
    private static RefractionIndex
    bsdfGeometricIOR(PhongBidirectionalScatteringDistributionFunction bsdf) {
        RefractionIndex nc = new RefractionIndex();

        if ( bsdf == null ) {
            nc.set(1.0, 0.0); // Vacuum
        } else {
            bsdf.indexOfRefraction(nc);
        }

        // Convert to geometric IOR if necessary
        if ( nc.getNi() > VSDK.EPSILON ) {
            nc.set(nc.complexToGeometricRefractionIndex(), 0.0);
        }

        return nc;
    }

    private static boolean
    chooseFresnelDirection(
        SimpleRaytracingPathNode thisNode,
        byte flags,
        double x2,
        Vector3Dd[] dir,
        double[] pdfDir,
        ColorRgb[] scatteringColor,
        boolean[] doCosInverse)
    {
        // Index of refractions are taken
        RefractionIndex nc_in = bsdfGeometricIOR(thisNode.m_inBsdf);
        RefractionIndex nc_out = bsdfGeometricIOR(thisNode.m_outBsdf);

        // Reflectance and Transmittance values are taken. Normally one of the two
        // would be zero
        PhongBidirectionalScatteringDistributionFunction bsdf = thisNode.m_useBsdf;
        ColorRgb reflectance = new ColorRgb();
        reflectance = new ColorRgb(0.0, 0.0, 0.0);
        if ( bsdf != null ) {
            reflectance = new ColorRgb(bsdf.splitBsdfScatteredPower(thisNode.m_hit.shadingContext(),
                BsdfComponent.BRDF_SPECULAR_COMPONENT | BsdfComponent.BTDF_SPECULAR_COMPONENT));
        }

        ColorRgb transmittance = new ColorRgb();
        transmittance = new ColorRgb(0.0, 0.0, 0.0);
        if ( bsdf != null ) {
            transmittance = new ColorRgb(bsdf.splitBsdfScatteredPower(thisNode.m_hit.shadingContext(),
                BsdfComponent.BRDF_SPECULAR_COMPONENT | BsdfComponent.BTDF_SPECULAR_COMPONENT));
        }

        boolean reflective = (reflectance.average() > VSDK.EPSILON);
        boolean trans = (transmittance.average() > VSDK.EPSILON);

        if ( reflective && trans ) {
            Logger.reportMessage(null, Logger.ERROR, "FresnelFactor", "Cannot deal with simultaneous reflective & transit materials");
            return false;
        }

        // Fresnel reflection factor is computed.
        double cosI;
        double cost;
        double F;  // Fresnel reflection. (Refraction = 1 - T)
        boolean[] tir = new boolean[1]; // total internal reflection
        Vector3Dd reflectedDir = new Vector3Dd();
        Vector3Dd refractedDir = new Vector3Dd();

        if ( reflective ) {
            if ( (flags & BsdfComponent.BRDF_SPECULAR_COMPONENT) != 0 ) {
                // Hack !?
                F = 1.0;
                reflectedDir = Xxdf.idealReflectedDirection(thisNode.m_inDirT, thisNode.m_normal);
                cosI = thisNode.m_normal.dotProduct(thisNode.m_inDirF);
                if ( cosI < 0 ) {
                    Logger.reportMessage(null, Logger.ERROR, "fresnelSample", "cosI < 0");
                }
            } else {
                F = 0;
            }
        } else {
            refractedDir = Xxdf.idealRefractedDirection(thisNode.m_inDirT,
                thisNode.m_normal,
                nc_in,
                nc_out,
                tir);

            if ( !tir[0] ) {
                reflectedDir = Xxdf.idealReflectedDirection(thisNode.m_inDirT,
                    thisNode.m_normal);
            }

            // 4. Cosines and fresnel formulas are computed
            cosI = thisNode.m_normal.dotProduct(thisNode.m_inDirF);

            if ( cosI < 0 ) {
                Logger.reportMessage(null, Logger.ERROR, "fresnelSample", "cosI < 0");
            }

            if ( !tir[0] ) {
                cost = -thisNode.m_normal.dotProduct(refractedDir);

                if ( cost < 0 ) {
                    Logger.reportMessage(null, Logger.ERROR, "fresnelSample", "cost < 0");
                }

                double rParallel;
                double rPerpendicular;
                double nt = nc_out.getNr();
                double ni = nc_in.getNr();

                rParallel = (nt * cosI - ni * cost) / (nt * cosI + ni * cost);
                rPerpendicular = (ni * cosI - nt * cost) / (ni * cosI + nt * cost);

                F = 0.5 * (rParallel * rParallel + rPerpendicular * rPerpendicular);
            } else {
                F = 0; // All in refracted dir, which == reflected dir
            }
        }

        double T = 1.0 - F;
        boolean reflected;

        // Choose reflection or refraction
        double sum = 0.0;

        if ( (flags & BsdfComponent.BTDF_SPECULAR_COMPONENT) != 0 ) {
            sum += T;
        } else {
            T = 0;
        }

        if ( (flags & BsdfComponent.BRDF_SPECULAR_COMPONENT) != 0 ) {
            sum += F;
        } else {
            F = 0;
        }

        if ( sum < VSDK.EPSILON ) {
            return false;
        }

        if ( x2 < T / sum ) {
            reflected = false;
            dir[0] = refractedDir;
            pdfDir[0] = T / sum;
        } else {
            reflected = true;
            dir[0] = reflectedDir;
            pdfDir[0] = F / sum;
        }

        // Compute bsdf evaluation here and determine if we
        // still need to divide by the cosine to get 'real'
        // specular transmission
        if ( reflected ) {
            if ( reflective ) {
                scatteringColor[0] = reflectance.multiply(F);
                doCosInverse[0] = false;
            } else {
                scatteringColor[0] = transmittance.multiply(F);
                doCosInverse[0] = true;
            }
        } else {
            scatteringColor[0] = transmittance.multiply(T);
            doCosInverse[0] = true;
        }

        return true;
    }

    private boolean
    fresnelSample(
        SceneQuery sceneQuery,
        SceneQueryWorkspace sceneSceneQueryWorkspace,
        Background sceneBackground,
        SimpleRaytracingPathNode prevNode,
        SimpleRaytracingPathNode thisNode,
        SimpleRaytracingPathNode newNode,
        double x2,
        byte flags)
    {
        Vector3Dd[] dirHolder = {new Vector3Dd()};
        double[] pdfDir = new double[1];
        boolean[] doCosInverse = new boolean[1];
        final ColorRgb[] scatteringColorHolder = new ColorRgb[] {new ColorRgb(0.0, 0.0, 0.0)};

        if ( !chooseFresnelDirection(thisNode, flags, x2, dirHolder, pdfDir,
                                 scatteringColorHolder, doCosInverse) ) {
            return false;
        }
        final ColorRgb scatteringColor = scatteringColorHolder[0];
        final Vector3Dd dir = dirHolder[0];

        // Reflection Type, changes thisNode->m_rayType and newNode->m_inBsdf
        DetermineRayType(thisNode, newNode, dir);

        // Transfer
        if ( !sampleTransfer(sceneQuery, sceneSceneQueryWorkspace, sceneBackground, thisNode, newNode, dir, pdfDir[0]) ) {
            thisNode.m_rayType = PathRayType.STOPS;
            return false;
        }

        // Fill in bsdf evaluation. This is just reflectance or transmittance
        // given by ChooseFresnelDirection, but may be divided by a cosine
        // -- No bsdf components yet here !!
        if ( doCosInverse[0] ) {
            double cosB = Math.abs(newNode.m_hit.getNormal().dotProduct(newNode.m_inDirT));
            thisNode.m_bsdfEval = scatteringColor.scaleInverse(cosB);
        } else {
            thisNode.m_bsdfEval = scatteringColor;
        }

        // Fill in probability for previous node
        if ( m_computeFromNextPdf && prevNode != null ) {
            Logger.reportMessage(this, Logger.WARNING, "FresnelSampler", "FromNextPdf not supported");
        }

        // Component propagation
        if ( thisNode.m_rayType == PathRayType.REFLECTS ) {
            thisNode.m_usedComponents = (byte)BsdfComponent.BRDF_SPECULAR_COMPONENT;
        } else {
            thisNode.m_usedComponents = (byte)BsdfComponent.BTDF_SPECULAR_COMPONENT;
        }

        return true; // Node filled in
    }

    private boolean
    gdSample(
        Camera camera,
        SceneQuery sceneQuery,
        SceneQueryWorkspace sceneSceneQueryWorkspace,
        Background sceneBackground,
        SimpleRaytracingPathNode prevNode,
        SimpleRaytracingPathNode thisNode,
        SimpleRaytracingPathNode newNode,
        double x1,
        double x2,
        byte flags)
    {
        boolean ok;
        double[] x1Holder = new double[] {x1};
        double[] x2Holder = new double[] {x2};

        // Sample G|D and use m_photonMap for importance sampling if possible.
        if ( m_photonMap == null ) {
            // We can just use standard bsdf sampling
            ok = super.sample(
                camera, sceneQuery, sceneSceneQueryWorkspace, sceneBackground, prevNode, thisNode, newNode, x1, x2, false, flags);
            thisNode.m_usedComponents = flags;
            return ok;
        }

        // -- Currently NEVER reached!

        // Choose G or D
        final PhongBidirectionalScatteringDistributionFunction bsdf = thisNode.m_useBsdf;
        boolean[] dChosen = new boolean[1];
        double[] pdfChoice = new double[1];

        // Choose between D or G scattering
        if ( !chooseComponent((byte)(BsdfComponent.BRDF_DIFFUSE_COMPONENT & flags),
                          (byte)(BsdfComponent.BRDF_GLOSSY_COMPONENT & flags),
                          bsdf,
                          thisNode.m_hit,
                          false,
                          x1Holder,
                          pdfChoice,
                          dChosen) ) {
            return false;
        }

        // Importance sampling using photon map x_1 & x_2 get transformed

        CoordinateSystem coord = new CoordinateSystem();
        double glossy_exponent;

        if ( dChosen[0] ) {
            // Equation [ARVO1995b](6) in CoordinateSystem::setFromZAxis builds the
            // local tangent basis used by the spherical importance map.
            coord.setFromZAxis(thisNode.m_normal);
            glossy_exponent = 1;
            flags = (byte)BsdfComponent.BRDF_DIFFUSE_COMPONENT;
        } else {
            flags = (byte)BsdfComponent.BRDF_GLOSSY_COMPONENT;

            Logger.reportMessage(this, Logger.ERROR, "PhotonMapSampler::gdSample", "Not done yet");
            return false;
        }

        double photonMapPdf = m_photonMap.sample(thisNode.m_hit.getPoint(), x1Holder, x2Holder, coord, flags, glossy_exponent);

        // Do real sampling
        ok = super.sample(
            camera,
            sceneQuery,
            sceneSceneQueryWorkspace,
            sceneBackground,
            prevNode,
            thisNode,
            newNode,
            x1Holder[0],
            x2Holder[0],
            false,
            flags);

        // Adjust probabilityDensityFunction
        if ( ok ) {
            newNode.m_pdfFromPrev *= pdfChoice[0] * photonMapPdf;
            thisNode.m_usedComponents = flags;
        }

        return ok;
    }
}
