package com.ysaito.shogi;

import java.io.File;
import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.ObjectInputStream;
import java.io.ObjectOutputStream;
import java.io.Serializable;
import java.security.MessageDigest;
import java.security.NoSuchAlgorithmException;
import java.util.HashMap;

import android.content.Context;
import android.util.Log;

/**
 * Class for managing a file system directory (Context.getCacheDir) as an LRU cache.
 * This class is generally MT safe, except for getInstance(), which must be called by the application's main 
 * event loop thread; it calls Context.getCacheDir, which doesn't look MT safe.
 */
public class ExternalCacheManager {
  private static final String TAG = "ExtenalCacheManager"; 
  private final File mDir;
  private final Context mContext;
  public static final int MAX_STATIC_PAGE_CACHE_STALENESS_MS = 7 * 24 * 3600 * 1000; // 1 week
  public static final int MAX_QUERY_CACHE_STALENSS_MS = 24 * 3600 * 1000; // 1 day
  
  // Cache key -> last access time
  private HashMap<String, Long> mLastAccessTimes;
  
  // The path where a serialized mLastAccessTimes is saved serialized.
  private static final String SUMMARY_PATH = "external_cache_summary";

  private static ExternalCacheManager mInstance;

  /**
   * Create and return a process-wide cache instance. This method must be called by the
   * main application event loop thread.
   * 
   * @param context It should be the value of getApplicationContext().
   * 
   * @param id The ID for this cache. It can be any string (it should be usable as a filename). 
   * It should be unique within the application, so that
   * multiple instances of this class can partition the per-application cache space.
   */
  public static ExternalCacheManager getInstance(Context context) {
    if (mInstance == null) {
      mInstance = new ExternalCacheManager(context);
    }
    Assert.isTrue(mInstance.mContext == context);
    return mInstance;
  }

  /** Convert the caller-supplied @p key to an internal key that can be safely used as a filename */
  private static String toKey(String k) {
    try {
      MessageDigest digest = MessageDigest.getInstance("SHA-1");
      digest.update(k.getBytes());
      return Util.bytesToHexText(digest.digest());
    } catch (NoSuchAlgorithmException e) {
      throw new AssertionError("MessageDigest.NoSuchAlgorithmException: " + e.getMessage());
    }
  }
  
  // TODO: add background purging
  
  @SuppressWarnings("unchecked")
  private ExternalCacheManager(Context context) {
    mContext = context;
    mDir = context.getExternalCacheDir();
    mDir.mkdirs();
    Log.d(TAG, "Cache=" + mDir.getAbsolutePath());
    mLastAccessTimes = new HashMap<String, Long>();
    
    FileInputStream summary_in = null;
    try {
      try {
        summary_in = context.openFileInput(SUMMARY_PATH);
        ObjectInputStream oin = new ObjectInputStream(summary_in);
        mLastAccessTimes = (HashMap<String, Long>)(oin.readObject());
      } finally {
        if (summary_in != null) summary_in.close();
      }
    } catch (FileNotFoundException e) {
      // ignore
    } catch (Throwable e) {
      Log.d(TAG, SUMMARY_PATH + ": failed to read summary: " + Util.throwableToString(e));
    }
  }
  
  /**
   * Clear the cache
   */
  public synchronized void clearAll() {
    try {
      for (String fileName : mDir.list()) {
        File f = new File(mDir, fileName);
        f.delete();
      }
      mLastAccessTimes = new HashMap<String, Long>();
      saveSummary();
    } catch (IOException e) {
      Log.d(TAG, SUMMARY_PATH + ": Failed to clear cache: " + Util.throwableToString(e));
    }
  }

  @SuppressWarnings("serial") 
  private static class CacheEntry implements Serializable {
    long createMs;  // The time the entry was created. Millisec since the epoch
    Serializable obj;
  }
  
  /**
   * Write the mapping key -> obj to the cache. Errors (e.g., IOError) are simply ignored.
   * 
   * @param key The cache entry key. It is used as the filename in the cache dir, so it shall not contain 
   * chars such as '/'.
   */
  public synchronized void write(String suppliedKey, Serializable obj) {
    final String key = toKey(suppliedKey);
    final CacheEntry ent = new CacheEntry();
    ent.createMs = System.currentTimeMillis();
    ent.obj = obj;
    
    FileOutputStream data_out = null;
    try {
      try {
        File path = new File(mDir, key);
        data_out = new FileOutputStream(path);
        new ObjectOutputStream(data_out).writeObject(ent);
        mLastAccessTimes.put(key, System.currentTimeMillis());
        saveSummary();
      } finally {
        if (data_out != null) data_out.close();
      }
    } catch (IOException e) {
      Log.d(TAG, "Failed to write: " + Util.throwableToString(e));
    }
  }
  
  /**
   * If object for "key" is in the cache, return it. Else, or in case of an error (e.g., IOError)
   * return null.
   */
  public static class ReadResult {
    public Object obj;
    public boolean needRefresh;
  }
  
  public synchronized ReadResult read(
      String suppliedKey,
      int maxCacheStalenessMillis) {
    final String key = toKey(suppliedKey);
    final long now = System.currentTimeMillis(); 
    ReadResult r = new ReadResult();
    r.obj = null;
    r.needRefresh = true;
    FileInputStream data_in = null;
    try {
      try {
        File path = new File(mDir, key);
        data_in = new FileInputStream(path);
        CacheEntry ent = (CacheEntry)(new ObjectInputStream(data_in).readObject());
        if (ent != null) {
          r.obj = ent.obj;
          r.needRefresh = (now - ent.createMs >= maxCacheStalenessMillis);
        }
        mLastAccessTimes.put(key, now);
        saveSummary();
      } finally {
        if (data_in != null) data_in.close();
      }
    } catch (FileNotFoundException e) {
    } catch (Throwable e) {
      Log.d(TAG, SUMMARY_PATH + ": failed to write: " + Util.throwableToString(e));
    }
    return r;
  }

  private void saveSummary() throws IOException {
    FileOutputStream out = null;
    try {
      out = mContext.openFileOutput(SUMMARY_PATH, Context.MODE_PRIVATE);
      new ObjectOutputStream(out).writeObject(mLastAccessTimes);
    } finally {
      if (out != null) out.close();
    }
  }
}
