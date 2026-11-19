/** Java-compatible synchronous byte output stream. */
export abstract class OutputStream {
    public abstract write(value: number): void;

    public writeBytes(buffer: Uint8Array, offset = 0, length = buffer.length - offset): void {
        OutputStream.requireRange(buffer, offset, length);
        for (let i = 0; i < length; i++) this.write(buffer[offset + i] ?? 0);
    }

    public flush(): void {
        /* Java OutputStream.flush is a no-op by default. */
    }
    public close(): void {
        this.flush();
    }
    public dispose(): void {
        this.close();
    }

    public static requireRange(buffer: Uint8Array, offset: number, length: number): void {
        if (
            !Number.isInteger(offset) ||
            !Number.isInteger(length) ||
            offset < 0 ||
            length < 0 ||
            offset > buffer.length - length
        ) {
            throw new RangeError("Invalid byte-buffer range");
        }
    }
}
