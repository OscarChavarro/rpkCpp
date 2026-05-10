package vsdk.toolkit.material;

/**
A simple combination of brdf and btdf.
Handles evaluation and sampling and also
functions that relate to brdf or btdf like reflectance etc.
*/

/*in*/

/*out*/

/*normal*/

/**
Bidirectional Reflectance Distribution Functions (BSDF)

Implementation of a BSDF consisting of one brdf and one bsdf. Either of the components may be nullptr
*/

/**
Creates a BSDF instance with given data and methods
*/

/**
Computes a shading frame at the given hit point. The Z axis of this frame is
the shading normal, The X axis is in the tangent plane on the surface at the
hit point ("brush" direction relevant for anisotropic shaders e.g.). Y
is perpendicular to X and Z. X and Y may be null pointers. In this case,
only the shading normal is returned, avoiding computation of the X and
Y axis if possible).
Note: edf can have also a routine for computing the shading frame. If a
material has both an edf and a bsdf, the shading frame shall of course
be the same.
This routine returns TRUE if a shading frame could be constructed and FALSE if
not. In the latter case, a default frame needs to be used (not computed by this
routine - pointShadingFrame() in material.[ch] constructs such a frame if
needed)
*/

/*hit*/

/*X*/

/*Y*/

/*Z*/

// Not implemented, should call to bsdf->methods->setShadingFrame or something like that

/**
Returns the scattered power (diffuse/glossy/specular reflectance and/or transmittance) according to flags
*/

// Avoid taking it into account again

/**
Albedo is assumed to be 1
*/

// Section [ARVO1995b].2: map (x1, x2) from [0,1]^2 into a hemisphere direction.

/**
Sample a split bsdf. If no sample was taken (RR/absorption)
the pdf will be 0 upon return  Computes probabilities for sampling the texture, reflection minus texture,
or transmission. Also determines b[r|t]dfFlags taking into
account potential texturing
*/

// bsdf has a texture for diffuse reflection and diffuse reflection needs to be sampled

// Rescale into [0,1) interval again

/**
Returns the index of refraction of the BSDF
*/

// Vacuum

/**
Sampling and pdf evaluation

Sampling routines, parameters as in evaluation, except that two
random numbers x1 and x2 are needed (2D sampling process)
*/

// So we can return safely

// Calculate probabilities for sampling the texture, reflection minus texture,

// and transmission. Also fills in correct b[r|t]dfFlags

// Decide whether to sample the texture reflectance, the reflectance

// modes not in the texture, transmission or absorption

// Normalize: no absorption sampling

// Sample according to the selected mode

// Don't care

/* other components will be added later */

// Add probability of sampling the same direction in other than the

// selected scattering mode (e.g. internal reflection) */

/**
Bsdf evaluations
All components of the Bsdf

Vector directions :

in: from patch
out: from patch
hit->normal : leaving from patch, on the incoming side.
         So in . hit->normal > 0!
*/

// Just add brdf and btdf contributions, the eval routines handle the direction of out.

// Note that out * normal is computed more than once :-(

/**
Constructs shading frame at hit point. Returns TRUE if successful and
FALSE if not. X and Y may be null pointers

Sample a split bsdf. If no sample was taken (RR/absorption)
the pdf will be 0 upon return
*/

// Survival probability

// Probability of sampling the outgoing direction, after survival decision

/**
Evaluates all requested components of the BSDF separately and
stores the result in 'colArray'.
Total evaluation is returned.
*/

// Some caching optimisation could be used here

// Set to 0 for safety

import vsdk.toolkit.common.color.ColorRgb;
import vsdk.toolkit.common.Error;
import vsdk.toolkit.common.linealAlgebra.CoordinateSystem;
import vsdk.toolkit.common.linealAlgebra.Numeric;
import vsdk.toolkit.common.linealAlgebra.Vector3D;
import vsdk.toolkit.skin.RayHitFlag;
import vsdk.toolkit.skin.RayHit;

