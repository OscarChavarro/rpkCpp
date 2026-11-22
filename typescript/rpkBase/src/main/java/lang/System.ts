import { FileOutputStream } from "../io/FileOutputStream";
import { IllegalArgumentException } from "vitral/dist/java/lang/IllegalArgumentException";
import { PrintStream } from "vitral/dist/java/io/PrintStream";

export class System {
    private static readonly standardOutput = new FileOutputStream("/dev/stdout");
    private static readonly standardError = new FileOutputStream("/dev/stderr");

    public static readonly out = new PrintStream(System.standardOutput);
    public static readonly err = new PrintStream(System.standardError);

    public static exit(status: number): void {
        process.exit(status);
    }

    /**
    `System.getProperty(String)`. There is no JVM system-property table on
    this runtime, so every property is reported as unset, consistently with
    the `Boolean.getBoolean` port (which always answers `false`).
    */
    public static getProperty(name: string): string | null {
        if (name.length === 0) {
            throw new IllegalArgumentException("Property name is empty");
        }
        return null;
    }

    public static nanoTime(): number {
        if (process?.hrtime?.bigint) {
            return Number(process.hrtime.bigint());
        }
        return Date.now() * 1_000_000;
    }
}
