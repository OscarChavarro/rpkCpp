export type ConsumesValueFunction = (option: unknown) => number;
export type ApplyFunction = (
  option: unknown,
  context: unknown,
  argc: number,
  argv: string[] | null
) => boolean;

export class OptionBase {
  private name_: string | null;
  private abbreviationLength_: number;
  private consumesValue_: ConsumesValueFunction | null;
  private apply_: ApplyFunction | null;
  private option_: unknown;

  public constructor();
  public constructor(
    name: string,
    abbreviationLength: number,
    consumesValue: ConsumesValueFunction,
    apply: ApplyFunction,
    option: unknown
  );
  public constructor(
    name?: string,
    abbreviationLength = 0,
    consumesValue: ConsumesValueFunction | null = null,
    apply: ApplyFunction | null = null,
    option: unknown = null
  ) {
    this.name_ = name ?? null;
    this.abbreviationLength_ = abbreviationLength;
    this.consumesValue_ = consumesValue;
    this.apply_ = apply;
    this.option_ = option;
  }

  public isConfigured(): boolean {
    return this.name_ !== null && this.apply_ !== null;
  }

  public getName(): string | null {
    return this.name_;
  }

  public getAbbreviationLength(): number {
    return this.abbreviationLength_;
  }

  public consumesValue(): number {
    if (this.consumesValue_ === null) {
      return 1;
    }
    return this.consumesValue_(this.option_);
  }

  public apply(context: unknown, argc: number, argv: string[] | null): boolean {
    if (this.apply_ === null) {
      return false;
    }
    return this.apply_(this.option_, context, argc, argv);
  }
}
