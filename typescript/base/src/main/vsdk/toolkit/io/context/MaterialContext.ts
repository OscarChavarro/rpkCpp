import { ColorContext } from "./ColorContext";

export class MaterialContext {
  public clock: number;
  public sided: boolean;
  public nr: number;
  public ni: number;
  public rd: number;
  public rd_c: ColorContext;
  public td: number;
  public td_c: ColorContext;
  public ed: number;
  public ed_c: ColorContext;
  public rs: number;
  public rs_c: ColorContext;
  public rs_a: number;
  public ts: number;
  public ts_c: ColorContext;
  public ts_a: number;

  public constructor() {
    this.clock = 0;
    this.sided = false;
    this.nr = 0.0;
    this.ni = 0.0;
    this.rd = 0.0;
    this.rd_c = new ColorContext();
    this.td = 0.0;
    this.td_c = new ColorContext();
    this.ed = 0.0;
    this.ed_c = new ColorContext();
    this.rs = 0.0;
    this.rs_c = new ColorContext();
    this.rs_a = 0.0;
    this.ts = 0.0;
    this.ts_c = new ColorContext();
    this.ts_a = 0.0;
  }

  public copy(source: MaterialContext | null): void {
    if (source === null) {
      return;
    }
    this.clock = source.clock;
    this.sided = source.sided;
    this.nr = source.nr;
    this.ni = source.ni;
    this.rd = source.rd;
    this.rd_c.copy(source.rd_c);
    this.td = source.td;
    this.td_c.copy(source.td_c);
    this.ed = source.ed;
    this.ed_c.copy(source.ed_c);
    this.rs = source.rs;
    this.rs_c.copy(source.rs_c);
    this.rs_a = source.rs_a;
    this.ts = source.ts;
    this.ts_c.copy(source.ts_c);
    this.ts_a = source.ts_a;
  }
}
