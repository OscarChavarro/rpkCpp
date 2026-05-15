export class KDQuery {
  public point: number[];
  public wantedN: number;
  public foundN: number;
  public notFilled: boolean;
  public results: unknown[];
  public distances: number[];
  public maximumDistance: number;
  public sqrRadius: number;
  public excludeFlags: number;

  public constructor() {
    this.point = [];
    this.wantedN = 0;
    this.foundN = 0;
    this.notFilled = true;
    this.results = [];
    this.distances = [];
    this.maximumDistance = 0.0;
    this.sqrRadius = 0.0;
    this.excludeFlags = 0;
  }

  public print(): void {
    process.stdout.write(`Point X ${this.point[0]}, Y ${this.point[1]}, Z ${this.point[2]}\n`);
    process.stdout.write(`Wanted N: ${this.wantedN}, found N: ${this.foundN}\n`);
    process.stdout.write(`maximumDistance ${this.maximumDistance}\n`);
    process.stdout.write(`sqrRadius ${this.sqrRadius}\n`);
    process.stdout.write(`excludeFlags ${this.excludeFlags.toString(16)}\n`);
  }
}
