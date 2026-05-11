/**
Definition of the light list class
this class can be used for sampling lights
*/

package vsdk.toolkit.raycasting.bidirectionalRaytracing;

import java.util.ArrayList;

import vsdk.toolkit.common.color.ColorRgb;
import vsdk.toolkit.common.logging.Logger;
import vsdk.toolkit.common.dataStructures.CircularList;
import vsdk.toolkit.common.dataStructures.CircularListIterator;
import vsdk.toolkit.common.linealAlgebra.Numeric;
import vsdk.toolkit.common.linealAlgebra.Vector3D;
import vsdk.toolkit.material.XxdfComponentFlag;
import vsdk.toolkit.numericalAnalysis.PatchVisitor;
import vsdk.toolkit.environment.geometry.elements.Patch;
import vsdk.toolkit.environment.geometry.elements.RayHit;

public class LightList extends CircularList<LightInfo> {
    // Total flux ( sum(L * A * PI))
    private float totalFlux;
    private float totalImp;
    private boolean includeVirtual;
    private int lightCount;

    // Iteration over lights, not multi-thread!

    // Discrete sampling of light sources

    // A getPatchList must be supplied for building a light list.
    // Non emitting patches (edf == nullptr) are NOT put in the list.
    public LightList(ArrayList<Patch> list, boolean includeVirtualPatches) {
        LightInfo info = new LightInfo();
        ColorRgb lightColor;

        totalFlux = 0.0f;
        lightCount = 0;
        includeVirtual = includeVirtualPatches;
        totalImp = 0.0f;

        for ( int i = 0; list != null && i < list.size(); i++ ) {
            Patch light = list.get(i);
            if ( (!light.hasZeroVertices() || includeVirtual)
                && ( light.material.getEdf() != null ) ) {
                info = new LightInfo();
                info.light = light;

                // calc emittedFlux
                if ( light.hasZeroVertices() ) {
                    ColorRgb e;
                    if ( light.material.getEdf() == null ) {
                        e = new ColorRgb();
                        e.clear();
                    } else {
                        e = light.material.getEdf().phongEmittance((RayHit)null, XxdfComponentFlag.DIFFUSE_COMPONENT);
                    }
                    info.emittedFlux = e.average();
                } else {
                    lightColor = PatchVisitor.averageEmittance(light, XxdfComponentFlag.DIFFUSE_COMPONENT);
                    info.emittedFlux = lightColor.average() * light.area;
                }

                totalFlux += info.emittedFlux;
                lightCount++;
                append(info);
            }
        }
    }

    public LightList(ArrayList<Patch> list) {
        this(list, false);
    }

    public CircularList<LightInfo> entries() {
        return this;
    }

    /**
    Returns sampled patch, scales x_1 back to a random in 0..1
    */
    public Patch sample(double[] x1, double[] pdf) {
        LightInfo info;
        LightInfo lastInfo = null;
        CircularListIterator<LightInfo> iterator = new CircularListIterator<>(this);

        double rnd = x1[0] * totalFlux;
        double currentSum;

        info = iterator.nextOnSequence();
        while ( (info != null) && (info.light.hasZeroVertices()) && (!includeVirtual) ) {
            info = iterator.nextOnSequence();
        }

        if ( info == null ) {
            Logger.warning("CLightList::sample", "No lights available");
            return null;
        }

        currentSum = info.emittedFlux;

        while ( (rnd > currentSum) && (info != null) ) {
            lastInfo = info;
            info = iterator.nextOnSequence();
            while ( (info != null) && (info.light.hasZeroVertices()) && (!includeVirtual) ) {
                info = iterator.nextOnSequence();
            }

            if ( info != null ) {
                currentSum += info.emittedFlux;
            } else {
                info = lastInfo;
                rnd = currentSum - 1.0; // :-(  Damn float inaccuracies
            }
        }

        if ( info != null ) {
            x1[0] = ((x1[0] - ((currentSum - info.emittedFlux) / totalFlux)) /
                (info.emittedFlux / totalFlux));
            if ( pdf != null && pdf.length > 0 ) {
                pdf[0] = info.emittedFlux / totalFlux;
            }
            return info.light;
        }

        return null;
    }

