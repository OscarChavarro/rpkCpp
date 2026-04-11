import { OptionBase } from "./OptionBase";

export class OptionGroupT<TOptionBase extends OptionBase> {
  public name: string | null;
  public options: TOptionBase[] | null;
  public count: number;

  public constructor();
  public constructor(groupName: string, groupOptions: TOptionBase[], groupCount: number);
  public constructor(groupName?: string, groupOptions: TOptionBase[] | null = null, groupCount = 0) {
    this.name = groupName ?? null;
    this.options = groupOptions;
    this.count = groupCount;
  }
}
