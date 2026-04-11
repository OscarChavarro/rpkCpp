import { Error as VsdkError } from "../Error";
import { Numeric } from "../linealAlgebra/Numeric";
import { BalancedKDTreeNode } from "./BalancedKDTreeNode";
import { KDQuery } from "./KDQuery";
import { KDTreeNode } from "./KDTreeNode";

export interface NodeCallback {
  call(userData: unknown, nodeData: unknown): void;
}

export class KDTree {
  protected numberOfNodes: number;
  protected dataSize: number;
  protected numUnbalanced: number;
  protected root: KDTreeNode | null;
  protected numBalanced: number;
  protected firstLeaf: number;
  protected balancedRootNode: BalancedKDTreeNode[] | null;
  protected copyData: boolean;
  protected static distances: number[] | null = null;

  public static readonly KD_MAX_RADIUS = 1e10;

  public constructor(dataSize: number);
  public constructor(inDataSize: number, copyData: boolean);
  public constructor(inDataSize: number, copyData = true) {
    this.dataSize = inDataSize;
    this.numberOfNodes = 0;
    this.numUnbalanced = 0;
    this.root = null;
    this.numBalanced = 0;
    this.balancedRootNode = null;
    this.copyData = copyData;

    if (KDTree.distances === null) {
      KDTree.distances = new Array<number>(1000).fill(0.0);
    }
    this.firstLeaf = 0;
  }

  public dispose(): void {
    this.deleteNodes(this.root, this.copyData);
    this.root = null;
    this.deleteBNodes(this.copyData);
  }

  public addPoint(data: unknown): void;
  public addPoint(data: unknown, flags: number): void;
  public addPoint(data: unknown, flags = 0): void {
    const newNode = new KDTreeNode();
    newNode.mData = this.assignData(data);
    newNode.mFlags = flags;
    newNode.setDiscriminator(0);
    newNode.loson = null;
    newNode.hison = null;

    const newPoint = KDTree.asPoint(data);

    this.numberOfNodes++;
    this.numUnbalanced++;

    let parent: KDTreeNode | null = null;
    let current = this.root;

    while (current !== null) {
      parent = current;
      const discriminator = parent.discriminator();
      const parentPoint = KDTree.asPoint(parent.mData);
      if (newPoint[discriminator] <= parentPoint[discriminator]) {
        current = parent.loson;
      }
      else {
        current = parent.hison;
      }
    }

    if ((parent !== null) && (parent.loson === null) && (parent.hison === null)) {
      const parentPoint = KDTree.asPoint(parent.mData);
      const dx = globalThis.Math.abs(newPoint[0] - parentPoint[0]);
      const dy = globalThis.Math.abs(newPoint[1] - parentPoint[1]);
      const dz = globalThis.Math.abs(newPoint[2] - parentPoint[2]);

      let discriminator: number;
      if (dx > dy) {
        discriminator = (dx > dz) ? 0 : 2;
      }
      else {
        discriminator = (dy > dz) ? 1 : 2;
      }

      parent.setDiscriminator(discriminator);
      if (newPoint[discriminator] <= parentPoint[discriminator]) {
        parent.loson = newNode;
      }
      else {
        parent.hison = newNode;
      }
    }
    else if (parent === null) {
      this.root = newNode;
    }
    else {
      const discriminator = parent.discriminator();
      const parentPoint = KDTree.asPoint(parent.mData);
      if (newPoint[discriminator] <= parentPoint[discriminator]) {
        parent.loson = newNode;
      }
      else {
        parent.hison = newNode;
      }
    }
  }

  public iterateNodes(callback: NodeCallback, data: unknown): void {
    if (this.numUnbalanced > 0) {
      VsdkError.error("KDTree::iterateNodes", "Cannot iterate unbalanced trees");
      return;
    }

    for (let i = 0; i < this.numBalanced; i++) {
      callback.call(data, (this.balancedRootNode as BalancedKDTreeNode[])[i].mData);
    }
  }

  public query(point: number[], n: number, results: unknown[]): number;
  public query(
    point: number[],
    n: number,
    results: unknown[],
    inDistances: number[] | null,
    radius: number,
    excludeFlags: number
  ): number;
  public query(
    point: number[],
    n: number,
    results: unknown[],
    inDistances: number[] | null = null,
    radius = KDTree.KD_MAX_RADIUS,
    excludeFlags = 0
  ): number {
    const queryData = new KDQuery();

    let usedDistances: number[];
    if (inDistances === null) {
      if (n > (KDTree.distances as number[]).length) {
        VsdkError.error("KDTree::query", "Too many nodes requested");
        return 0;
      }
      usedDistances = KDTree.distances as number[];
    }
    else {
      usedDistances = inDistances;
    }

    queryData.point = point;
    queryData.wantedN = n;
    queryData.foundN = 0;
    queryData.results = results;
    queryData.distances = usedDistances;
    queryData.maximumDistance = radius;
    queryData.sqrRadius = radius;
    queryData.excludeFlags = excludeFlags;
    queryData.notFilled = true;

    if (this.balancedRootNode !== null) {
      this.balancedQueryRec(0, queryData);
    }

    if (this.root !== null) {
      this.queryRec(this.root, queryData);
    }

    return queryData.foundN;
  }