    private double evalPdfVirtual(Patch light, Vector3D point) {
        // EvalPDF for virtual patches (see EvalPDF)
        double probabilityDensityFunction;

        // Prob for choosing this light
        int all = XxdfComponentFlag.DIFFUSE_COMPONENT | XxdfComponentFlag.GLOSSY_COMPONENT | XxdfComponentFlag.SPECULAR_COMPONENT;

        ColorRgb e;
        if ( light.material.getEdf() == null ) {
            e = new ColorRgb();
            e.clear();
        } else {
            e = light.material.getEdf().phongEmittance((RayHit)null, all);
        }
        probabilityDensityFunction = e.average() / totalFlux;

        return probabilityDensityFunction;
    }

    private double evalPdfReal(Patch light, Vector3D point) {
        // Eval PDF for normal patches (see EvalPDF)
        ColorRgb color;
        double pdf;

        color = PatchVisitor.averageEmittance(light, XxdfComponentFlag.DIFFUSE_COMPONENT);

        // Prob for choosing this light
        pdf = color.average() * light.area / totalFlux;

        return pdf;
    }

    public double evalPdf(Patch light, Vector3D point) {
        // TODO!!!  1) patch should become class
        //          2) virtual patch should become child-class
        //          3) this method should be handled by specialisation
        if ( totalFlux < Numeric.EPSILON ) {
            return 0.0;
        }
        if ( light.hasZeroVertices() ) {
            return evalPdfVirtual(light, point);
        } else {
            return evalPdfReal(light, point);
        }
    }

    /* Important light sampling */

    public static double
    computeOneLightImportanceVirtual(
        Patch light,
        Vector3D point,
        Vector3D normal,
        float emittedFlux)
    {
        // ComputeOneLightImportance for virtual patches
        int all = XxdfComponentFlag.DIFFUSE_COMPONENT | XxdfComponentFlag.GLOSSY_COMPONENT | XxdfComponentFlag.SPECULAR_COMPONENT;

        ColorRgb e;

        if ( light.material.getEdf() == null ) {
            e = new ColorRgb();
            e.clear();
        } else {
            e = light.material.getEdf().phongEmittance((RayHit)null, all);
        }
        return e.average();
    }

    public static double
    computeOneLightImportanceReal(
        Patch light,
        Vector3D point,
        Vector3D normal,
        float emittedFlux)
    {
        // ComputeOneLightImportance for real patches
        int tried = 0;  // No positions on the patch are tried yet
        boolean done = false;
        double contribution = 0.0;
        Vector3D lightPoint = new Vector3D();
        Vector3D lightNormal = new Vector3D();
        Vector3D dir = new Vector3D();
        double cosRayPatch;
        double cosRayLight;
        double dist2;

        while ( !done && tried <= light.numberOfVertices ) {
            // Choose a point on the patch according to 'tried'

            if ( tried == 0 ) {
                lightPoint.copy(light.midPoint);
                lightNormal.copy(light.normal);
            } else {
                lightPoint.copy(light.vertex[tried - 1].point);
                if ( light.vertex[tried - 1].normal != null ) {
                    lightNormal.copy(light.vertex[tried - 1].normal);
                } else {
                    lightNormal.copy(light.normal);
                }
            }

            // Estimate the contribution

            // Ray direction (but no ray is shot of course)
            Vector3D copy = new Vector3D(point.x, point.y, point.z);

            dir.subtraction(lightPoint, copy);
            dist2 = dir.norm2();

            // Cosines have an addition distance length in them
            cosRayLight = -dir.dotProduct(lightNormal);
            cosRayPatch = dir.dotProduct(normal);

            if ( cosRayLight > 0 && cosRayPatch > 0 ) {
                // Orientation of surfaces ok.
                // BRDF is not taken into account since
                // we're expecting diffuse/glossy surfaces here.

                contribution = (cosRayPatch * cosRayLight * emittedFlux / (Math.PI * dist2));
                done = true;
            }

            tried++; // Try next point on light
        }

        return contribution;
    }

