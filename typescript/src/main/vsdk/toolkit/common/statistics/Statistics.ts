import { PotentialStatistics } from "./PotentialStatistics";
import { RadianceStatistics } from "./RadianceStatistics";
import { RayTracerStatistics } from "./RayTracerStatistics";
import { ReaderStatistics } from "./ReaderStatistics";
import { ShadowStatistics } from "./ShadowStatistics";

export class Statistics {
  public reader: ReaderStatistics;
  public radiance: RadianceStatistics;
  public potential: PotentialStatistics;
  public shadow: ShadowStatistics;
  public rayTracer: RayTracerStatistics;

  private static instanceValue: Statistics | null = null;

  public constructor() {
    this.reader = new ReaderStatistics();
    this.radiance = new RadianceStatistics();
    this.potential = new PotentialStatistics();
    this.shadow = new ShadowStatistics();
    this.rayTracer = new RayTracerStatistics();
  }

  public static instance(): Statistics {
    if (Statistics.instanceValue === null) {
      Statistics.instanceValue = new Statistics();
    }
    return Statistics.instanceValue;
  }
}
