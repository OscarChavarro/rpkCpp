import { VSDKException } from "../../common/VSDKException.js";

/**
This class represents the exception in which an image can not be handled
by an image persistence operation.

Note that Java's counterpart also keeps the offending `java.io.File`. Local
files are a runtime-specific concern that browsers do not have, so on this
port the offending resource is reported by name; `@vitral/fs` supplies the
file-system-bound overloads.
*/
export class ImageNotRecognizedException extends VSDKException {
    private readonly image: string | null;

    /**
    Constructs an Image exception with a message and the name of the image
    resource that has been processed when the exception was generated.
    @param message The message to display in the stack trace
    @param image The image resource that caused the exception
    */
    public constructor(message: string, image: string | null) {
        super(message);
        this.image = image;
    }

    /**
    This method returns the image resource that has been processed when this
    Exception was generated
    @return The image resource that has been processed when the exception
    was generated
    */
    public getImage(): string | null {
        return this.image;
    }
}
