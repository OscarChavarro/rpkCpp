/**
A `ProcessingElement` in VitralSDK is a software element with algorithms and
data structures (i.e. a class) with the specific functionality for
manipulating and extract metadata from a data Entity (usually a geometric
low-level entity). It can also be the placeholder for wrappers that call
external libraries and tools.

As in Java, it does not extend `Entity`.
*/
export abstract class ProcessingElement {}
