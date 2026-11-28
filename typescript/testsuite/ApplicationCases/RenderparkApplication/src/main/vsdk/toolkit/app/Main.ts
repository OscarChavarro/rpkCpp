import { NodeWorkerExecutor } from "rpk-base/dist/java/concurrent/NodeWorkerExecutor";
import { RefinementReplicas } from "rpk-base/dist/vsdk/toolkit/galerkin/processing/RefinementReplicas";
import { ReplicaMemory } from "./ReplicaMemory";
import { RpkApplication } from "./RpkApplication";

export class Main {
  public static main(args: string[] | null): void {
    const argv = new Array<string>(args === null ? 1 : args.length + 1);
    argv[0] = "rpk";
    if (args !== null) {
      for (let i = 0; i < args.length; i++) {
        argv[i + 1] = args[i]!;
      }
    }

    const application = new RpkApplication();
    const result = application.entryPoint(argv.length, argv);
    process.exit(result);
  }
}

if ((require as any).main === module) {
  // The workers of the parallel Jacobi iterations are replicas of this
  // program, with its command line (see RefinementReplicaMain)
  const replicaModule = require("node:path").join(__dirname, "RefinementReplicaMain.js");
  RefinementReplicas.setReplicaFactory(
    () => new NodeWorkerExecutor(replicaModule, ReplicaMemory.workerOptions()),
    process.argv.slice(2),
  );
  RefinementReplicas.setMemoryReclaimer(ReplicaMemory.fullCollection());
  Main.main(process.argv.slice(2));
}