/**
A simple combination of BRDF and BTDF.
*/
public class PhongBidirectionalScatteringDistributionFunction {
    private static final int TEXTURED_COMPONENT = BsdfComponent.BRDF_DIFFUSE_COMPONENT;

    private PhongBidirectionalReflectanceDistributionFunction brdf;
    private PhongBidirectionalTransmittanceDistributionFunction btdf;
    private Texture texture;

    public PhongBidirectionalScatteringDistributionFunction(
        PhongBidirectionalReflectanceDistributionFunction brdf,
        PhongBidirectionalTransmittanceDistributionFunction btdf,
        Texture texture) {
        this.brdf = brdf;
        this.btdf = btdf;
        this.texture = texture;
    }

    public PhongBidirectionalReflectanceDistributionFunction getBrdf() {
        return brdf;
    }

    public PhongBidirectionalTransmittanceDistributionFunction getBtdf() {
        return btdf;
    }

    public Texture getTexture() {
        return texture;
    }

    public static boolean bsdfShadingFrame(RayHit hit, Vector3D X, Vector3D Y, Vector3D Z) {
        return false;
    }

    public static boolean bsdfShadingFrame(ShadingContext context, Vector3D X, Vector3D Y, Vector3D Z) {
        return false;
    }

    private static boolean extractHitData(RayHit hit, Vector3D normal, Vector3D texCoord, int[] flags) {
        if (hit == null || normal == null || texCoord == null || flags == null || flags.length == 0) {
            return false;
        }
        if (!hit.shadingNormal(normal)) {
            return false;
        }
        flags[0] = RayHitFlag.NORMAL;
        if (hit.getTexCoord(texCoord)) {
            flags[0] |= RayHitFlag.TEXTURE_COORDINATE;
        }
        else {
            texCoord.set(0.0f, 0.0f, 0.0f);
        }
        return true;
    }

    private static ColorRgb splitBsdfEvalTexture(Texture texture, ShadingContext context) {
        ColorRgb col = new ColorRgb();
        col.clear();

        if (texture == null) {
            return col;
        }
        if (context == null || !context.hasFlag(RayHitFlag.TEXTURE_COORDINATE)) {
            Error.warning("splitBsdfEvalTexture", "Couldn't get texture coordinates");
            return col;
        }
        Vector3D texCoord = context.getTexCoord();
        return texture.evaluateColor(texCoord.x, texCoord.y);
    }

    private static ColorRgb splitBsdfEvalTexture(Texture texture, RayHit hit) {
        Vector3D texCoord = new Vector3D();
        ColorRgb col = new ColorRgb();
        col.clear();

        if (texture == null) {
            return col;
        }

        if (hit == null || !hit.getTexCoord(texCoord)) {
            Error.warning("splitBsdfEvalTexture", "Couldn't get texture coordinates");
            return col;
        }

        return texture.evaluateColor(texCoord.x, texCoord.y);
    }

    public ColorRgb splitBsdfScatteredPower(ShadingContext context, int flags) {
        ColorRgb albedo = new ColorRgb();
        albedo.clear();

        if (texture != null && (flags & TEXTURED_COMPONENT) != 0) {
            ColorRgb textureColor = splitBsdfEvalTexture(texture, context);
            albedo.add(albedo, textureColor);
            flags &= ~TEXTURED_COMPONENT;
        }

        if (brdf != null) {
            ColorRgb reflectance = brdf.reflectance(flags);
            if (!Float.isFinite(reflectance.average())) {
                Error.fatal(-1, "brdfReflectance", "Oops - test Rd is not finite!");
            }
            albedo.add(albedo, reflectance);
        }

        if (btdf != null) {
            ColorRgb transmitted = btdf.transmittance(BsdfComponentFlag.getBtdfFlags(flags));
            albedo.add(albedo, transmitted);
        }

        return albedo;
    }

