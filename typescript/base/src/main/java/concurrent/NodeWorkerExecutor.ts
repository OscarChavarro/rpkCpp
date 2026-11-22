/*
Deep module specifiers rather than the `@vitral/base` barrel: this module is on
the import path of the raytracing worker thread, and the barrel makes every
worker compile the whole library (measured: 19.7 s versus 1.3 s to boot 72
workers on a 72-core host).
*/
import { Worker } from "node:worker_threads";

import {
    type WorkerExecutionOptions,
    type WorkerExecutor,
    type WorkerRequest,
    type WorkerResponse,
    type WorkerTransferValue,
} from "@vitral/base/java/concurrent/WorkerProtocol";

/**
Node counterpart of `BrowserWorkerExecutor`: a one-request-at-a-time executor
over a `node:worker_threads` worker, speaking the very same Vitral worker
request/response protocol.

Having both means a computation module written against the protocol runs
unchanged in a browser frontend and in a standalone Node backend; only the
executor that owns the thread differs. As in the browser version, only
structured-clone values cross the boundary, so no runtime state leaks into the
computation code.
*/
export class NodeWorkerExecutor<
    TInput extends WorkerTransferValue,
    TResult extends WorkerTransferValue,
> implements WorkerExecutor<TInput, TResult> {
    private nextRequestId = 1;
    private activeRequest:
        | {
              readonly id: string;
              readonly resolve: (value: TResult) => void;
              readonly reject: (reason: unknown) => void;
              readonly abortListener?: () => void;
              readonly abortSignal?: AbortSignal;
              readonly onNotice?: (notice: WorkerTransferValue) => void;
          }
        | undefined;
    private terminated = false;
    private readonly worker: Worker;

    /**
    @param moduleUrl the worker entry module, as a `file:` URL or absolute path.
    */
    public constructor(moduleUrl: string | URL) {
        this.worker = new Worker(moduleUrl);
        this.worker.on("message", this.onMessage);
        this.worker.on("error", this.onError);
        this.worker.on("messageerror", this.onMessageError);
    }

    public execute(input: TInput, options?: WorkerExecutionOptions): Promise<TResult> {
        if (this.terminated) return Promise.reject(new Error("This worker executor has been terminated"));
        if (this.activeRequest !== undefined) {
            return Promise.reject(new Error("This worker executor already has an active request"));
        }
        if (options?.signal?.aborted === true) {
            return Promise.reject(new Error("The operation was aborted"));
        }

        const id = String(this.nextRequestId++);
        return new Promise<TResult>((resolve, reject) => {
            const abortListener = (): void => {
                this.post({ id, kind: "cancel" });
                this.finish(id, reject, new Error("The operation was aborted"));
            };
            this.activeRequest = {
                id,
                resolve,
                reject,
                ...(options?.signal === undefined ? {} : { abortListener, abortSignal: options.signal }),
                ...(options?.onNotice === undefined ? {} : { onNotice: options.onNotice }),
            };
            options?.signal?.addEventListener("abort", abortListener, { once: true });
            this.post({ id, kind: "run", payload: input });
        });
    }

    public terminate(): void {
        if (this.terminated) return;
        this.terminated = true;
        const active = this.activeRequest;
        if (active !== undefined) {
            this.finish(active.id, active.reject, new Error("Worker terminated"));
        }
        this.worker.off("message", this.onMessage);
        this.worker.off("error", this.onError);
        this.worker.off("messageerror", this.onMessageError);
        void this.worker.terminate();
    }

    private readonly onMessage = (response: WorkerResponse<TResult>): void => {
        const active = this.activeRequest;
        if (active === undefined || response === null || response.id !== active.id) {
            return;
        }
        if (response.ok === "notice") {
            active.onNotice?.(response.value);
            return;
        }
        if (response.ok) {
            this.finish(active.id, active.resolve, response.value);
            return;
        }
        const error = new Error(response.error.message);
        error.name = response.error.name;
        if (response.error.stack !== undefined) {
            error.stack = response.error.stack;
        }
        this.finish(active.id, active.reject, error);
    };

    private readonly onError = (error: Error): void => {
        const active = this.activeRequest;
        if (active !== undefined) {
            this.finish(active.id, active.reject, error);
        }
    };

    private readonly onMessageError = (): void => {
        const active = this.activeRequest;
        if (active !== undefined) {
            this.finish(
                active.id,
                active.reject,
                new Error("Worker returned a value that cannot be structured-cloned"),
            );
        }
    };

    private post(request: WorkerRequest<TInput>): void {
        this.worker.postMessage(request);
    }

    private finish<T>(id: string, complete: (value: T) => void, value: T): void {
        const active = this.activeRequest;
        if (active === undefined || active.id !== id) {
            return;
        }
        this.activeRequest = undefined;
        if (active.abortListener !== undefined && active.abortSignal !== undefined) {
            active.abortSignal.removeEventListener("abort", active.abortListener);
        }
        complete(value);
    }
}
