/** Java-like throwable with message, cause, and a stable Java-facing API. */
export class Throwable extends Error {
    private throwableCause: Throwable | undefined;

    public constructor(message?: string, cause?: Throwable) {
        super(message);
        this.name = "Throwable";
        this.throwableCause = cause;
    }

    public getMessage(): string | undefined {
        return this.message.length === 0 ? undefined : this.message;
    }
    public getCause(): Throwable | undefined {
        return this.throwableCause;
    }

    public initCause(cause: Throwable): this {
        if (this.throwableCause !== undefined) throw new Error("Cause already initialized");
        if (cause === this) throw new Error("Self-causation not permitted");
        this.throwableCause = cause;
        return this;
    }

    public override toString(): string {
        return this.message.length === 0 ? this.name : `${this.name}: ${this.message}`;
    }
}