    public ColorRgb splitBsdfScatteredPower(RayHit hit, int flags) {
        Vector3D normal = new Vector3D();
        Vector3D texCoord = new Vector3D();
        int[] localFlags = new int[] {0};
        if (!extractHitData(hit, normal, texCoord, localFlags)) {
            ColorRgb out = new ColorRgb();
            out.clear();
            return out;
        }
        ShadingContext context = new ShadingContext(
            hit.getPoint(),
            hit.getGeometricNormal(),
            normal,
            texCoord,
            hit.getUv(),
            hit.getShadingFrame(),
            hit.getMaterial(),
            localFlags[0]);
        return splitBsdfScatteredPower(context, flags);
    }

    public boolean splitBsdfIsTextured() {
        return texture != null;
    }

    private static Vector3D texturedScattererSample(
        Vector3D in,
        Vector3D normal,
        double x1,
        double x2,
        double[] probabilityDensityFunction) {
        CoordinateSystem coord = new CoordinateSystem();
        coord.setFromZAxis(normal);
        return coord.sampleHemisphereCosTheta(x1, x2, probabilityDensityFunction);
    }

    private static void texturedScattererEvalPdf(
        Vector3D in,
        Vector3D out,
        Vector3D normal,
        double[] probabilityDensityFunction) {
        setOut(probabilityDensityFunction, normal.dotProduct(out) / Math.PI);
    }

    private void splitBsdfProbabilities(
        ShadingContext context,
        int flags,
        double[] inTexture,
        double[] reflection,
        double[] transmission,
        int[] brdfFlags,
        int[] btdfFlags) {
        setOut(inTexture, 0.0);

        if (texture != null && (flags & TEXTURED_COMPONENT) != 0) {
            ColorRgb textureColor = splitBsdfEvalTexture(texture, context);
            setOut(inTexture, textureColor.average());
            flags &= ~TEXTURED_COMPONENT;
        }

        setOut(brdfFlags, BsdfComponentFlag.getBrdfFlags(flags));
        setOut(btdfFlags, BsdfComponentFlag.getBtdfFlags(flags));

        ColorRgb reflectance;
        if (brdf == null) {
            reflectance = new ColorRgb();
            reflectance.clear();
        }
        else {
            reflectance = brdf.reflectance(brdfFlags[0]);
        }
        setOut(reflection, reflectance.average());

        ColorRgb transmittance;
        if (btdf == null) {
            transmittance = new ColorRgb();
            transmittance.clear();
        }
        else {
            transmittance = btdf.transmittance(btdfFlags[0]);
        }
        setOut(transmission, transmittance.average());
    }

    private void splitBsdfProbabilities(
        RayHit hit,
        int flags,
        double[] inTexture,
        double[] reflection,
        double[] transmission,
        int[] brdfFlags,
        int[] btdfFlags) {
        setOut(inTexture, 0.0);

        if (texture != null && (flags & TEXTURED_COMPONENT) != 0) {
            ColorRgb textureColor = splitBsdfEvalTexture(texture, hit);
            setOut(inTexture, textureColor.average());
            flags &= ~TEXTURED_COMPONENT;
        }

        setOut(brdfFlags, BsdfComponentFlag.getBrdfFlags(flags));
        setOut(btdfFlags, BsdfComponentFlag.getBtdfFlags(flags));

        ColorRgb reflectance;
        if (brdf == null) {
            reflectance = new ColorRgb();
            reflectance.clear();
        }
        else {
            reflectance = brdf.reflectance(brdfFlags[0]);
        }
        setOut(reflection, reflectance.average());

        ColorRgb transmittance;
        if (btdf == null) {
            transmittance = new ColorRgb();
            transmittance.clear();
        }
        else {
            transmittance = btdf.transmittance(btdfFlags[0]);
        }
        setOut(transmission, transmittance.average());
    }

