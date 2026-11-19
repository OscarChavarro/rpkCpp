import { Throwable } from "../../../../java/lang/Throwable.js";
import { VSDKFatalException } from "../VSDKFatalException.js";
import {
    getWithFatalExceptions,
    getWithSystemExit,
    setWithFatalExceptions,
    setWithSystemExit,
} from "../VSDKRuntimeSettings.js";

/** Platform-neutral Java Logger port. Browser runtimes report through console.error. */
export class Logger {
    public static readonly WARNING = 1;
    public static readonly ERROR = 2;
    public static readonly FATAL_ERROR = 3;
    public static reportMessageWithException(
        origin: unknown,
        level: number,
        method: string,
        message: string,
        error: Error | null,
    ): void {
        const report =
            Logger.header(origin, method, `Vitral exception message:\n${message}`) +
            (error === null
                ? " - Java exception is null! No detailed information about error.\n"
                : ` - Java exception class:\n${error.name}\n - Java exception message:\n${error.message}\n${error.stack ?? ""}\n`) +
            Logger.footer(level);
        console.error(report);
        if (error === null) console.error("Given exception is null! not reporting details!");
        else console.error(error.message, error.stack ?? "");
        Logger.fatal(level, method, message, error);
    }
    public static reportMessage(origin: unknown, level: number, method: string, message: string): void {
        console.error(Logger.header(origin, method, `Exception message:\n${message}`) + Logger.footer(level));
        Logger.fatal(level, method, message, null);
    }
    /**
    Java also reads the `vsdk.withSystemExit` system property in its static
    initializer; this runtime has no system properties, so the default
    (`true`) of `VSDKRuntimeSettings` applies.
    */
    public static isWithSystemExit(): boolean {
        return getWithSystemExit();
    }
    public static setWithSystemExit(flag: boolean): void {
        setWithSystemExit(flag);
    }
    public static setWithFatalExceptions(flag: boolean): void {
        setWithFatalExceptions(flag);
    }
    private static header(origin: unknown, method: string, detail: string): string {
        const type =
            origin === null || origin === undefined
                ? " - An exception has been thrown from a static context\n"
                : ` - An exception has been thrown in the "${(origin as { constructor?: { name?: string } }).constructor?.name ?? typeof origin}" class\n`;
        return `===========================================================================\n= VSDK Exception report                                                   =\n${type} - Exception located at method ${method}\n - ${detail}\n`;
    }
    private static footer(level: number): string {
        return `===========================================================================\n${level === Logger.FATAL_ERROR ? "Program excecution suspended!\n" : ""}`;
    }
    private static fatal(level: number, method: string, message: string, cause: Error | null): void {
        if (level !== Logger.FATAL_ERROR) return;
        if (!getWithSystemExit() && !getWithFatalExceptions()) return;
        const prefix = method.length === 0 ? "VSDK fatal error" : `VSDK fatal error at ${method}`;
        const text = message.length === 0 ? prefix : `${prefix}: ${message}`;
        throw new VSDKFatalException(text, cause instanceof Throwable ? cause : undefined);
    }
}
