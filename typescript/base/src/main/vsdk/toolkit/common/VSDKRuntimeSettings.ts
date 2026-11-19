/** Shared fatal-report configuration used by VSDK now and Logger in phase 6. */
let withSystemExit = true;
let withFatalExceptions = true;

export function setWithSystemExit(value: boolean): void {
    withSystemExit = value;
}
export function setWithFatalExceptions(value: boolean): void {
    withFatalExceptions = value;
}
export function getWithSystemExit(): boolean {
    return withSystemExit;
}
export function getWithFatalExceptions(): boolean {
    return withFatalExceptions;
}
