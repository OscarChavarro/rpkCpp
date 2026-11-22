/** A value that can be transferred through the structured-clone algorithm. */
export type WorkerTransferValue =
    | null
    | boolean
    | number
    | string
    | ArrayBuffer
    | ArrayBufferView
    | WorkerTransferValue[]
    | { readonly [key: string]: WorkerTransferValue };

export interface WorkerRequest<TInput extends WorkerTransferValue> {
    readonly id: string;
    readonly kind: "run" | "cancel";
    readonly payload?: TInput;
}

export interface WorkerSuccess<TResult extends WorkerTransferValue> {
    readonly id: string;
    readonly ok: true;
    readonly value: TResult;
}

export interface WorkerFailure {
    readonly id: string;
    readonly ok: false;
    readonly error: {
        readonly name: string;
        readonly message: string;
        readonly stack?: string;
    };
}

/**
 * An intermediate message a running computation sends back before it finishes,
 * used for progress reporting. Notices never settle the request.
 */
export interface WorkerNotice {
    readonly id: string;
    readonly ok: "notice";
    readonly value: WorkerTransferValue;
}

export type WorkerResponse<TResult extends WorkerTransferValue> = WorkerSuccess<TResult> | WorkerFailure | WorkerNotice;

export interface WorkerExecutionOptions {
    readonly transfer?: readonly Transferable[];
    readonly signal?: AbortSignal;
    /** Called for every notice the computation emits while the request runs. */
    readonly onNotice?: (notice: WorkerTransferValue) => void;
}

export interface WorkerExecutor<TInput extends WorkerTransferValue, TResult extends WorkerTransferValue> {
    execute(input: TInput, options?: WorkerExecutionOptions): Promise<TResult>;
    terminate(): void;
}

/**
 * Installs the worker-side half of the Vitral protocol.  Computations receive
 * an AbortSignal, so cancellation is cooperative and never leaks browser UI
 * state into a worker.  The returned handler is suitable for
 * `self.addEventListener("message", handler)` in a Web Worker entry module.
 */
export function createWorkerMessageHandler<TInput extends WorkerTransferValue, TResult extends WorkerTransferValue>(
    run: (
        input: TInput,
        signal: AbortSignal,
        notify: (value: WorkerTransferValue) => void,
    ) => Promise<TResult> | TResult,
    post: (response: WorkerResponse<TResult>) => void,
): (event: MessageEvent<WorkerRequest<TInput>>) => void {
    const controllers = new Map<string, AbortController>();
    return (event): void => {
        const request = event.data;
        if (request.kind === "cancel") {
            controllers.get(request.id)?.abort();
            return;
        }
        if (request.payload === undefined) {
            post({ id: request.id, ok: false, error: { name: "TypeError", message: "Run request has no payload" } });
            return;
        }
        const controller = new AbortController();
        controllers.set(request.id, controller);
        const notify = (value: WorkerTransferValue): void => {
            if (!controller.signal.aborted) post({ id: request.id, ok: "notice", value });
        };
        void Promise.resolve(run(request.payload, controller.signal, notify))
            .then(
                (value) => {
                    if (!controller.signal.aborted) post({ id: request.id, ok: true, value });
                },
                (error: unknown) => {
                    if (!controller.signal.aborted) post({ id: request.id, ok: false, error: workerError(error) });
                },
            )
            .finally(() => controllers.delete(request.id));
    };
}

export function workerError(error: unknown): WorkerFailure["error"] {
    if (error instanceof Error) {
        return {
            name: error.name,
            message: error.message,
            ...(error.stack === undefined ? {} : { stack: error.stack }),
        };
    }
    return { name: "Error", message: globalThis.String(error) };
}