    public static double
    computeOneLightImportance(
        Patch light,
        Vector3D point,
        Vector3D normal,
        float emittedFlux)
    {
        // TODO!!!  1) patch should become class
        //          2) virtual patch should become child-class
        //          3) this method should be handled by specialisation
        if ( light.hasZeroVertices() ) {
            return computeOneLightImportanceVirtual(light, point, normal, emittedFlux);
        } else {
            return computeOneLightImportanceReal(light, point, normal, emittedFlux);
        }
    }

    private Vector3D lastPoint = new Vector3D();
    private Vector3D lastNormal = new Vector3D();

    public void computeLightImportance(Vector3D point, Vector3D normal) {
        if ( (point.equals(lastPoint, Numeric.EPSILON_FLOAT)) &&
            (normal.equals(lastNormal, Numeric.EPSILON_FLOAT)) ) {
            return; // Still ok !!
        }


        LightInfo info;
        CircularListIterator<LightInfo> iterator = new CircularListIterator<>(this);
        double imp;

        totalImp = 0.0f;

        // next
        info = iterator.nextOnSequence();
        while ( (info != null) && (info.light.hasZeroVertices()) && (!includeVirtual) ) {
            info = iterator.nextOnSequence();
        }

        while ( info != null ) {
            imp = computeOneLightImportance(info.light, point, normal, info.emittedFlux);
            totalImp += (float)imp;
            info.importance = (float)imp;

            // next
            info = iterator.nextOnSequence();
            while ( (info != null) && (info.light.hasZeroVertices()) && (!includeVirtual) ) {
                info = iterator.nextOnSequence();
            }
        }
    }

    public Patch sampleImportant(Vector3D point, Vector3D normal, double[] x1, double[] pdf) {
        LightInfo info;
        LightInfo lastInfo = null;
        CircularListIterator<LightInfo> iterator = new CircularListIterator<>(this);
        double rnd;
        double currentSum;

        computeLightImportance(point, normal);

        if ( totalImp == 0 ) {
            // No light is important, but we must return one (->optimize ?)
            return sample(x1, pdf);
        }

        rnd = x1[0] * totalImp;

        // Next
        info = iterator.nextOnSequence();
        while ( (info != null) && (info.light.hasZeroVertices()) && (!includeVirtual) ) {
            info = iterator.nextOnSequence();
        }

        if ( info == null ) {
            Logger.warning("CLightList::sample", "No lights available");
            return null;
        }

        currentSum = info.importance;

        while ( (rnd > currentSum) && (info != null) ) {
            lastInfo = info;

            // next
            info = iterator.nextOnSequence();
            while ( info != null && info.light.hasZeroVertices() && !includeVirtual ) {
                info = iterator.nextOnSequence();
            }

            if ( info != null ) {
                currentSum += info.importance;
            } else {
                info = lastInfo;
                rnd = currentSum - 1.0; // :-(  Damn float inaccuracies
            }
        }

        if ( info != null ) {
            x1[0] = ((x1[0] - ((currentSum - info.importance) / totalImp)) /
                (info.importance / totalImp));
            if ( pdf != null && pdf.length > 0 ) {
                pdf[0] = info.importance / totalImp;
            }
            return info.light;
        }

        return null;
    }

    public double
    evalPdfImportant(
        Patch light,
        Vector3D lightPoint,
        Vector3D litPoint,
        Vector3D normal)
    {
        double pdf;
        LightInfo info;
        CircularListIterator<LightInfo> iterator = new CircularListIterator<>(this);

        computeLightImportance(litPoint, normal);

        // Search the light in the list :-(

        do {
            info = iterator.nextOnSequence();
        } while ( info != null && info.light != light );

        if ( info == null ) {
            Logger.warning("CLightList::evalPdfImportant", "Could not find light");
            return 0.0;
        }

        // Prob for choosing this light
        if ( totalImp < Numeric.EPSILON ) {
            pdf = 0.0;
        } else {
            pdf = info.importance / totalImp;
        }

        return pdf;
    }
}