    private static SplitBSDFSamplingMode splitBsdfSamplingMode(
        double texture,
        double reflection,
        double transmission,
        double[] x1) {
        SplitBSDFSamplingMode mode = SplitBSDFSamplingMode.SAMPLE_ABSORPTION;

        if (x1[0] < texture) {
            mode = SplitBSDFSamplingMode.SAMPLE_TEXTURE;
            x1[0] /= texture;
        }
        else {
            x1[0] -= texture;
            if (x1[0] < reflection) {
                mode = SplitBSDFSamplingMode.SAMPLE_REFLECTION;
                x1[0] /= reflection;
            }
            else {
                x1[0] -= reflection;
                if (x1[0] < transmission) {
                    mode = SplitBSDFSamplingMode.SAMPLE_TRANSMISSION;
                    x1[0] /= transmission;
                }
            }
        }

        return mode;
    }

    public void indexOfRefraction(RefractionIndex index) {
        if (btdf == null) {
            index.set(1.0f, 0.0f);
        }
        else {
            btdf.setIndexOfRefraction(index);
        }
    }

    public Vector3D sample(
        ShadingContext context,
        PhongBidirectionalScatteringDistributionFunction inBsdf,
        PhongBidirectionalScatteringDistributionFunction outBsdf,
        Vector3D in,
        int doRussianRoulette,
        int flags,
        double x1,
        double x2,
        double[] probabilityDensityFunction) {
        Vector3D normal = new Vector3D();
        Vector3D out = new Vector3D();

        setOut(probabilityDensityFunction, 0.0);
        if (context == null || !context.hasFlag(RayHitFlag.NORMAL)) {
            Error.warning("sample", "Couldn't determine shading normal");
            out.set(0.0f, 0.0f, 1.0f);
            return out;
        }
        normal.copy(context.getShadingNormal());

        double[] localTexture = new double[] {0.0};
        double[] reflection = new double[] {0.0};
        double[] transmission = new double[] {0.0};
        int[] brdfFlags = new int[] {0};
        int[] btdfFlags = new int[] {0};

        splitBsdfProbabilities(context, flags, localTexture, reflection, transmission, brdfFlags, btdfFlags);

        double scattering = localTexture[0] + reflection[0] + transmission[0];
        if (scattering < Numeric.EPSILON) {
            return out;
        }

        if (doRussianRoulette == 0) {
            localTexture[0] /= scattering;
            reflection[0] /= scattering;
            transmission[0] /= scattering;
        }

        double[] localX1 = new double[] {x1};
        SplitBSDFSamplingMode mode = splitBsdfSamplingMode(localTexture[0], reflection[0], transmission[0], localX1);

        RefractionIndex inIndex = new RefractionIndex();
        RefractionIndex outIndex = new RefractionIndex();

        if (inBsdf != null) {
            inBsdf.indexOfRefraction(inIndex);
        }
        if (outBsdf != null) {
            outBsdf.indexOfRefraction(outIndex);
        }

        double p;
        switch (mode) {
            case SAMPLE_TEXTURE:
                double[] pTexture = new double[] {0.0};
                out = texturedScattererSample(in, normal, localX1[0], x2, pTexture);
                p = pTexture[0];
                if (p < Numeric.EPSILON) {
                    return out;
                }
                setOut(probabilityDensityFunction, localTexture[0] * p);
                break;
            case SAMPLE_REFLECTION:
                if (brdf == null) {
                    p = 0.0;
                }
                else {
                    double[] pReflection = new double[] {0.0};
                    out = brdf.sample(in, normal, 0, brdfFlags[0], localX1[0], x2, pReflection);
                    p = pReflection[0];
                }
                if (p < Numeric.EPSILON) {
                    return out;
                }
                setOut(probabilityDensityFunction, reflection[0] * p);
                break;
            case SAMPLE_TRANSMISSION:
                if (btdf == null) {
                    p = 0.0;
                    out.x = 0.0f;
                    out.y = 0.0f;
                    out.z = 0.0f;
                }
                else {
                    double[] pTransmission = new double[] {0.0};
                    out = btdf.sample(inIndex, outIndex, in, normal, 0, btdfFlags[0], localX1[0], x2, pTransmission);
                    p = pTransmission[0];
                }
                if (p < Numeric.EPSILON) {
                    return out;
                }
                setOut(probabilityDensityFunction, transmission[0] * p);
                break;
            case SAMPLE_ABSORPTION:
            default:
                setOut(probabilityDensityFunction, 0.0);
                return out;
        }

        if (mode != SplitBSDFSamplingMode.SAMPLE_TEXTURE) {
            double[] pTexture = new double[] {0.0};
            texturedScattererEvalPdf(in, out, normal, pTexture);
            setOut(probabilityDensityFunction, probabilityDensityFunction[0] + localTexture[0] * pTexture[0]);
        }

        if (mode != SplitBSDFSamplingMode.SAMPLE_REFLECTION) {
            double pReflection = 0.0;
            if (brdf != null) {
                double[] pRR = new double[] {0.0};
                double[] outP = new double[] {0.0};
                brdf.evaluateProbabilityDensityFunction(in, out, normal, brdfFlags[0], outP, pRR);
                pReflection = outP[0];
            }
            setOut(probabilityDensityFunction, probabilityDensityFunction[0] + reflection[0] * pReflection);
        }

        if (mode != SplitBSDFSamplingMode.SAMPLE_TRANSMISSION) {
            double pTransmission = 0.0;
            if (btdf != null) {
                double[] pRR = new double[] {0.0};
                double[] outP = new double[] {0.0};
                btdf.evaluateProbabilityDensityFunction(inIndex, outIndex, in, out, normal, btdfFlags[0], outP, pRR);
                pTransmission = outP[0];
            }
            setOut(probabilityDensityFunction, probabilityDensityFunction[0] + transmission[0] * pTransmission);
        }

        return out;
    }

