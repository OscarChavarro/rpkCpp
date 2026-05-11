import { ArrayList } from "../../../../java/util/ArrayList";
import { ColorRgb } from "../../common/color/ColorRgb";
import { Logger as VsdkLogger } from "../../common/logging/Logger";
import { CircularList } from "../../common/dataStructures/CircularList";
import { CircularListIterator } from "../../common/dataStructures/CircularListIterator";
import { Numeric } from "../../common/linealAlgebra/Numeric";
import { Vector3D } from "../../common/linealAlgebra/Vector3D";
import { XxdfComponentFlag } from "../../material/XxdfComponentFlag";
import { PatchVisitor } from "../../numericalAnalysis/PatchVisitor";
import { Patch } from "../../environment/geometry/elements/Patch";
import { LightInfo } from "./LightInfo";

export class LightList extends CircularList<LightInfo> {
  private totalFlux: number;
  private totalImp: number;
  private includeVirtual: boolean;
  private lightCount: number;

  private lastPoint: Vector3D;
  private lastNormal: Vector3D;

  public constructor(list: ArrayList<Patch>, includeVirtualPatches = false) {
    super();

    this.totalFlux = 0.0;
    this.lightCount = 0;
    this.includeVirtual = includeVirtualPatches;
    this.totalImp = 0.0;

    this.lastPoint = new Vector3D();
    this.lastNormal = new Vector3D();

    for (let i = 0; list !== null && i < list.size(); i++) {
      const light = list.get(i);
      const hasEdf = light.material !== null && light.material.getEdf() !== null;
      if ((!light.hasZeroVertices() || this.includeVirtual) && hasEdf) {
        const info = new LightInfo();
        info.light = light;

        if (light.hasZeroVertices()) {
          let e = new ColorRgb();
          if (light.material === null || light.material.getEdf() === null) {
            e.clear();
          }
          else {
            e = light.material.getEdf()!.phongEmittance(null as any, XxdfComponentFlag.DIFFUSE_COMPONENT);
          }
          info.emittedFlux = e.average();
        }
        else {
          const lightColor = PatchVisitor.averageEmittance(light, XxdfComponentFlag.DIFFUSE_COMPONENT);
          info.emittedFlux = lightColor.average() * light.area;
        }

        this.totalFlux += info.emittedFlux;
        this.lightCount++;
        this.append(info);
      }
    }
  }

  public entries(): CircularList<LightInfo> {
    return this;
  }

  public sample(x1: number[], pdf: number[] | null): Patch | null {
    let lastInfo: LightInfo | null = null;
    const iterator = new CircularListIterator<LightInfo>(this);

    const rnd = x1[0] * this.totalFlux;

    let info = iterator.nextOnSequence();
    while (info !== null && info.light !== null && info.light.hasZeroVertices() && !this.includeVirtual) {
      info = iterator.nextOnSequence();
    }

    if (info === null) {
      VsdkLogger.warning("CLightList::sample", "No lights available");
      return null;
    }

    let currentSum = info.emittedFlux;

    while (rnd > currentSum && info !== null) {
      lastInfo = info;
      info = iterator.nextOnSequence();
      while (info !== null && info.light !== null && info.light.hasZeroVertices() && !this.includeVirtual) {
        info = iterator.nextOnSequence();
      }

      if (info !== null) {
        currentSum += info.emittedFlux;
      }
      else {
        info = lastInfo;
        currentSum = currentSum - 1.0;
      }
    }

    if (info !== null) {
      x1[0] = (x1[0] - ((currentSum - info.emittedFlux) / this.totalFlux))
        / (info.emittedFlux / this.totalFlux);
      if (pdf !== null && pdf.length > 0) {
        pdf[0] = info.emittedFlux / this.totalFlux;
      }
      return info.light;
    }

    return null;
  }

  private evalPdfVirtual(light: Patch, point: Vector3D): number {
    void point;
    let probabilityDensityFunction: number;

    const all = XxdfComponentFlag.DIFFUSE_COMPONENT | XxdfComponentFlag.GLOSSY_COMPONENT | XxdfComponentFlag.SPECULAR_COMPONENT;

    let e: ColorRgb;

    if (light.material === null || light.material.getEdf() === null) {
      e = new ColorRgb();
      e.clear();
    }
    else {
      e = light.material.getEdf()!.phongEmittance(null as any, all);
    }
    probabilityDensityFunction = e.average() / this.totalFlux;

    return probabilityDensityFunction;
  }

  private evalPdfReal(light: Patch, point: Vector3D): number {
    void point;
    const color = PatchVisitor.averageEmittance(light, XxdfComponentFlag.DIFFUSE_COMPONENT);

    return color.average() * light.area / this.totalFlux;
  }

  public evalPdf(light: Patch, point: Vector3D): number {
    if (this.totalFlux < Numeric.EPSILON) {
      return 0.0;
    }
    if (light.hasZeroVertices()) {
      return this.evalPdfVirtual(light, point);
    }
    return this.evalPdfReal(light, point);
  }

