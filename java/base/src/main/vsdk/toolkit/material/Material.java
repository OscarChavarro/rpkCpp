package vsdk.toolkit.material;

// Emittance distribution function

// Reflection and transmission together

// True for 1-sided surface, false for 2-sided, see mgf docs

// Material name

public class Material {
    private PhongEmittanceDistributionFunction edf;
    private PhongBidirectionalScatteringDistributionFunction bsdf;
    private boolean sided;
    private String name;

    public Material(
        String inName,
        PhongEmittanceDistributionFunction inEdf,
        PhongBidirectionalScatteringDistributionFunction inBsdf,
        boolean inSided) {
        name = inName == null ? "" : inName;
        sided = inSided;
        edf = inEdf;
        bsdf = inBsdf;
    }

    public PhongEmittanceDistributionFunction getEdf() {
        return edf;
    }

    public PhongBidirectionalScatteringDistributionFunction getBsdf() {
        return bsdf;
    }

    public boolean isSided() {
        return sided;
    }

    public String getName() {
        return name;
    }
}
