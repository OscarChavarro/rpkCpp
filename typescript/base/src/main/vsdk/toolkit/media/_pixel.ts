import { VSDK } from "../common/VSDK.js";
export const byte = (value: number): number => VSDK.unsigned8BitInteger2signedByte(Math.trunc(value));
export const unsigned = (value: number): number => VSDK.signedByte2unsignedInteger(value);