  public static computeOneLightImportanceVirtual(
    light: Patch,
    point: Vector3D,
    normal: Vector3D,
    emittedFlux: number
  ): number {
    void point;
    void normal;
    void emittedFlux;
    const all = XxdfComponentFlag.DIFFUSE_COMPONENT | XxdfComponentFlag.GLOSSY_COMPONENT | XxdfComponentFlag.SPECULAR_COMPONENT;

    let e: ColorRgb;

    if (light.material === null || light.material.getEdf() === null) {
      e = new ColorRgb();
      e.clear();
    }
    else {
      e = light.material.getEdf()!.phongEmittance(null as any, all);
    }
    return e.average();
  }

  public static computeOneLightImportanceReal(
    light: Patch,
    point: Vector3D,
    normal: Vector3D,
    emittedFlux: number
  ): number {
    let tried = 0;
    let done = false;
    let contribution = 0.0;
    const lightPoint = new Vector3D();
    const lightNormal = new Vector3D();
    const dir = new Vector3D();

    while (!done && tried <= light.numberOfVertices) {
      if (tried === 0) {
        lightPoint.copy(light.midPoint);
        lightNormal.copy(light.normal);
      }
      else {
        lightPoint.copy(light.vertex[tried - 1]!.point);
        if (light.vertex[tried - 1]!.normal !== null) {
          lightNormal.copy(light.vertex[tried - 1]!.normal as Vector3D);
        }
        else {
          lightNormal.copy(light.normal);
        }
      }

      const copy = new Vector3D(point.x, point.y, point.z);

      dir.subtraction(lightPoint, copy);
      const dist2 = dir.norm2();

      const cosRayLight = -dir.dotProduct(lightNormal);
      const cosRayPatch = dir.dotProduct(normal);

      if (cosRayLight > 0 && cosRayPatch > 0) {
        contribution = cosRayPatch * cosRayLight * emittedFlux / (globalThis.Math.PI * dist2);
        done = true;
      }

      tried++;
    }

    return contribution;
  }

  public static computeOneLightImportance(
    light: Patch,
    point: Vector3D,
    normal: Vector3D,
    emittedFlux: number
  ): number {
    if (light.hasZeroVertices()) {
      return LightList.computeOneLightImportanceVirtual(light, point, normal, emittedFlux);
    }
    return LightList.computeOneLightImportanceReal(light, point, normal, emittedFlux);
  }

  public computeLightImportance(point: Vector3D, normal: Vector3D): void {
    if (point.equals(this.lastPoint, Numeric.EPSILON_FLOAT)
      && normal.equals(this.lastNormal, Numeric.EPSILON_FLOAT)) {
      return;
    }

    const iterator = new CircularListIterator<LightInfo>(this);

    this.totalImp = 0.0;

    let info = iterator.nextOnSequence();
    while (info !== null && info.light !== null && info.light.hasZeroVertices() && !this.includeVirtual) {
      info = iterator.nextOnSequence();
    }

    while (info !== null) {
      const imp = LightList.computeOneLightImportance(info.light as Patch, point, normal, info.emittedFlux);
      this.totalImp += imp;
      info.importance = imp;

      info = iterator.nextOnSequence();
      while (info !== null && info.light !== null && info.light.hasZeroVertices() && !this.includeVirtual) {
        info = iterator.nextOnSequence();
      }
    }

    this.lastPoint.copy(point);
    this.lastNormal.copy(normal);
  }

  public sampleImportant(point: Vector3D, normal: Vector3D, x1: number[], pdf: number[] | null): Patch | null {
    let lastInfo: LightInfo | null = null;
    const iterator = new CircularListIterator<LightInfo>(this);

    this.computeLightImportance(point, normal);

    if (this.totalImp === 0) {
      return this.sample(x1, pdf);
    }

    const rnd = x1[0] * this.totalImp;

    let info = iterator.nextOnSequence();
    while (info !== null && info.light !== null && info.light.hasZeroVertices() && !this.includeVirtual) {
      info = iterator.nextOnSequence();
    }

    if (info === null) {
      VsdkLogger.warning("CLightList::sample", "No lights available");
      return null;
    }

    let currentSum = info.importance;

    while (rnd > currentSum && info !== null) {
      lastInfo = info;

      info = iterator.nextOnSequence();
      while (info !== null && info.light !== null && info.light.hasZeroVertices() && !this.includeVirtual) {
        info = iterator.nextOnSequence();
      }

      if (info !== null) {
        currentSum += info.importance;
      }
      else {
        info = lastInfo;
        currentSum = currentSum - 1.0;
      }
    }

    if (info !== null) {
      x1[0] = (x1[0] - ((currentSum - info.importance) / this.totalImp))
        / (info.importance / this.totalImp);
      if (pdf !== null && pdf.length > 0) {
        pdf[0] = info.importance / this.totalImp;
      }
      return info.light;
    }

    return null;
  }

  public evalPdfImportant(
    light: Patch,
    lightPoint: Vector3D,
    litPoint: Vector3D,
    normal: Vector3D
  ): number {
    void lightPoint;
    this.computeLightImportance(litPoint, normal);

    const iterator = new CircularListIterator<LightInfo>(this);

    let info: LightInfo | null;
    do {
      info = iterator.nextOnSequence();
    } while (info !== null && info.light !== light);

    if (info === null) {
      VsdkLogger.warning("CLightList::evalPdfImportant", "Could not find light");
      return 0.0;
    }

    let pdf: number;
    if (this.totalImp < Numeric.EPSILON) {
      pdf = 0.0;
    }
    else {
      pdf = info.importance / this.totalImp;
    }

    return pdf;
  }
}
