export interface OptionValue {
  getName(): string | null;
  getConsumesValue(): number;
  apply(argc: number, argv: string[] | null): boolean;
}

export class OptionBase {
  public constructor(private readonly option: OptionValue | null = null,
                     private readonly abbreviationLength = 0) {}
  public isConfigured(): boolean { return this.option !== null && this.option.getName() !== null; }
  public getName(): string | null { return this.option?.getName() ?? null; }
  public getAbbreviationLength(): number { return this.abbreviationLength; }
  public consumesValue(): number { return this.option?.getConsumesValue() ?? 1; }
  public apply(argc: number, argv: string[] | null): boolean {
    return this.option !== null && this.option.apply(argc, argv);
  }
}
