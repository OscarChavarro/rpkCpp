import { OutputStream } from "../../../../java/io/OutputStream";

const childProcess = require("node:child_process");

type CommandDescriptor = {
  program: string;
  args: string[];
};

type PipeOutputState = {
  command: string;
  chunks: Uint8Array[];
};

const pipeOutputStates = new WeakMap<PipeOutputStream, PipeOutputState>();

export class PipeOutputStream extends OutputStream {
  private processHandle: unknown | null;
  private pipeOutput: OutputStream | null;

  public constructor(command: string) {
    super();
    this.processHandle = null;
    this.pipeOutput = null;
    if (command !== null && command !== undefined && command.length > 0) {
      pipeOutputStates.set(this, {
        command,
        chunks: []
      });
      this.pipeOutput = this;
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
    return this.pipeOutput !== null;
  }

  public write(value: number): void;
  public write(buffer: Uint8Array, offset: number, length: number): void;
  public write(valueOrBuffer: number | Uint8Array, offset?: number, length?: number): void {
    if (this.pipeOutput === null) {
      return;
    }
    const state = pipeOutputStates.get(this);
    if (state === undefined) {
      return;
    }

    if (typeof valueOrBuffer === "number") {
      state.chunks.push(Buffer.from([valueOrBuffer & 0xFF]));
      return;
    }

    const safeOffset = offset ?? 0;
    const safeLength = length ?? 0;
    if (safeOffset < 0 || safeLength < 0 || safeOffset + safeLength > valueOrBuffer.length) {
      return;
    }
    if (safeLength === 0) {
      return;
    }

    state.chunks.push(Buffer.from(valueOrBuffer.slice(safeOffset, safeOffset + safeLength)));
  }

  public override flush(): void {
    if (this.pipeOutput === null) {
      return;
    }
  }

  public close(): void {
    if (this.pipeOutput === null) {
      return;
    }
    const state = pipeOutputStates.get(this);
    if (state === undefined) {
      this.pipeOutput = null;
      this.processHandle = null;
      return;
    }

    const activeCommand = state.command;
    this.pipeOutput = null;

    const payload = state.chunks.length > 0 ? Buffer.concat(state.chunks) : Buffer.alloc(0);
    pipeOutputStates.delete(this);

    const processDescriptor = PipeOutputStream.processBuilderForCommand(activeCommand);
    try {
      this.processHandle = childProcess.spawnSync(processDescriptor.program, processDescriptor.args, {
        input: payload,
        stdio: ["pipe", "ignore", "ignore"]
      });
    }
    catch (_error) {
      this.processHandle = null;
    }
    this.processHandle = null;
  }

  public override dispose(): void {
    this.close();
  }
}
