package mobi.omegacentauri.shogi;

public class Assert {
  static public final boolean ENABLED = true;
  public static void isTrue(boolean v) {
    if (ENABLED && !v) {
      throw new AssertionError("true");
    }
  }
  public static void isFalse(boolean v) {
    if (ENABLED && v) {
      throw new AssertionError("false");
    }
  }
  public static void equals(Object o1, Object o2) {
    if (ENABLED && !o1.equals(o2)) {
      throw new AssertionError(o1.toString() + "!=" + o2.toString());
    }
  }
  public static void equals(int i1, int i2) {
    if (ENABLED && i1 != i2) {
      throw new AssertionError(String.format("%d != %d", i1, i2));
    }
  }
  public static void ge(int i1, int i2) { 
    if (ENABLED && i1 < i2) {
      throw new AssertionError(String.format("%d < %d", i1, i2));
    }
  }
  public static void lt(int i1, int i2) { 
    if (ENABLED && i1 >= i2) {
      throw new AssertionError(String.format("%d < %d", i1, i2));
    }
  }
}
