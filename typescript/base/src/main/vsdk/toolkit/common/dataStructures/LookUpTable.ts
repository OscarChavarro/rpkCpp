import { LookUpBehaviors } from "./LookUpBehaviors";
import { LookUpEntity } from "./LookUpEntity";

export class LookUpTable<T> {
  private behaviorType: LookUpBehaviors;
  private currentTableSize: number;
  private table: Array<LookUpEntity<T>> | null;
  private numberOfDeletedEntries: number;

  private static readonly SHUFFLE: number[] = [
    0, 157, 58, 215, 116, 17, 174, 75, 232, 133, 34, 191, 92, 249, 150, 51,
    208, 109, 10, 167, 68, 225, 126, 27, 184, 85, 242, 143, 44, 201, 102, 3,
    160, 61, 218, 119, 20, 177, 78, 235, 136, 37, 194, 95, 252, 153, 54, 211,
    112, 13, 170, 71, 228, 129, 30, 187, 88, 245, 146, 47, 204, 105, 6, 163,
    64, 221, 122, 23, 180, 81, 238, 139, 40, 197, 98, 255, 156, 57, 214, 115,
    16, 173, 74, 231, 132, 33, 190, 91, 248, 149, 50, 207, 108, 9, 166, 67,
    224, 125, 26, 183, 84, 241, 142, 43, 200, 101, 2, 159, 60, 217, 118, 19,
    176, 77, 234, 135, 36, 193, 94, 251, 152, 53, 210, 111, 12, 169, 70, 227,
    128, 29, 186, 87, 244, 145, 46, 203, 104, 5, 162, 63, 220, 121, 22, 179,
    80, 237, 138, 39, 196, 97, 254, 155, 56, 213, 114, 15, 172, 73, 230, 131,
    32, 189, 90, 247, 148, 49, 206, 107, 8, 165, 66, 223, 124, 25, 182, 83,
    240, 141, 42, 199, 100, 1, 158, 59, 216, 117, 18, 175, 76, 233, 134, 35,
    192, 93, 250, 151, 52, 209, 110, 11, 168, 69, 226, 127, 28, 185, 86, 243,
    144, 45, 202, 103, 4, 161, 62, 219, 120, 21, 178, 79, 236, 137, 38, 195,
    96, 253, 154, 55, 212, 113, 14, 171, 72, 229, 130, 31, 188, 89, 246, 147,
    48, 205, 106, 7, 164, 65, 222, 123, 24, 181, 82, 239, 140, 41, 198, 99
  ];

  public constructor();
  public constructor(behaviorType: LookUpBehaviors);
  public constructor(behaviorType = LookUpBehaviors.NON_OWNING) {
    this.behaviorType = behaviorType;
    this.currentTableSize = 0;
    this.table = null;
    this.numberOfDeletedEntries = 0;
  }

  public getCurrentTableSize(): number {
    return this.currentTableSize;
  }

  private keysEqual(left: string, right: string): boolean {
    return left === right;
  }

  private freeKey(_key: string): void {
    if (this.behaviorType === LookUpBehaviors.OWNING) {
      // No-op in this TypeScript port.
    }
  }

  private freeData(_data: T): void {
    if (this.behaviorType === LookUpBehaviors.OWNING) {
      // No-op in this TypeScript port.
    }
  }

  private static lookUpShuffleHash(text: string): number {
    let bitShift = 0;
    let hash = 0;

    for (let i = 0; i < text.length; i++) {
      const b = text.charCodeAt(i) & 0xFF;
      hash ^= ((LookUpTable.SHUFFLE[b] ?? 0) << ((bitShift += 11) & 0xF));
    }

    return hash;
  }

  public lookUpInit(nel: number): number {
    const hSizeTab = [
      31, 61, 127, 251, 509, 1021, 2039, 4093, 8191, 16381,
      32749, 65521, 131071, 262139, 524287, 1048573, 2097143,
      4194301, 8388593, 0
    ];

    let chosenTableSize = 0;

    nel += nel >> 1;
    for (let i = 0; ; i++) {
      const bucketSize = hSizeTab[i];
      if (bucketSize === undefined || bucketSize === 0) {
        break;
      }
      if (bucketSize > nel) {
        chosenTableSize = bucketSize;
        break;
      }
    }

    this.currentTableSize = chosenTableSize;
    if (this.currentTableSize === 0) {
      this.currentTableSize = nel * 2 + 1;
    }

    this.table = new Array<LookUpEntity<T>>(this.currentTableSize);
    if (this.table === null) {
      this.currentTableSize = 0;
      return 0;
    }

    for (let i = 0; i < this.currentTableSize; i++) {
      const entry = new LookUpEntity<T>();
      entry.key = null;
      entry.data = null;
      entry.value = 0;
      this.table[i] = entry;
    }
    this.numberOfDeletedEntries = 0;

    return this.currentTableSize;
  }

  public lookUpFind(key: string): LookUpEntity<T> | null {
    if (this.currentTableSize <= 0) {
      if (this.lookUpInit(1) === 0) {
        return null;
      }
    }

    const hashValue = LookUpTable.lookUpShuffleHash(key);

    while (true) {
      let index = hashValue % this.currentTableSize;
      if (index < 0) {
        index += this.currentTableSize;
      }
      let i = 0;
      let n = -1;
      do {
        const entry = (this.table as Array<LookUpEntity<T>>)[index];
        if (entry === undefined) {
          return null;
        }
        if (entry.key === null) {
          entry.value = hashValue;
          return entry;
        }
        if (entry.value === hashValue && this.keysEqual(entry.key, key)) {
          return entry;
        }

        i++;
        n += 2;
        index += n;
        if (index >= this.currentTableSize) {
          index = index % this.currentTableSize;
        }
      } while (i < this.currentTableSize);

      if (this.lookUpReAlloc(this.currentTableSize - this.numberOfDeletedEntries + 1) === 0) {
        return null;
      }
    }
  }

  public lookUpDone(): void {
    if (this.currentTableSize <= 0 || this.table === null) {
      return;
    }

    for (let i = this.currentTableSize - 1; i >= 0; i--) {
      const entry = this.table[i];
      if (entry === undefined) {
        continue;
      }
      if (entry.key !== null) {
        this.freeKey(entry.key);
        if (entry.data !== null) {
          this.freeData(entry.data);
        }
      }
    }

    this.table = null;
    this.currentTableSize = 0;
    this.numberOfDeletedEntries = 0;
  }

  private lookUpReAlloc(nel: number): number {
    const oldTable = this.table;
    const oldTableSize = this.currentTableSize;
    const oldDeletedEntries = this.numberOfDeletedEntries;

    if (this.lookUpInit(nel) === 0) {
      this.table = oldTable;
      this.currentTableSize = oldTableSize;
      this.numberOfDeletedEntries = oldDeletedEntries;
      return 0;
    }

    for (let i = 0; i < oldTableSize; i++) {
      const entry = (oldTable as Array<LookUpEntity<T>>)[i];
      if (entry === undefined) {
        continue;
      }
      if (entry.key === null) {
        continue;
      }
      if (entry.data !== null) {
        const newEntry = this.lookUpFind(entry.key);
        if (newEntry === null) {
          continue;
        }
        newEntry.key = entry.key;
        newEntry.value = entry.value;
        newEntry.data = entry.data;
      }
      else {
        this.freeKey(entry.key);
      }
    }

    return this.currentTableSize;
  }
}
