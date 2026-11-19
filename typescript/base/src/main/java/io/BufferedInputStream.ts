import { InputStream } from "./InputStream.js";

/** Buffered implementation of Java's InputStream contract. */
export class BufferedInputStream extends InputStream {
    private readonly buffer: Uint8Array;
    private position = 0;
    private limit = 0;
    private markPosition = -1;
    private markLimit = 0;
    private closed = false;

    public constructor(
        private readonly inputStream: InputStream,
        bufferSize = 8_192,
    ) {
        super();
        if (!Number.isInteger(bufferSize) || bufferSize <= 0) throw new RangeError("Buffer size must be positive");
        this.buffer = new Uint8Array(bufferSize);
    }

    public override read(): number {
        this.requireOpen();
        if (this.position >= this.limit && this.fill() < 0) return -1;
        return this.buffer[this.position++] ?? -1;
    }

    public override readBytes(buffer: Uint8Array, offset = 0, length = buffer.length - offset): number {
        InputStream.requireRange(buffer, offset, length);
        this.requireOpen();
        if (length === 0) return 0;
        let copied = 0;
        while (copied < length) {
            if (this.position >= this.limit && this.fill() < 0) return copied === 0 ? -1 : copied;
            const amount = globalThis.Math.min(this.limit - this.position, length - copied);
            buffer.set(this.buffer.subarray(this.position, this.position + amount), offset + copied);
            this.position += amount;
            copied += amount;
        }
        return copied;
    }

    public override skip(count: number): number {
        this.requireOpen();
        if (count <= 0) return 0;
        const buffered = globalThis.Math.min(count, this.limit - this.position);
        this.position += buffered;
        return buffered + this.inputStream.skip(count - buffered);
    }

    public override available(): number {
        this.requireOpen();
        return this.limit - this.position + this.inputStream.available();
    }
    public override mark(readLimit: number): void {
        this.requireOpen();
        this.markLimit = globalThis.Math.max(0, readLimit);
        this.markPosition = this.position;
    }
    public override reset(): void {
        this.requireOpen();
        if (this.markPosition < 0) throw new Error("Resetting to an invalid mark");
        this.position = this.markPosition;
    }
    public override markSupported(): boolean {
        return true;
    }

    public override close(): void {
        if (this.closed) return;
        this.closed = true;
        this.limit = 0;
        this.position = 0;
        this.markPosition = -1;
        this.inputStream.close();
    }

    private fill(): number {
        if (this.markPosition < 0 || this.position - this.markPosition >= this.markLimit) {
            this.markPosition = -1;
            this.position = 0;
            this.limit = this.inputStream.readBytes(this.buffer);
            return this.limit;
        }
        if (this.markPosition > 0) {
            this.buffer.copyWithin(0, this.markPosition, this.limit);
            this.position -= this.markPosition;
            this.limit -= this.markPosition;
            this.markPosition = 0;
        }
        if (this.limit === this.buffer.length) {
            this.markPosition = -1;
            this.position = 0;
            this.limit = this.inputStream.readBytes(this.buffer);
            return this.limit;
        }
        const read = this.inputStream.readBytes(this.buffer, this.limit, this.buffer.length - this.limit);
        if (read > 0) this.limit += read;
        return read;
    }

    private requireOpen(): void {
        if (this.closed) throw new Error("Stream closed");
    }
}
