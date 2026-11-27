import { PrintStream } from "../../../../java/io/PrintStream.js";
import { Vector3Dd } from "../../common/linealAlgebra/Vector3Dd.js";

export class Vector3DPrinter {
    public static print(vector: Vector3Dd, stream: PrintStream | null): void {
        if (stream === null) {
            return;
        }
        stream.printf("%g %g %g", vector.x(), vector.y(), vector.z());
    }
}
