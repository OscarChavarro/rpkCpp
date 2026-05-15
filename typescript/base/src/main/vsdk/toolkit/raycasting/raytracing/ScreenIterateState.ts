export class ScreenIterateState {
  public lastTime: bigint;
  public wakeUp: number;

  public constructor() {
    this.lastTime = 0n;
    this.wakeUp = 0;
  }
}
