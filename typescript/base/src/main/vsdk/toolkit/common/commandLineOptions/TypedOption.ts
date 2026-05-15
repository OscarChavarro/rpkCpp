import { DefaultParser } from "./DefaultParser";
import { OptionBase } from "./OptionBase";

export class TypedOption<T> {
  private name_: string | null;
  private target_: TypedOption.Reference<T> | null;
  private offset_: number;
  private useOffset_: boolean;
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
    name: string,
    offset: number,
    consumesValue: number,
    onSet: TypedOption.OnSet<T> | null,
    parseArgs: TypedOption.ParseArgs<T> | null
  );
  public constructor(
    name: string,
    offset: number,
    consumesValue: number,
    onSet: TypedOption.OnSet<T> | null,
    parseArgs: TypedOption.ParseArgs<T> | null,
    validate: TypedOption.Validate<T> | null,
    transform: TypedOption.Transform<T> | null
  );
  public constructor(
    name?: string,
    targetOrOffset?: TypedOption.Reference<T> | number,
    consumesValue = 1,
    onSet: TypedOption.OnSet<T> | null = null,
    parseArgs: TypedOption.ParseArgs<T> | null = null,
    validate: TypedOption.Validate<T> | null = null,
    transform: TypedOption.Transform<T> | null = null
  ) {
    this.name_ = name ?? null;
    this.target_ = null;
    this.offset_ = 0;
    this.useOffset_ = false;
    this.consumesValue_ = consumesValue;
    this.onSet_ = onSet;
    this.parseArgs_ = parseArgs;
    this.validate_ = validate;
    this.transform_ = transform;

    if (typeof targetOrOffset === "number") {
      this.offset_ = targetOrOffset;
      this.useOffset_ = true;
    }
    else if (targetOrOffset !== undefined) {
      this.target_ = targetOrOffset;
    }
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

  public apply(context: unknown, argc: number, argv: string[] | null): boolean {
    let target = this.target_;

    if (this.useOffset_) {
      if (
        context === null
        || context === undefined
        || typeof (context as TypedOption.ContextBinding).resolve !== "function"
      ) {
        return false;
      }
      target = (context as TypedOption.ContextBinding).resolve<T>(this.offset_);
    }

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
      parsed = DefaultParser.parse(argv[0], value);
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

  public static applyOption<T>(
    opt: TypedOption<T>,
    context: unknown,
    argc: number,
    argv: string[] | null
  ): boolean {
    return opt.apply(context, argc, argv);
  }

  public static applyAdapter<T>(
    opt: unknown,
    context: unknown,
    argc: number,
    argv: string[] | null
  ): boolean {
    if (opt === null || opt === undefined) {
      return false;
    }
    return TypedOption.applyOption(opt as TypedOption<T>, context, argc, argv);
  }

  public static consumesValueAdapter<T>(opt: unknown): number {
    if (opt === null || opt === undefined) {
      return 1;
    }
    return (opt as TypedOption<T>).getConsumesValue();
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
    return new OptionBase(
      optionInstance.getName() ?? "",
      abbr,
      TypedOption.consumesValueAdapter,
      TypedOption.applyAdapter,
      optionInstance
    );
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

  export interface ContextBinding {
    resolve<T>(offset: number): Reference<T> | null;
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
