// Must be the first import: a replica writes nothing
import "./RefinementReplicaSandbox";
import { createWorkerMessageHandler } from "vitral/dist/java/concurrent/WorkerProtocol";
import {
  RefinementReplicas,
  type RefinementReplicaRequest,
} from "rpk-base/dist/vsdk/toolkit/galerkin/processing/RefinementReplicas";
import { Main } from "./Main";

/*
Worker module of a replica of the parallel Jacobi iterations (see
RefinementReplicas), started by the NodeWorkerExecutor that Main gives to
RefinementReplicas: the worker half of VITRAL's worker protocol, whose one
request carries the command line of the program and the shared memory of the
iterations. The replica runs the program with that command line (its
iterations meet the main thread's ones in HierarchicalRefinementStrategy) and
ends with it; it never answers the request.

As VITRAL's NodeWorkerRuntime, but over createWorkerMessageHandler: a Node
worker receives the bare message, which the handler expects as the data of a
browser MessageEvent.
*/
const replicaPort = require("node:worker_threads").parentPort;

const handler = createWorkerMessageHandler<RefinementReplicaRequest, null>(
  (request: RefinementReplicaRequest): null => {
    try {
      RefinementReplicas.joinAsReplica(request);
      Main.main(request.argv);
    }
    catch (error: unknown) {
      RefinementReplicas.failReplica();
      throw error;
    }
    return null;
  },
  (response: unknown): void => {
    replicaPort.postMessage(response);
  },
);

replicaPort.on("message", (request: unknown): void => {
  handler({ data: request } as unknown as Parameters<typeof handler>[0]);
});
