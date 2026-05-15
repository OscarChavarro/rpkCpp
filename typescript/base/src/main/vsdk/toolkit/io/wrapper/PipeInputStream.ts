import { InputStream } from "../../../../java/io/InputStream";

const childProcess = require("node:child_process");

type CommandDescriptor = {
  program: string;
  args: string[];
};

type PipeInputState = {
  buffer: Uint8Array;
  position: number;
};

const pipeInputStates = new WeakMap<PipeInputStream, PipeInputState>();

export class PipeInputStream extends InputStream {
  private processHandle: unknown | null;
  private pipeInput: InputStream | null;

  public constructor(command: string) {
    super();
    this.processHandle = null;
    this.pipeInput = null;

    if (command !== null && command !== undefined && command.length > 0) {
      try {
        const processBuilder = PipeInputStream.processBuilderForCommand(command);
        const spawnResult = childProcess.spawnSync(processBuilder.program, processBuilder.args, {
          stdio: ["ignore", "pipe", "ignore"]
        });
        if (spawnResult !== null && spawnResult.error === undefined) {
          const stdoutData = spawnResult.stdout instanceof Uint8Array
            ? spawnResult.stdout
            : new Uint8Array(0);
          pipeInputStates.set(this, {
            buffer: new Uint8Array(stdoutData),
            position: 0
          });
          this.processHandle = spawnResult;
          this.pipeInput = this;
        }
      }
      catch (_error) {
        this.processHandle = null;
        this.pipeInput = null;
      }
    }
  }

  private static processBuilderForCommand(command: string): CommandDescriptor {
    if (process.platform === "win32") {
      return {
        program: "cmd.exe",
        args: ["/c", command]
      };
    }
    return {
      program: "/bin/sh",
      args: ["-c", command]
    };
  }

  public isOpen(): boolean {
    return this.pipeInput !== null;
  }

  private readRange(buffer: Uint8Array, offset: number, length: number): number {
    const state = pipeInputStates.get(this);
    if (state === undefined) {
      return -1;
    }
    const available = state.buffer.length - state.position;
    if (available <= 0) {
      return -1;
    }
    const count = available < length ? available : length;
    buffer.set(state.buffer.subarray(state.position, state.position + count), offset);
    state.position += count;
    return count;
  }

  public read(): number;
  public read(buffer: Uint8Array, offset: number, length: number): number;
  public read(buffer?: Uint8Array, offset?: number, length?: number): number {
    if (this.pipeInput === null) {
      return -1;
    }

    if (buffer === undefined) {
      const localBuffer = new Uint8Array(1);
      const readCount = this.readRange(localBuffer, 0, 1);
      if (readCount <= 0) {
        return -1;
      }
      return localBuffer[0];
    }

    const safeOffset = offset ?? 0;
    const safeLength = length ?? 0;
    if (safeOffset < 0 || safeLength < 0 || safeOffset + safeLength > buffer.length) {
      return -1;
    }
    if (safeLength === 0) {
      return 0;
    }

    return this.readRange(buffer, safeOffset, safeLength);
  }

  public close(): void {
    if (this.pipeInput === null) {
      return;
    }
    this.pipeInput = null;
    this.processHandle = null;
    pipeInputStates.delete(this);
  }

  public override dispose(): void {
    this.close();
  }
}
