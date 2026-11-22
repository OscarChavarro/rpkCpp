import { IllegalArgumentException } from "./IllegalArgumentException.js";
export class Boolean {
    public static readonly TRUE = true;
    public static readonly FALSE = false;
    public static parseBoolean(value: string): boolean {
        return value.toLowerCase() === "true";
    }
    public static valueOf(value: boolean | string): boolean {
        return typeof value === "boolean" ? value : Boolean.parseBoolean(value);
    }
    public static getBoolean(name: string): boolean {
        if (name.length === 0) throw new IllegalArgumentException("Property name is empty");
        return false;
    }
    public static compare(a: boolean, b: boolean): number {
        return a === b ? 0 : a ? 1 : -1;
    }
    public static hashCode(value: boolean): number {
        return value ? 1231 : 1237;
    }
    public static toString(value: boolean): string {
        return value ? "true" : "false";
    }
}
