import { String } from "./String";

export class StringBuilder {
  private value: string;

  public constructor(text?: StringBuilder | String) {
    if (text instanceof StringBuilder) {
      this.value = text.toString();
      return;
    }
    if (text instanceof String) {
      this.value = text.toCString();
      return;
    }
    this.value = "";
  }

  public dispose(): void {
    this.value = "";
  }

  public assign(other: StringBuilder): StringBuilder {
    this.value = other.toString();
    return this;
  }

  public length(): number {
    return this.value.length;
  }

  public charAt(index: number): string {
    if (index < 0 || index >= this.value.length) {
      return "\0";
    }
    return this.value.charAt(index);
  }

  public clear(): void {
    this.value = "";
  }

  public append(text: String | string): StringBuilder;
  public append(text: string, textLength: number): StringBuilder;
  public append(text: String | string, textLength?: number): StringBuilder {
    const raw = text instanceof String ? text.toCString() : text;
    if (!raw || raw.length === 0) {
      return this;
    }
    if (textLength === undefined) {
      this.value += raw;
      return this;
    }
    if (textLength > 0) {
      this.value += raw.slice(0, textLength);
    }
    return this;
  }

  public appendChar(ch: string): StringBuilder {
    if (!ch || ch.length === 0) {
      return this;
    }
    this.value += ch.charAt(0);
    return this;
  }

  public toStringObject(): String {
    return new String(this.value);
  }

  public toString(): string {
    return this.value;
  }
}
