import { FileOutputStream } from "../io/FileOutputStream";
import { PrintStream } from "../io/PrintStream";

export class System {
  private static readonly standardOutput = new FileOutputStream("/dev/stdout");
  private static readonly standardError = new FileOutputStream("/dev/stderr");

  public static readonly out = new PrintStream(System.standardOutput);
  public static readonly err = new PrintStream(System.standardError);

  public static exit(status: number): void {
    process.exit(status);
  }

  public static nanoTime(): number {
    if (process?.hrtime?.bigint) {
      return Number(process.hrtime.bigint());
    }
    return Date.now() * 1_000_000;
  }
}
