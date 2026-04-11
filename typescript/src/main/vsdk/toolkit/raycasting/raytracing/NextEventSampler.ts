import { Sampler } from "./Sampler";

export abstract class NextEventSampler extends Sampler {
  public ActivateFirstUnit(): boolean {
    return false;
  }

  public ActivateNextUnit(): boolean {
    return false;
  }
}
