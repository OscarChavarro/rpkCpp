import { RpkApplication } from "./RpkApplication";

export class Main {
  public static main(args: string[] | null): void {
    const argv = new Array<string>(args === null ? 1 : args.length + 1);
    argv[0] = "rpk";
    if (args !== null) {
      for (let i = 0; i < args.length; i++) {
        argv[i + 1] = args[i];
      }
    }

    const application = new RpkApplication();
    const result = application.entryPoint(argv.length, argv);
    process.exit(result);
  }
}

if ((require as any).main === module) {
  Main.main(process.argv.slice(2));
}
