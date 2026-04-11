export class OptionTextUtils {
  private constructor() {
  }

  private static toLowerAscii(c: string): string {
    if (c >= "A" && c <= "Z") {
      return String.fromCharCode(c.charCodeAt(0) - "A".charCodeAt(0) + "a".charCodeAt(0));
    }
    return c;
  }

  public static equalsIgnoreCase(a: string | null, b: string | null): boolean {
    if (a === null || b === null) {
      return false;
    }
    if (a.length !== b.length) {
      return false;
    }
    for (let i = 0; i < a.length; i++) {
      if (OptionTextUtils.toLowerAscii(a.charAt(i)) !== OptionTextUtils.toLowerAscii(b.charAt(i))) {
        return false;
      }
    }
    return true;
  }

  public static equalsIgnoreCasePrefix(input: string | null, name: string | null, prefixLength: number): boolean {
    if (input === null || name === null || prefixLength <= 0) {
      return false;
    }
    if (input.length < prefixLength || name.length < prefixLength) {
      return false;
    }
    for (let i = 0; i < prefixLength; i++) {
      if (OptionTextUtils.toLowerAscii(input.charAt(i)) !== OptionTextUtils.toLowerAscii(name.charAt(i))) {
        return false;
      }
    }
    return true;
  }

  public static parseBoolInt(text: string | null, out: OptionTextUtils.TypedIntValue | null): boolean {
    if (text === null || out === null) {
      return false;
    }
    if (
      OptionTextUtils.equalsIgnoreCase(text, "true")
      || OptionTextUtils.equalsIgnoreCase(text, "yes")
      || text === "1"
    ) {
      out.value = 1;
      return true;
    }
    if (
      OptionTextUtils.equalsIgnoreCase(text, "false")
      || OptionTextUtils.equalsIgnoreCase(text, "no")
      || text === "0"
    ) {
      out.value = 0;
      return true;
    }
    return false;
  }
}

export namespace OptionTextUtils {
  export class TypedIntValue {
    public value: number;

    public constructor(value: number) {
      this.value = value;
    }
  }
}
