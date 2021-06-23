package com.ysaito.shogi.test;

import java.io.Serializable;

import com.ysaito.shogi.ExternalCacheManager;

import android.test.InstrumentationTestCase;

public class ExternalCacheManagerTest extends InstrumentationTestCase {
  @SuppressWarnings("serial")
  private static class TestObject implements Serializable {
    public final int value;
    TestObject(int v) { value = v; }
  }
  
  public void testBasic() {
    ExternalCacheManager cache = newCache();
    cache.write("foobar", new TestObject(1234));
    
    ExternalCacheManager.ReadResult r = cache.read("blah");
    assertTrue(r.obj == null);
    assertTrue(r.needRefresh);
    
    r = cache.read("foobar");
    assertEquals(1234, ((TestObject)r.obj).value);
    assertFalse(r.needRefresh);
  }
  
  private ExternalCacheManager mCache = null;
  private ExternalCacheManager newCache() {
    if (mCache == null) {
      mCache = ExternalCacheManager.getInstance(getInstrumentation().getTargetContext());
    }
    mCache.clearAll();
    return mCache;
  }
}
