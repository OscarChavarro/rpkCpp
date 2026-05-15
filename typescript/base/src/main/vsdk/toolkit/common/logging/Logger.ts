const util = require("node:util");

export class Logger {
  private constructor() {
  }

  public static error(routine: string | null, text: string | null, ...args: unknown[]): void {
    process.stderr.write("Error: ");
    if (routine !== null) {
      process.stderr.write(`${routine}(): `);
    }
    process.stderr.write(`${Logger.formatMessage(text, ...args)}.\n`);
  }

  public static warning(routine: string | null, text: string | null, ...args: unknown[]): void {
    process.stderr.write("Warning: ");
    if (routine !== null) {
      process.stderr.write(`${routine}(): `);
    }
    process.stderr.write(`${Logger.formatMessage(text, ...args)}.\n`);
  }

  public static fatal(errcode: number, routine: string | null, text: string | null, ...args: unknown[]): void {
    process.stderr.write("logFatal error: ");
    if (routine !== null) {
      process.stderr.write(`${routine}(): `);
    }
    process.stderr.write(`${Logger.formatMessage(text, ...args)}.\n`);
    process.exit(errcode);
  }

  private static formatMessage(text: string | null, ...args: unknown[]): string {
    if (text === null) {
      return "";
    }
    try {
      return util.format(text, ...args);
    }
    catch (_e) {
      return text;
    }
  }
}
