import { TypedOption } from "./TypedOption";

export class DefaultParser {
  private constructor() {
  }

  public static parse<T>(input: string | null, out: TypedOption.MutableValue<T> | null): boolean {
    if (input === null || out === null) {
      return false;
    }

    const currentValue = out.value as unknown;

    if (typeof currentValue === "number") {
      if (Number.isInteger(currentValue)) {
        const converted = new TypedOption.MutableValue<number>(currentValue);
        if (DefaultParser.parseInt(input, converted)) {
          out.value = converted.value as T;
          return true;
        }
      }

      const converted = new TypedOption.MutableValue<number>(currentValue);
      if (!DefaultParser.parseFloat(input, converted)) {
        return false;
      }
      out.value = converted.value as T;
      return true;
    }

    if (typeof currentValue === "bigint") {
      const converted = new TypedOption.MutableValue<bigint>(currentValue);
      if (!DefaultParser.parseLong(input, converted)) {
        return false;
      }
      out.value = converted.value as T;
      return true;
    }

    if (typeof currentValue === "boolean") {
      const boolValue = DefaultParser.parseBoolean(input);
      if (boolValue === null) {
        return false;
      }
      out.value = boolValue as T;
      return true;
    }

    out.value = input as T;
    return true;
  }

  private static parseInt(input: string, out: TypedOption.MutableValue<number> | null): boolean {
    if (input === null || out === null) {
      return false;
    }

    const parsedValue = DefaultParser.parseInteger(input);
    if (parsedValue === null) {
      return false;
    }
    if (parsedValue < -2147483648 || parsedValue > 2147483647) {
      return false;
    }

    out.value = parsedValue;
    return true;
  }

  private static parseLong(input: string, out: TypedOption.MutableValue<bigint> | null): boolean {
    if (input === null || out === null) {
      return false;
    }
    if (!/^[+-]?\d+$/.test(input)) {
      return false;
    }

    try {
      out.value = BigInt(input);
      return true;
    }
    catch (_e) {
      return false;
    }
  }

  private static parseFloat(input: string, out: TypedOption.MutableValue<number> | null): boolean {
    if (input === null || out === null) {
      return false;
    }

    const parsedValue = DefaultParser.parseNumber(input);
    if (parsedValue === null) {
      return false;
    }

    out.value = parsedValue;
    return true;
  }

  private static parseInteger(input: string): number | null {
    if (!/^[+-]?\d+$/.test(input)) {
      return null;
    }

    const parsedValue = Number.parseInt(input, 10);
    if (!Number.isFinite(parsedValue)) {
      return null;
    }
    return parsedValue;
  }

  private static parseNumber(input: string): number | null {
    if (input.trim().length === 0) {
      return null;
    }
    const parsedValue = Number(input);
    if (!Number.isFinite(parsedValue)) {
      return null;
    }
    return parsedValue;
  }

  private static parseBoolean(input: string): boolean | null {
    if (
      DefaultParser.equalsIgnoreCase(input, "true")
      || DefaultParser.equalsIgnoreCase(input, "yes")
      || input === "1"
    ) {
      return true;
    }

    if (
      DefaultParser.equalsIgnoreCase(input, "false")
      || DefaultParser.equalsIgnoreCase(input, "no")
      || input === "0"
    ) {
      return false;
    }

    return null;
  }

  private static toLowerAscii(c: string): string {
    if (c >= "A" && c <= "Z") {
      return String.fromCharCode(c.charCodeAt(0) - "A".charCodeAt(0) + "a".charCodeAt(0));
    }
    return c;
  }

  private static equalsIgnoreCase(a: string | null, b: string | null): boolean {
    if (a === null || b === null) {
      return false;
    }

    if (a.length !== b.length) {
      return false;
    }

    for (let i = 0; i < a.length; i++) {
      if (DefaultParser.toLowerAscii(a.charAt(i)) !== DefaultParser.toLowerAscii(b.charAt(i))) {
        return false;
      }
    }
    return true;
  }
}
