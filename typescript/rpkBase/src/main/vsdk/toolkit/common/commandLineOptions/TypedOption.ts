import { DefaultParser } from "./DefaultParser";
import { OptionBase } from "./OptionBase";

export class TypedOption<T> {
  private name_: string | null;
  private target_: TypedOption.Reference<T> | null;
  private consumesValue_: number;
  private onSet_: TypedOption.OnSet<T> | null;
  private parseArgs_: TypedOption.ParseArgs<T> | null;
  private validate_: TypedOption.Validate<T> | null;
  private transform_: TypedOption.Transform<T> | null;

  public constructor();
  public constructor(
    name: string,
    target: TypedOption.Reference<T>,
    consumesValue: number,
    onSet: TypedOption.OnSet<T> | null,
    parseArgs: TypedOption.ParseArgs<T> | null
  );
  public constructor(
    name: string,
    target: TypedOption.Reference<T>,
    consumesValue: number,
    onSet: TypedOption.OnSet<T> | null,
    parseArgs: TypedOption.ParseArgs<T> | null,
    validate: TypedOption.Validate<T> | null,
    transform: TypedOption.Transform<T> | null
  );
  public constructor(
    name?: string,
    target?: TypedOption.Reference<T>,
    consumesValue = 1,
    onSet: TypedOption.OnSet<T> | null = null,
    parseArgs: TypedOption.ParseArgs<T> | null = null,
    validate: TypedOption.Validate<T> | null = null,
    transform: TypedOption.Transform<T> | null = null
  ) {
    this.name_ = name ?? null;
    this.target_ = target ?? null;
    this.consumesValue_ = consumesValue;
    this.onSet_ = onSet;
    this.parseArgs_ = parseArgs;
    this.validate_ = validate;
    this.transform_ = transform;

  }

  public static valueRef<T>(initialValue: T): TypedOption.ValueRef<T> {
    return new TypedOption.ValueRef<T>(initialValue);
  }

  public static reference<T>(getter: () => T, setter: (value: T) => void): TypedOption.Reference<T> {
    return {
      get: (): T => getter(),
      set: (value: T): void => setter(value),
    };
  }

  public getName(): string | null {
    return this.name_;
  }

  public getConsumesValue(): number {
    return this.consumesValue_;
  }

  public apply(argc: number, argv: string[] | null): boolean {
    let target = this.target_;


    if (target === null) {
      return false;
    }

    const value = new TypedOption.MutableValue<T>(target.get());

    if (this.consumesValue_ === 0) {
      if (this.parseArgs_ !== null && !this.parseArgs_(0, null, value)) {
        return false;
      }
      if (this.validate_ !== null && !this.validate_(value)) {
        return false;
      }
      if (this.transform_ !== null) {
        this.transform_(value);
      }

      target.set(value.value);

      if (this.onSet_ !== null) {
        const targetValue = new TypedOption.MutableValue<T>(target.get());
        this.onSet_(targetValue);
        target.set(targetValue.value);
      }
      return true;
    }

    if (argc < this.consumesValue_) {
      return false;
    }

    let parsed = false;
    if (this.parseArgs_ !== null) {
      parsed = this.parseArgs_(this.consumesValue_, argv, value);
    }
    else if (this.consumesValue_ === 1 && argv !== null && argv.length > 0) {
      const firstArg = argv[0];
      if (firstArg === undefined) {
        return false;
      }
      parsed = DefaultParser.parse(firstArg, value);
    }

    if (!parsed) {
      return false;
    }

    if (this.validate_ !== null && !this.validate_(value)) {
      return false;
    }
    if (this.transform_ !== null) {
      this.transform_(value);
    }

    target.set(value.value);
    if (this.onSet_ !== null) {
      const targetValue = new TypedOption.MutableValue<T>(target.get());
      this.onSet_(targetValue);
      target.set(targetValue.value);
    }

    return true;
  }

  public static matchOption(input: string | null, name: string | null, abbrLen: number): boolean {
    if (input === null || name === null) {
      return false;
    }
    if (input === name) {
      return true;
    }
    if (abbrLen <= 0) {
      return false;
    }

    const inputLength = input.length;
    if (inputLength > abbrLen) {
      return false;
    }

    return name.startsWith(input);
  }

  public static registerOption<T>(optionInstance: TypedOption<T>, abbr: number): OptionBase {
    return new OptionBase(optionInstance, abbr);
  }

  public static REGISTER_OPTION<T>(optionInstance: TypedOption<T>, abbr: number): OptionBase {
    return TypedOption.registerOption(optionInstance, abbr);
  }
}

export namespace TypedOption {
  export interface Reference<T> {
    get(): T;
    set(value: T): void;
  }

  export class ValueRef<T> implements Reference<T> {
    private value_: T;

    public constructor(initialValue: T) {
      this.value_ = initialValue;
    }

    public get(): T {
      return this.value_;
    }

    public set(value: T): void {
      this.value_ = value;
    }
  }

  export class MutableValue<T> {
    public value: T;

    public constructor(value: T) {
      this.value = value;
    }
  }

  export type OnSet<T> = (value: MutableValue<T>) => void;
  export type ParseArgs<T> = (argc: number, argv: string[] | null, value: MutableValue<T>) => boolean;
  export type Validate<T> = (value: MutableValue<T>) => boolean;
  export type Transform<T> = (value: MutableValue<T>) => void;
}
