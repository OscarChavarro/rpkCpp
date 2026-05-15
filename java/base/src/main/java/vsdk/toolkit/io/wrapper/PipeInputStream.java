package vsdk.toolkit.io.wrapper;

import java.io.IOException;
import java.io.InputStream;
import java.util.Locale;

public class PipeInputStream extends InputStream {
    private Process processHandle;
    private InputStream pipeInput;

    private static ProcessBuilder processBuilderForCommand(String command) {
        String osName = System.getProperty("os.name", "").toLowerCase(Locale.ROOT);
        if (osName.contains("win")) {
            return new ProcessBuilder("cmd.exe", "/c", command);
        }
        return new ProcessBuilder("/bin/sh", "-c", command);
    }

    public PipeInputStream(String command) {
        processHandle = null;
        pipeInput = null;
        if (command != null && !command.isEmpty()) {
            try {
                ProcessBuilder processBuilder = processBuilderForCommand(command);
                processHandle = processBuilder.start();
                pipeInput = processHandle.getInputStream();
            }
            catch (IOException ignored) {
                processHandle = null;
                pipeInput = null;
            }
        }
    }

    public boolean isOpen() {
        return pipeInput != null;
    }

    @Override
    public int read() {
        if (pipeInput == null) {
            return -1;
        }
        try {
            return pipeInput.read();
        }
        catch (IOException ignored) {
            return -1;
        }
    }

    @Override
    public int read(byte[] buffer, int offset, int length) {
        if (pipeInput == null) {
            return -1;
        }
        if (buffer == null || offset < 0 || length < 0 || offset + length > buffer.length) {
            return -1;
        }
        if (length == 0) {
            return 0;
        }
        try {
            return pipeInput.read(buffer, offset, length);
        }
        catch (IOException ignored) {
            return -1;
        }
    }

    @Override
    public void close() {
        if (pipeInput == null) {
            return;
        }
        try {
            pipeInput.close();
        }
        catch (IOException ignored) {
        }
        pipeInput = null;

        if (processHandle != null) {
            processHandle.destroy();
            try {
                processHandle.waitFor();
            }
            catch (InterruptedException e) {
                Thread.currentThread().interrupt();
            }
            processHandle = null;
        }
    }

    public void dispose() {
        close();
    }
}