    public Vector3D sample(
        RayHit hit,
        PhongBidirectionalScatteringDistributionFunction inBsdf,
        PhongBidirectionalScatteringDistributionFunction outBsdf,
        Vector3D in,
        int doRussianRoulette,
        int flags,
        double x1,
        double x2,
        double[] probabilityDensityFunction) {
        Vector3D normal = new Vector3D();
        Vector3D texCoord = new Vector3D();
        int[] localFlags = new int[] {0};
        if (!extractHitData(hit, normal, texCoord, localFlags)) {
            Vector3D out = new Vector3D(0.0, 0.0, 1.0);
            setOut(probabilityDensityFunction, 0.0);
            return out;
        }
        ShadingContext context = new ShadingContext(
            hit.getPoint(),
            hit.getGeometricNormal(),
            normal,
            texCoord,
            hit.getUv(),
            hit.getShadingFrame(),
            hit.getMaterial(),
            localFlags[0]);
        return sample(context, inBsdf, outBsdf, in, doRussianRoulette, flags, x1, x2, probabilityDensityFunction);
    }

    private static double texturedScattererEval(Vector3D in, Vector3D out, Vector3D normal) {
        return 1.0 / Math.PI;
    }

    public ColorRgb evaluate(
        ShadingContext context,
        PhongBidirectionalScatteringDistributionFunction inBsdf,
        PhongBidirectionalScatteringDistributionFunction outBsdf,
        Vector3D in,
        Vector3D out,
        int flags) {
        ColorRgb result = new ColorRgb();
        Vector3D normal = new Vector3D();

        result.clear();
        if (context == null || !context.hasFlag(RayHitFlag.NORMAL)) {
            Error.warning("evaluate", "Couldn't determine shading normal");
            return result;
        }
        normal.copy(context.getShadingNormal());

        if (texture != null && (flags & TEXTURED_COMPONENT) != 0) {
            double textureBsdf = texturedScattererEval(in, out, normal);
            ColorRgb textureCol = splitBsdfEvalTexture(texture, context);
            result.addScaled(result, (float)textureBsdf, textureCol);
            flags &= ~TEXTURED_COMPONENT;
        }

        if (brdf != null) {
            ColorRgb reflectionCol = brdf.evaluate(in, out, normal, BsdfComponentFlag.getBrdfFlags(flags));
            result.add(result, reflectionCol);

            RefractionIndex inIndex = new RefractionIndex();
            RefractionIndex outIndex = new RefractionIndex();
            ColorRgb refractionCol;

            if (inBsdf != null) {
                inBsdf.indexOfRefraction(inIndex);
            }
            if (outBsdf != null) {
                outBsdf.indexOfRefraction(outIndex);
            }

            if (btdf == null) {
                refractionCol = new ColorRgb();
                refractionCol.clear();
            }
            else {
                refractionCol = btdf.evaluate(inIndex, outIndex, in, out, normal, BsdfComponentFlag.getBtdfFlags(flags));
            }

            result.add(result, refractionCol);
        }

        return result;
    }

