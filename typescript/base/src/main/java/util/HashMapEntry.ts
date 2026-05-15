export class HashMapEntry<K, V> {
  public key: K;
  public value: V;
  public next: HashMapEntry<K, V> | null;

  public constructor(inKey: K, inValue: V, inNext: HashMapEntry<K, V> | null) {
    this.key = inKey;
    this.value = inValue;
    this.next = inNext;
  }
}
