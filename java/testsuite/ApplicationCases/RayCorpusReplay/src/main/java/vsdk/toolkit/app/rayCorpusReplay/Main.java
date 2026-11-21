package vsdk.toolkit.app.rayCorpusReplay;

import java.util.regex.Matcher;
import java.util.regex.Pattern;
import vsdk.toolkit.app.RpkApplication;
import vsdk.toolkit.common.Random;

/**
Replays a step 1 ray corpus against the scene built from the same command
line as the run that captured it (doc/vitralAlignmentPlan.md, step 4 item 4).
The command line is the rpk one; the replay is configured by environment, so
that scripts/replayRays.sh can run each corpus scene's own script with this
program as the application task:

  RPK_RAY_REPLAY_FILE           corpus file (required)
  RPK_RAY_REPLAY_REPORT         file receiving each disagreement with its ray
  RPK_RAY_REPLAY_THREADS        workers of the threaded pass (default 1, no
                                threaded pass; see RayCorpusReplay)
  RPK_RAY_REPLAY_LINEAR_STRIDE  replay 1 query in N through the linear
                                reference (default 1, all)
  RPK_RAY_REPLAY_LIMIT          stop after N replayed queries (default all)
*/
public final class Main {
    private static final Pattern LEADING_INTEGER = Pattern.compile("^\\s*([+-]?\\d+)");

    private Main() {
    }

    private static long environmentNumber(String name, long defaultValue) {
        final String value = System.getenv(name);
        if (value == null || value.isEmpty()) {
            return defaultValue;
        }
        Matcher matcher = LEADING_INTEGER.matcher(value);
        if (!matcher.find()) {
            return 0;
        }
        try {
            return Long.parseLong(matcher.group(1));
        }
        catch (NumberFormatException e) {
            return 0;
        }
    }

    public static void main(String[] args) {
        final String corpus = System.getenv("RPK_RAY_REPLAY_FILE");
        if (corpus == null || corpus.isEmpty()) {
            System.err.println("rayCorpusReplay: set RPK_RAY_REPLAY_FILE to a ray corpus (.vqc)");
            System.exit(2);
        }
        String[] argv = new String[args == null ? 1 : args.length + 1];
        argv[0] = "rayCorpusReplay";
        if (args != null) {
            System.arraycopy(args, 0, argv, 1, args.length);
        }
        Runtime.getRuntime().addShutdownHook(new Thread(Random::shutdown));

        RayCorpusReplay replay = new RayCorpusReplay(
            corpus,
            System.getenv("RPK_RAY_REPLAY_REPORT"),
            (int)environmentNumber("RPK_RAY_REPLAY_THREADS", 1),
            environmentNumber("RPK_RAY_REPLAY_LINEAR_STRIDE", 1),
            environmentNumber("RPK_RAY_REPLAY_LIMIT", 0));
        RpkApplication application = new RpkApplication();
        final int status = application.entryPoint(argv.length, argv, replay);
        replay.destroy();
        System.out.flush();
        System.exit(status);
    }
}
