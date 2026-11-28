import { Throwable } from "../../../../java/lang/Throwable.js";
import { Exception } from "../../../../java/lang/Exception.js";
import { VSDKFatalException } from "../VSDKFatalException.js";
import {
    getWithFatalExceptions,
    getWithSystemExit,
    setWithFatalExceptions,
    setWithSystemExit,
} from "../VSDKRuntimeSettings.js";

/** Platform-neutral Java Logger port. Host-specific names and stack frames come from JavaScript. */
export class Logger {
    public static readonly WARNING = 1;
    public static readonly ERROR = 2;
    public static readonly FATAL_ERROR = 3;
    private static readonly EXCEPTION_SEPARATOR =
        "---------------------------------------------------------------------------";
    public static reportMessageWithException(
        origin: unknown,
        level: number,
        method: string | null,
        message: string | null,
        error: Error | null,
    ): void {
        const frames = error === null ? [] : Logger.stackFrames(error);
        const exceptionMessage = error instanceof Throwable ? (error.getMessage() ?? "null") : error?.message;
        const report =
            Logger.header(origin, method, `Vitral exception message:\n${message}`) +
            (error === null
                ? " - Java exception is null! No detailed information about error.\n"
                : ` - Java exception class:\n${error.name}\n - Java exception message:\n${exceptionMessage}\n` +
                  frames.map((frame) => frame + "\n").join("")) +
            Logger.footer(level);
        console.error(report);
        console.error(Logger.EXCEPTION_SEPARATOR);
        if (error === null) {
            console.error("Given exception is null! not reporting details!");
        } else {
            console.error(exceptionMessage);
            for (const frame of frames) console.error(frame);
        }
        console.error(Logger.EXCEPTION_SEPARATOR);
        if (level === Logger.FATAL_ERROR) {
            Logger.reportFatalStack();
            Logger.processFatalError(method, message, error);
        }
    }
    public static reportMessage(origin: unknown, level: number, method: string | null, message: string | null): void {
        console.error(Logger.header(origin, method, `Exception message:\n${message}`) + Logger.footer(level));
        if (level === Logger.FATAL_ERROR) {
            Logger.reportFatalStack();
            Logger.processFatalError(method, message, null);
        }
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
    private static header(origin: unknown, method: string | null, detail: string): string {
        const type =
            origin === null || origin === undefined
                ? " - An exception has been thrown from a static context\n"
                : ` - An exception has been thrown in the "${(origin as { constructor?: { name?: string } }).constructor?.name ?? typeof origin}" class\n`;
        return `===========================================================================\n= VSDK Exception report                                                   =\n${type} - Exception located at method ${method}\n - ${detail}\n`;
    }
    private static footer(level: number): string {
        return `===========================================================================\n${level === Logger.FATAL_ERROR ? "Program excecution suspended!\n" : ""}`;
    }
    private static stackFrames(error: Error): string[] {
        const lines = error.stack?.split("\n") ?? [];
        const header = `${error.name}: ${error.message}`;
        return lines
            .slice(error.stack?.startsWith(header) ? error.message.split("\n").length : 0)
            .map((line) => line.trim().replace(/^at /, ""))
            .filter((line) => line.length > 0 && line !== error.name);
    }

    private static reportFatalStack(): void {
        const error = new Error("Logger.reportMessage(FATAL_ERROR)");
        console.error(error.message);
        for (const frame of Logger.stackFrames(error).slice(1)) console.error(frame);
    }

    private static processFatalError(method: string | null, message: string | null, cause: Error | null): void {
        // As in Java, a fatal error never terminates the process: it is
        // thrown as a VSDKFatalException for the calling application.
        if (!getWithSystemExit() && !getWithFatalExceptions()) return;
        let text = method === null || method.length === 0 ? "VSDK fatal error" : `VSDK fatal error at ${method}`;
        if (message !== null && message.length > 0) text += ": " + message;
        if (cause === null || cause instanceof Throwable) {
            throw new VSDKFatalException(text, cause ?? undefined);
        }
        const adapted = new Exception(cause.message);
        adapted.name = cause.name;
        if (cause.stack !== undefined) adapted.stack = cause.stack;
        const error = new VSDKFatalException(text, adapted);
        error.cause = cause;
        throw error;
    }
}