    public ColorRgb evaluate(
        RayHit hit,
        PhongBidirectionalScatteringDistributionFunction inBsdf,
        PhongBidirectionalScatteringDistributionFunction outBsdf,
        Vector3D in,
        Vector3D out,
        int flags) {
        Vector3D normal = new Vector3D();
        Vector3D texCoord = new Vector3D();
        int[] localFlags = new int[] {0};
        if (!extractHitData(hit, normal, texCoord, localFlags)) {
            ColorRgb result = new ColorRgb();
            result.clear();
            return result;
        }
        ShadingContext context = new ShadingContext(
            hit.getPoint(),
            hit.getGeometricNormal(),
            normal,
            texCoord,
            hit.getUv(),
            hit.getShadingFrame(),
            hit.getMaterial(),
            localFlags[0]);
        return evaluate(context, inBsdf, outBsdf, in, out, flags);
    }

    public void evaluateProbabilityDensityFunction(
        ShadingContext context,
        PhongBidirectionalScatteringDistributionFunction inBsdf,
        PhongBidirectionalScatteringDistributionFunction outBsdf,
        Vector3D in,
        Vector3D out,
        int flags,
        double[] probabilityDensityFunction,
        double[] probabilityDensityFunctionRR) {
        setOut(probabilityDensityFunction, 0.0);
        setOut(probabilityDensityFunctionRR, 0.0);

        Vector3D normal = new Vector3D();
        if (context == null || !context.hasFlag(RayHitFlag.NORMAL)) {
            Error.warning("evaluateProbabilityDensityFunction", "Couldn't determine shading normal");
            return;
        }
        normal.copy(context.getShadingNormal());

        double[] pTexture = new double[] {0.0};
        double[] pReflection = new double[] {0.0};
        double[] pTransmission = new double[] {0.0};
        int[] brdfFlags = new int[] {0};
        int[] btdfFlags = new int[] {0};

        splitBsdfProbabilities(context, flags, pTexture, pReflection, pTransmission, brdfFlags, btdfFlags);

        double pScattering = pTexture[0] + pReflection[0] + pTransmission[0];
        if (pScattering < Numeric.EPSILON) {
            return;
        }

        setOut(probabilityDensityFunctionRR, pScattering);

        RefractionIndex inIndex = new RefractionIndex();
        RefractionIndex outIndex = new RefractionIndex();
        if (inBsdf == null) {
            inIndex.set(1.0f, 0.0f);
        }
        else {
            inBsdf.indexOfRefraction(inIndex);
        }

        if (outBsdf == null) {
            outIndex.set(1.0f, 0.0f);
        }
        else {
            outBsdf.indexOfRefraction(outIndex);
        }

        double[] p = new double[] {0.0};
        texturedScattererEvalPdf(in, out, normal, p);
        setOut(probabilityDensityFunction, pTexture[0] * p[0]);

        if (brdf == null) {
            p[0] = 0.0;
        }
        else {
            double[] pRR = new double[] {0.0};
            brdf.evaluateProbabilityDensityFunction(in, out, normal, brdfFlags[0], p, pRR);
        }
        setOut(probabilityDensityFunction, probabilityDensityFunction[0] + pReflection[0] * p[0]);

        if (btdf == null) {
            p[0] = 0.0;
        }
        else {
            double[] pRR = new double[] {0.0};
            btdf.evaluateProbabilityDensityFunction(inIndex, outIndex, in, out, normal, btdfFlags[0], p, pRR);
        }
        setOut(probabilityDensityFunction, probabilityDensityFunction[0] + pTransmission[0] * p[0]);

        setOut(probabilityDensityFunction, probabilityDensityFunction[0] / pScattering);
    }

