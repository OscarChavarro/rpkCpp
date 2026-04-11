export class EnumDesc {
  public value: number;
  public name: string | null;
  public abbrev: number;

  public constructor();
  public constructor(value: number, name: string | null, abbrev: number);
  public constructor(value = 0, name: string | null = null, abbrev = 0) {
    this.value = value;
    this.name = name;
    this.abbrev = abbrev;
  }
}
