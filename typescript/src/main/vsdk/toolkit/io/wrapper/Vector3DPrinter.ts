import { PrintStream } from "../../../../java/io/PrintStream";
import { Vector3D } from "../../common/linealAlgebra/Vector3D";

export class Vector3DPrinter {
  private static toGeneralNumber(value: number): string {
    return Number.isFinite(value) ? value.toString() : `${value}`;
  }

  public static print(vector: Vector3D, stream: PrintStream | null): void {
    if (stream === null) {
      return;
    }
    stream.printf(
      "%s %s %s",
      Vector3DPrinter.toGeneralNumber(vector.x),
      Vector3DPrinter.toGeneralNumber(vector.y),
      Vector3DPrinter.toGeneralNumber(vector.z)
    );
  }
}
