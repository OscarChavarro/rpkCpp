package vsdk.toolkit.io.wrapper;

import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.InputStream;
import java.io.OutputStream;
import vsdk.toolkit.common.Error;

public class FileUncompressWrapper {
    public static InputStream openInputStreamCompressWrapper(String fileName, int[] isPipe) {
        if (isPipe != null && isPipe.length > 0) {
            isPipe[0] = 0;
        }
        if (isInvalidFileName(fileName)) {
            return null;
        }

        int commandLength = fileName.length() + 20;
        StringBuilder command = new StringBuilder(commandLength);
        boolean pipeFlag = buildPipeCommand(fileName, StreamOpenMode.READ, command, commandLength);

        InputStream stream = null;
        if (pipeFlag) {
            stream = openPipeInputStream(command.toString());
        }
        else {
            File file = new File(fileName);
            if (file.exists() && file.canRead() && file.isFile()) {
                try {
                    stream = new FileInputStream(fileName);
                }
                catch (Exception ignored) {
                    stream = null;
                }
            }
        }

        if (stream == null) {
            Error.error(null, "Can't open file '%s' for %s", fileName, modeToLogAction(StreamOpenMode.READ));
            if (isPipe != null && isPipe.length > 0) {
                isPipe[0] = 0;
            }
            return null;
        }

        if (isPipe != null && isPipe.length > 0) {
            isPipe[0] = pipeFlag ? 1 : 0;
        }
        return stream;
    }

    public static OutputStream openOutputStreamCompressWrapper(String fileName, int[] isPipe) {
        if (isPipe != null && isPipe.length > 0) {
            isPipe[0] = 0;
        }
        if (isInvalidFileName(fileName)) {
            return null;
        }

        int commandLength = fileName.length() + 20;
        StringBuilder command = new StringBuilder(commandLength);
        boolean pipeFlag = buildPipeCommand(fileName, StreamOpenMode.WRITE, command, commandLength);

        OutputStream stream = null;
        if (pipeFlag) {
            stream = openPipeOutputStream(command.toString());
        }
        else {
            File file = new File(fileName);
            if (!file.isDirectory()) {
                try {
                    stream = new FileOutputStream(fileName);
                }
                catch (Exception ignored) {
                    stream = null;
                }
            }
        }

        if (stream == null) {
            Error.error(null, "Can't open file '%s' for %s", fileName, modeToLogAction(StreamOpenMode.WRITE));
            if (isPipe != null && isPipe.length > 0) {
                isPipe[0] = 0;
            }
            return null;
        }

        if (isPipe != null && isPipe.length > 0) {
            isPipe[0] = pipeFlag ? 1 : 0;
        }
        return stream;
    }

    public static void closeInputStream(InputStream stream) {
        if (stream == null) {
            return;
        }
        try {
            stream.close();
        }
        catch (Exception ignored) {
        }
    }

    public static void closeOutputStream(OutputStream stream) {
        if (stream == null) {
            return;
        }
        try {
            stream.close();
        }
        catch (Exception ignored) {
        }
    }

    private static String modeToLogAction(StreamOpenMode mode) {
        return mode == StreamOpenMode.READ ? "reading" : "writing";
    }

    private static boolean isInvalidFileName(String fileName) {
        if (fileName == null || fileName.isEmpty() || fileName.endsWith("/")) {
            return true;
        }
        return false;
    }

    private static boolean buildPipeCommand(String fileName, StreamOpenMode openMode, StringBuilder command, int commandLength) {
        if (fileName == null || command == null || commandLength <= 0) {
            return false;
        }

        command.setLength(0);

        if (fileName.charAt(0) == '|') {
            command.append(fileName.substring(1));
            return true;
        }

        int dot = fileName.lastIndexOf('.');
        String ext = dot >= 0 ? fileName.substring(dot) : null;
        if (".gz".equals(ext)) {
            if (openMode == StreamOpenMode.READ) {
                command.append("gunzip < ").append(fileName);
            }
            else {
                command.append("gzip > ").append(fileName);
            }
        }
        else if (".Z".equals(ext)) {
            if (openMode == StreamOpenMode.READ) {
                command.append("uncompress < ").append(fileName);
            }
            else {
                command.append("compress > ").append(fileName);
            }
        }
        else if (".bz".equals(ext)) {
            if (openMode == StreamOpenMode.READ) {
                command.append("bunzip < ").append(fileName);
            }
            else {
                command.append("bzip > ").append(fileName);
            }
        }
        else if (".bz2".equals(ext)) {
            if (openMode == StreamOpenMode.READ) {
                command.append("bunzip2 < ").append(fileName);
            }
            else {
                command.append("bzip2 > ").append(fileName);
            }
        }
        else {
            return false;
        }
        return true;
    }

    private static InputStream openPipeInputStream(String command) {
        PipeInputStream pipeStream = new PipeInputStream(command);
        if (!pipeStream.isOpen()) {
            return null;
        }
        return pipeStream;
    }

    private static OutputStream openPipeOutputStream(String command) {
        PipeOutputStream pipeStream = new PipeOutputStream(command);
        if (!pipeStream.isOpen()) {
            return null;
        }
        return pipeStream;
    }
}