  public balance(): void {
    if (this.numUnbalanced === 0) {
      return;
    }

    process.stderr.write(`Balancing kd-tree: ${this.numberOfNodes} nodes...\n`);

    const broot = new Array<BalancedKDTreeNode>(this.numberOfNodes + 1);
    for (let i = 0; i < broot.length; i++) {
      broot[i] = new BalancedKDTreeNode();
    }
    broot[this.numberOfNodes].mData = [Number.POSITIVE_INFINITY, Number.POSITIVE_INFINITY, Number.POSITIVE_INFINITY];
    broot[this.numberOfNodes].mFlags = 128;

    let index = 0;
    for (let i = 0; i < this.numBalanced; i++) {
      broot[index++] = (this.balancedRootNode as BalancedKDTreeNode[])[i];
    }

    const pIndex = [index];
    KDTree.copyUnbalancedRec(this.root, broot, pIndex);

    this.deleteNodes(this.root, false);
    this.root = null;
    this.numUnbalanced = 0;
    this.deleteBNodes(false);

    this.numBalanced = this.numberOfNodes;
    const dest = new Array<BalancedKDTreeNode>(this.numberOfNodes + 1);
    for (let i = 0; i < dest.length; i++) {
      dest[i] = new BalancedKDTreeNode();
    }
    dest[this.numberOfNodes].mData = [Number.POSITIVE_INFINITY, Number.POSITIVE_INFINITY, Number.POSITIVE_INFINITY];
    dest[this.numberOfNodes].mFlags = 64;

    this.balanceRec(broot, dest, 0, 0, this.numberOfNodes - 1);

    this.balancedRootNode = dest;
    this.firstLeaf = globalThis.Math.trunc((this.numBalanced + 1) / 2);

    process.stderr.write("done\n");
  }

  public balanceRec(
    broot: BalancedKDTreeNode[],
    dest: BalancedKDTreeNode[],
    destIndex: number,
    low: number,
    high: number
  ): void {
    if (low === high) {
      dest[destIndex] = broot[low];
      dest[destIndex].setDiscriminator(0);
      return;
    }

    const discr = KDTree.bestDiscriminator(broot, low, high);
    const median = KDTree.quickSelect(broot, low, high, discr);

    dest[destIndex] = broot[median];
    dest[destIndex].setDiscriminator(discr);

    if (low < median) {
      this.balanceRec(broot, dest, (destIndex << 1) + 1, low, median - 1);
    }
    if (high > median) {
      this.balanceRec(broot, dest, (destIndex << 1) + 2, median + 1, high);
    }
  }

  private assignData(data: unknown): unknown {
    if (!this.copyData) {
      return data;
    }

    if (Array.isArray(data)) {
      return data.slice();
    }

    if (data instanceof Uint8Array) {
      return data.slice();
    }

    return data;
  }

  private static asPoint(data: unknown): number[] {
    if (!Array.isArray(data)) {
      throw new globalThis.Error("KDTree data must be a float[] with xyz coordinates in first entries");
    }

    if (data.length < 3) {
      throw new globalThis.Error("KDTree point data must have at least 3 components");
    }

    return data as number[];
  }

  private deleteNodes(node: KDTreeNode | null, deleteData: boolean): void {
    if (node === null) {
      return;
    }

    this.deleteNodes(node.loson, deleteData);
    this.deleteNodes(node.hison, deleteData);

    node.loson = null;
    node.hison = null;
    if (deleteData) {
      node.mData = null;
    }
  }

  private deleteBNodes(deleteData: boolean): void {
    if (this.balancedRootNode === null) {
      return;
    }

    if (deleteData) {
      for (let node = 0; node < this.numBalanced; node++) {
        this.balancedRootNode[node].mData = null;
      }
    }

    this.balancedRootNode = null;
  }

  private static sqrDistance3D(a: number[], b: number[]): number {
    let tmp = a[0] - b[0];
    let result = tmp * tmp;

    tmp = a[1] - b[1];
    result += tmp * tmp;

    tmp = a[2] - b[2];
    result += tmp * tmp;

    return result;
  }