    public void evaluateProbabilityDensityFunction(
        RayHit hit,
        PhongBidirectionalScatteringDistributionFunction inBsdf,
        PhongBidirectionalScatteringDistributionFunction outBsdf,
        Vector3D in,
        Vector3D out,
        int flags,
        double[] probabilityDensityFunction,
        double[] probabilityDensityFunctionRR) {
        Vector3D normal = new Vector3D();
        Vector3D texCoord = new Vector3D();
        int[] localFlags = new int[] {0};
        if (!extractHitData(hit, normal, texCoord, localFlags)) {
            setOut(probabilityDensityFunction, 0.0);
            setOut(probabilityDensityFunctionRR, 0.0);
            return;
        }
        ShadingContext context = new ShadingContext(
            hit.getPoint(),
            hit.getGeometricNormal(),
            normal,
            texCoord,
            hit.getUv(),
            hit.getShadingFrame(),
            hit.getMaterial(),
            localFlags[0]);
        evaluateProbabilityDensityFunction(context, inBsdf, outBsdf, in, out, flags, probabilityDensityFunction, probabilityDensityFunctionRR);
    }

    public ColorRgb bsdfEvalComponents(
        ShadingContext context,
        PhongBidirectionalScatteringDistributionFunction inBsdf,
        PhongBidirectionalScatteringDistributionFunction outBsdf,
        Vector3D in,
        Vector3D out,
        int flags,
        ColorRgb[] colArray) {
        ColorRgb result = new ColorRgb();
        ColorRgb empty = new ColorRgb();
        empty.clear();
        result.clear();

        for (int i = 0; i < BsdfComponentInfo.BSDF_COMPONENTS; i++) {
            int thisFlag = BsdfComponentFlag.bsdfIndexToComp(i);
            if ((flags & thisFlag) != 0) {
                colArray[i] = evaluate(context, inBsdf, outBsdf, in, out, thisFlag);
                result.add(result, colArray[i]);
            }
            else {
                colArray[i] = new ColorRgb(empty.r, empty.g, empty.b);
            }
        }

        return result;
    }

    public ColorRgb bsdfEvalComponents(
        RayHit hit,
        PhongBidirectionalScatteringDistributionFunction inBsdf,
        PhongBidirectionalScatteringDistributionFunction outBsdf,
        Vector3D in,
        Vector3D out,
        int flags,
        ColorRgb[] colArray) {
        Vector3D normal = new Vector3D();
        Vector3D texCoord = new Vector3D();
        int[] localFlags = new int[] {0};
        if (!extractHitData(hit, normal, texCoord, localFlags)) {
            ColorRgb result = new ColorRgb();
            result.clear();
            return result;
        }
        ShadingContext context = new ShadingContext(
            hit.getPoint(),
            hit.getGeometricNormal(),
            normal,
            texCoord,
            hit.getUv(),
            hit.getShadingFrame(),
            hit.getMaterial(),
            localFlags[0]);
        return bsdfEvalComponents(context, inBsdf, outBsdf, in, out, flags, colArray);
    }

    private static void setOut(double[] out, double value) {
        if (out != null && out.length > 0) {
            out[0] = value;
        }
    }

    private static void setOut(int[] out, int value) {
        if (out != null && out.length > 0) {
            out[0] = value;
        }
    }
}
