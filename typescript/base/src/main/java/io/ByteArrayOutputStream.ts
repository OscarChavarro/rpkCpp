import { OutputStream } from "./OutputStream.js";

export class ByteArrayOutputStream extends OutputStream {
    private bytes: Uint8Array;
    private length = 0;

    public constructor(initialSize = 32) {
        super();
        if (!Number.isInteger(initialSize) || initialSize < 0)
            throw new RangeError("Initial size must not be negative");
        this.bytes = new Uint8Array(initialSize);
    }

    public override write(value: number): void {
        this.ensureCapacity(this.length + 1);
        this.bytes[this.length++] = value & 0xff;
    }
    public override writeBytes(buffer: Uint8Array, offset = 0, length = buffer.length - offset): void {
        OutputStream.requireRange(buffer, offset, length);
        this.ensureCapacity(this.length + length);
        this.bytes.set(buffer.subarray(offset, offset + length), this.length);
        this.length += length;
    }
    public size(): number {
        return this.length;
    }
    public reset(): void {
        this.length = 0;
    }
    public toByteArray(): Uint8Array {
        return this.bytes.slice(0, this.length);
    }
    public override toString(encoding = "utf-8"): string {
        return new TextDecoder(encoding).decode(this.bytes.subarray(0, this.length));
    }
    public writeTo(output: OutputStream): void {
        output.writeBytes(this.bytes, 0, this.length);
    }

    private ensureCapacity(required: number): void {
        if (required <= this.bytes.length) return;
        const capacity = globalThis.Math.max(required, globalThis.Math.max(1, this.bytes.length * 2));
        const replacement = new Uint8Array(capacity);
        replacement.set(this.bytes.subarray(0, this.length));
        this.bytes = replacement;
    }
}