  private static fixUp(queryData: KDQuery): void {
    let son = queryData.foundN;
    let parent = (son - 1) >> 1;

    while ((son > 0) && queryData.distances[parent] < queryData.distances[son]) {
      const tmpDist = queryData.distances[parent];
      const tmpData = queryData.results[parent];

      queryData.distances[parent] = queryData.distances[son];
      queryData.results[parent] = queryData.results[son];

      queryData.distances[son] = tmpDist;
      queryData.results[son] = tmpData;

      son = parent;
      parent = (son - 1) >> 1;
    }
  }

  private static mhInsert(queryData: KDQuery, data: unknown, dist: number): void {
    queryData.distances[queryData.foundN] = dist;
    queryData.results[queryData.foundN] = data;

    KDTree.fixUp(queryData);

    if (++queryData.foundN === queryData.wantedN) {
      queryData.maximumDistance = queryData.distances[0];
      queryData.notFilled = false;
    }
  }

  private static fixDown(queryData: KDQuery): void {
    const max = queryData.foundN;
    let parent = 0;
    let son = 1;

    while (son < max) {
      if (queryData.distances[son] <= queryData.distances[parent]) {
        if ((++son >= max) || queryData.distances[son] <= queryData.distances[parent]) {
          return;
        }
      }
      else if ((son + 1 < max) && queryData.distances[son + 1] > queryData.distances[son]) {
        son++;
      }

      const tmpDist = queryData.distances[parent];
      const tmpData = queryData.results[parent];

      queryData.distances[parent] = queryData.distances[son];
      queryData.results[parent] = queryData.results[son];

      queryData.distances[son] = tmpDist;
      queryData.results[son] = tmpData;

      parent = son;
      son = (parent << 1) + 1;
    }
  }

  private static mhReplaceMax(queryData: KDQuery, data: unknown, dist: number): void {
    queryData.distances[0] = dist;
    queryData.results[0] = data;

    KDTree.fixDown(queryData);
    queryData.maximumDistance = queryData.distances[0];
  }

  private queryRec(node: KDTreeNode, queryData: KDQuery): void {
    const discriminator = node.discriminator();
    let dist = KDTree.sqrDistance3D(KDTree.asPoint(node.mData), queryData.point);

    if (dist < queryData.maximumDistance) {
      if (queryData.notFilled) {
        KDTree.mhInsert(queryData, node.mData, dist);
      }
      else {
        KDTree.mhReplaceMax(queryData, node.mData, dist);
      }
    }

    dist = KDTree.asPoint(node.mData)[discriminator] - queryData.point[discriminator];

    let nearNode: KDTreeNode | null;
    let farNode: KDTreeNode | null;
    if (dist >= 0.0) {
      nearNode = node.loson;
      farNode = node.hison;
    }
    else {
      nearNode = node.hison;
      farNode = node.loson;
    }

    if (nearNode !== null) {
      this.queryRec(nearNode, queryData);
    }

    dist *= dist;
    if (
      farNode !== null
      && (
        ((queryData.foundN < queryData.wantedN) && (dist < queryData.sqrRadius))
        || (dist < queryData.maximumDistance)
      )
    ) {
      this.queryRec(farNode, queryData);
    }
  }

  private balancedQueryRec(index: number, queryData: KDQuery): void {
    const node = (this.balancedRootNode as BalancedKDTreeNode[])[index];
    const discr = node.discriminator();
    let dist: number;
    let nearIndex: number;
    let farIndex: number;

    if (index < this.firstLeaf) {
      dist = KDTree.asPoint(node.mData)[discr] - queryData.point[discr];

      if (dist >= 0.0) {
        nearIndex = (index << 1) + 1;
        farIndex = nearIndex + 1;
      }
      else {
        farIndex = (index << 1) + 1;
        nearIndex = farIndex + 1;
      }

      if (nearIndex < this.numBalanced) {
        this.balancedQueryRec(nearIndex, queryData);
      }

      dist *= dist;
      if (
        (farIndex < this.numBalanced)
        && (((queryData.notFilled) && (dist < queryData.sqrRadius)) || (dist < queryData.maximumDistance))
      ) {
        this.balancedQueryRec(farIndex, queryData);
      }
    }

    dist = KDTree.sqrDistance3D(KDTree.asPoint(node.mData), queryData.point);

    if (dist < queryData.maximumDistance) {
      if (queryData.notFilled) {
        KDTree.mhInsert(queryData, node.mData, dist);
      }
      else {
        KDTree.mhReplaceMax(queryData, node.mData, dist);
      }
    }
  }

  private static bkdswap(root: BalancedKDTreeNode[], a: number, b: number): void {
    const tmp = root[a];
    root[a] = root[b];
    root[b] = tmp;
  }

