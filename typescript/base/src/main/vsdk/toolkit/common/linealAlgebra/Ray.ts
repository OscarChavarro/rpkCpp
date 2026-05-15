import { Vector3D } from "./Vector3D";

export class Ray {
  public position: Vector3D;
  public direction: Vector3D;

  public constructor(position?: Vector3D, direction?: Vector3D) {
    this.position = position ?? new Vector3D();
    this.direction = direction ?? new Vector3D();
  }
}
