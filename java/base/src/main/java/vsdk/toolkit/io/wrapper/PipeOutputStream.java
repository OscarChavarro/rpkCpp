package vsdk.toolkit.io.wrapper;

import java.io.IOException;
import java.io.OutputStream;
import java.util.Locale;

public class PipeOutputStream extends OutputStream {
    private Process processHandle;
    private OutputStream pipeOutput;

    private static ProcessBuilder processBuilderForCommand(String command) {
        String osName = System.getProperty("os.name", "").toLowerCase(Locale.ROOT);
        if (osName.contains("win")) {
            return new ProcessBuilder("cmd.exe", "/c", command);
        }
        return new ProcessBuilder("/bin/sh", "-c", command);
    }

    public PipeOutputStream(String command) {
        processHandle = null;
        pipeOutput = null;
        if (command != null && !command.isEmpty()) {
            try {
                ProcessBuilder processBuilder = processBuilderForCommand(command);
                processHandle = processBuilder.start();
                pipeOutput = processHandle.getOutputStream();
            }
            catch (IOException ignored) {
                processHandle = null;
                pipeOutput = null;
            }
        }
    }

    public boolean isOpen() {
        return pipeOutput != null;
    }

    @Override
    public void write(int value) {
        if (pipeOutput == null) {
            return;
        }
        try {
            pipeOutput.write(value & 0xFF);
        }
        catch (IOException ignored) {
        }
    }

    @Override
    public void write(byte[] buffer, int offset, int length) {
        if (pipeOutput == null || buffer == null || offset < 0 || length < 0 || offset + length > buffer.length) {
            return;
        }
        if (length == 0) {
            return;
        }
        try {
            pipeOutput.write(buffer, offset, length);
        }
        catch (IOException ignored) {
        }
    }

    @Override
    public void flush() {
        if (pipeOutput == null) {
            return;
        }
        try {
            pipeOutput.flush();
        }
        catch (IOException ignored) {
        }
    }

    @Override
    public void close() {
        if (pipeOutput == null) {
            return;
        }

        try {
            pipeOutput.flush();
        }
        catch (IOException ignored) {
        }

        try {
            pipeOutput.close();
        }
        catch (IOException ignored) {
        }
        pipeOutput = null;

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