  private static bkdval(root: BalancedKDTreeNode[], index: number, discr: number): number {
    return KDTree.asPoint(root[index].mData)[discr];
  }

  private static eSwap(broot: BalancedKDTreeNode[], a: number, b: number): void {
    KDTree.bkdswap(broot, a, b);
  }

  private static eVal(broot: BalancedKDTreeNode[], index: number, discr: number): number {
    return KDTree.bkdval(broot, index, discr);
  }

  private static getBalancedMedian(low: number, high: number): number {
    const n = high - low + 1;
    if (n <= 1) {
      return low;
    }

    const fl = globalThis.Math.trunc(globalThis.Math.log(n + 0.1) / globalThis.Math.log(2.0));
    const p2fl = (1 << fl);
    const lastN = n - (p2fl - 1);
    const lasts2 = globalThis.Math.trunc(p2fl / 2);

    let left: number;
    if (lastN < lasts2) {
      left = lastN + (lasts2 - 1);
    }
    else {
      left = lasts2 + lasts2 - 1;
    }

    return low + left;
  }

  private static quickSelect(broot: BalancedKDTreeNode[], low: number, high: number, discr: number): number {
    const median = KDTree.getBalancedMedian(low, high);

    while (true) {
      if (high <= low) {
        return median;
      }

      if (high === low + 1) {
        if (KDTree.eVal(broot, low, discr) > KDTree.eVal(broot, high, discr)) {
          KDTree.eSwap(broot, low, high);
        }
        return median;
      }

      const middle = globalThis.Math.trunc((low + high + 1) / 2);
      if (KDTree.eVal(broot, middle, discr) > KDTree.eVal(broot, high, discr)) {
        KDTree.eSwap(broot, middle, high);
      }
      if (KDTree.eVal(broot, low, discr) > KDTree.eVal(broot, high, discr)) {
        KDTree.eSwap(broot, low, high);
      }
      if (KDTree.eVal(broot, middle, discr) > KDTree.eVal(broot, low, discr)) {
        KDTree.eSwap(broot, middle, low);
      }

      KDTree.eSwap(broot, middle, low + 1);

      let ll = low + 1;
      let hh = high;
      while (true) {
        do {
          ll++;
        } while (KDTree.eVal(broot, low, discr) > KDTree.eVal(broot, ll, discr));

        do {
          hh--;
        } while (KDTree.eVal(broot, hh, discr) > KDTree.eVal(broot, low, discr));

        if (hh < ll) {
          break;
        }

        KDTree.eSwap(broot, ll, hh);
      }

      KDTree.eSwap(broot, low, hh);

      if (hh <= median) {
        low = ll;
      }
      if (hh >= median) {
        high = hh - 1;
      }
    }
  }

  private static copyUnbalancedRec(
    node: KDTreeNode | null,
    broot: BalancedKDTreeNode[],
    pindex: number[]
  ): void {
    if (node !== null) {
      broot[pindex[0]++].copy(node);
      KDTree.copyUnbalancedRec(node.loson, broot, pindex);
      KDTree.copyUnbalancedRec(node.hison, broot, pindex);
    }
  }

  private static bestDiscriminator(broot: BalancedKDTreeNode[], low: number, high: number): number {
    const bMin = [
      Numeric.HUGE_FLOAT_VALUE,
      Numeric.HUGE_FLOAT_VALUE,
      Numeric.HUGE_FLOAT_VALUE
    ];
    const bMax = [
      -Numeric.HUGE_FLOAT_VALUE,
      -Numeric.HUGE_FLOAT_VALUE,
      -Numeric.HUGE_FLOAT_VALUE
    ];

    for (let i = low; i <= high; i++) {
      let tmp = KDTree.bkdval(broot, i, 0);
      if (bMin[0] > tmp) {
        bMin[0] = tmp;
      }
      if (bMax[0] < tmp) {
        bMax[0] = tmp;
      }

      tmp = KDTree.bkdval(broot, i, 1);
      if (bMin[1] > tmp) {
        bMin[1] = tmp;
      }
      if (bMax[1] < tmp) {
        bMax[1] = tmp;
      }

      tmp = KDTree.bkdval(broot, i, 2);
      if (bMin[2] > tmp) {
        bMin[2] = tmp;
      }
      if (bMax[2] < tmp) {
        bMax[2] = tmp;
      }
    }

    let discr = 0;
    let spread = bMax[0] - bMin[0];

    let tmp = bMax[1] - bMin[1];
    if (tmp > spread) {
      discr = 1;
      spread = tmp;
    }

    tmp = bMax[2] - bMin[2];
    if (tmp > spread) {
      discr = 2;
    }

    return discr;
  }
}
