package vsdk.toolkit.environment.material;

import vsdk.toolkit.common.linealAlgebra.Vector3Dd;

public interface Material
{
    Material copy();
    Material translate(Vector3Dd vector);
    Material rotate(Vector3Dd vector);
    Material scale(Vector3Dd vector);
}
