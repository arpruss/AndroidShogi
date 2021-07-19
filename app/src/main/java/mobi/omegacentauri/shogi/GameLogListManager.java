package mobi.omegacentauri.shogi;

import android.app.Activity;
import android.content.Context;
import android.content.SharedPreferences;
import android.preference.PreferenceManager;
import android.util.Log;
import android.widget.Toast;

import java.io.File;
import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.FilenameFilter;
import java.io.IOException;
import java.io.InputStream;
import java.io.ObjectInputStream;
import java.io.ObjectOutputStream;
import java.io.Serializable;
import java.util.ArrayList;
import java.util.Collection;
import java.util.Collections;
import java.util.HashMap;
import java.util.Map;

/**
 * 
 * A utility class for reading and writing game logs on memory and the local sdcard.
 * This class is thread safe. Most of the methods in this class may block while accessing the sdcard, so
 * they must be called in a non-main thread.
 *
 */
public class GameLogListManager {
  /**
   * Maximum number of logs that are not saved in the sdcard, but only in the summary file.
   * Exceeding this limit, oldest logs will be deleted.
   */
  private static final int MAX_IN_MEMORY_LOGS = 30;
  
  public static class UndoToken {
    public enum Op {  // the operation to undo 
      DELETED  // the log was deleted (the undo will restore the log)
    }
    public UndoToken(Op o, GameLog l) { op = o; log = l; }
    public final Op op;
    public final GameLog log;
  }

  private static String SUMMARY_PATH = "log_summary";
  private static String SAVED_GAME = "current_game";

  public enum Mode {
    READ_SDCARD_SUMMARY,
    RESET_SDCARD_SUMMARY,
  }

  private GameLogListManager() { }
  
  private static GameLogListManager mSingletonInstance;
  public static synchronized GameLogListManager getInstance() {
    if (mSingletonInstance == null) {
      mSingletonInstance = new GameLogListManager();
    }
    return mSingletonInstance;
  }

  /**
   * Find all the in-memory and in-sdcard game logs
   * 
   */
  public synchronized Collection<GameLog> listLogs(
      Context context, 
      Mode mode) {
    LogList summary = readSummary(context);
    if (mode == Mode.RESET_SDCARD_SUMMARY) {
      removeLogsInSdCard(summary);
    }
    
    long scanStartTimeMs = System.currentTimeMillis();
    scanDirectory(new File("/sdcard/download"), summary);
    scanDirectory(getLogDir(context), summary);

    // TODO: don't write the summary if it hasn't changed.
    summary.lastScanTimeMs = scanStartTimeMs;
    writeSummary(context, summary);

    return summary.logs.values();
  }

  private void removeLogsInSdCard(LogList summary) {
    summary.lastScanTimeMs = -1;
    ArrayList<String> to_remove = new ArrayList<String>();
    for (Map.Entry<String, GameLog> e : summary.logs.entrySet()) {
      if (e.getValue().path() != null) {
        to_remove.add(e.getKey());
      }
    }
    for (String key : to_remove) {
      summary.logs.remove(key);
    }
  }

  public void removeLogsInMemory(Context context) {
    LogList summary = readSummary(context);
    summary.lastScanTimeMs = -1;
    ArrayList<String> to_remove = new ArrayList<String>();
    for (Map.Entry<String, GameLog> e : summary.logs.entrySet()) {
      if (e.getValue().path() == null) {
        to_remove.add(e.getKey());
      }
    }
    for (String key : to_remove) {
      summary.logs.remove(key);
    }
    writeSummary(context, summary);
  }

  private void scanDirectory(File downloadDir, LogList summary) {
    String[] files = downloadDir.list(new FilenameFilter(){
      @Override
      public boolean accept(File dir, String filename) {
        return isHtml(filename) || isKif(filename);
      }
    });

    if (files == null) return;
    
    for (String basename: files) {
      File child = new File(downloadDir, basename);
      try {
        InputStream in = null;
        try {
          if (child.lastModified() >= summary.lastScanTimeMs) {
            in = new FileInputStream(child);
            GameLog log = null;
            if (isHtml(basename)) {
              log = GameLog.parseHtml(child, in);
            } else {
              log = GameLog.parseKif(child, in);
            }
            if (log != null) {
              summary.logs.put(log.digest(), log);
            }
          }
        } finally {
          if (in != null) in.close();
        }
      } catch (IOException e) {
        Log.d(TAG, child.getAbsolutePath() + ": I/O error: " + e.getMessage()); 
      } catch (ParseException e) {
        Log.d(TAG, child.getAbsolutePath() + ": KIF parse: " + e.getMessage());           
      }
    }
  }

  /**
   * Add a new game "log" in memory. 
   */
  public void saveLogInMemory(
      Activity activity,
      GameLog log) {
    LogList summary = readSummary(activity);
    if (summary == null) summary = new LogList();
    summary.logs.put(log.digest(), log);
    writeSummary(activity, summary);
    showToast(activity, activity.getResources().getString(R.string.saved_game_log_in_memory));
  }

  /**
   * Delete save.
   */
  public void deleteSaveGame(
          Activity activity
  ) {
    try {
      activity.deleteFile(SAVED_GAME);
    }
    catch(Exception e) {
    }
  }


  /**
   * Save game.
   */
  public void saveGame(
          Activity activity,
          GameLog log) {
    LogList summary = new LogList();
    summary.logs.put(log.digest(), log);
    deleteSaveGame(activity);
    writeSummary(activity, summary, SAVED_GAME);
//    showToast(activity, activity.getResources().getString(R.string.saved_game_log_in_memory));
  }

  /**
   * Add game "log" in sdcard. If "log" is in memory, it is removed from memory.
   */
  public void saveLogInSdcard(
      Activity activity,
      GameLog log) {
    File logFile = new File(getLogDir(activity), log.digest() + ".kif");
    try {
      saveInSdcard(activity, log, logFile);
      
      // Remove the in-memory log from the summary. The sdcard version of the log 
      // will be added back in ListLogs later.
      LogList summary = readSummary(activity);
      if (summary != null) {
        summary.logs.remove(log.digest());
        writeSummary(activity, summary);
      }
      
      showToast(activity, String.format(activity.getResources().getString(R.string.saved_log_in_sdcard), logFile.getAbsolutePath()));
    } catch (IOException e) {
      String message = "Error saving log: " + e.getMessage();
      showToast(activity, message);
    }
  }

  private void saveInSdcard(Activity activity, GameLog log, File logFile) throws IOException {
    SharedPreferences prefs = PreferenceManager.getDefaultSharedPreferences(activity);
    FileOutputStream stream = null; 
    try {
      logFile.getParentFile().mkdirs();
      stream = new FileOutputStream(logFile);
      log.toKif(stream, prefs.getString("log_save_format", "kif_dos"));
    } finally {
      if (stream != null) stream.close();
    }
  }
  
  /**
   * 
   * TODO this method assumes that context.openFileInput(path) opens the same file for a given path for any value of "context".
   */

  private LogList readSummary(Context context) {
    return readSummary(context, SUMMARY_PATH);
  }

  private LogList readSummary(Context context, String path) {
    LogList summary = new LogList();
    FileInputStream fin = null;
    try {
      try {
        fin = context.openFileInput(path);
        ObjectInputStream oin = new ObjectInputStream(fin);
        summary = (LogList)oin.readObject();
      } finally {
        if (fin != null) fin.close();
      }
    } catch (FileNotFoundException e) {
      ;
    } catch (ClassNotFoundException e) {
      Log.d(TAG, path + ": ClassNotFoundException: " + e.getMessage());
    } catch (IOException e) {
      Log.d(TAG, path + ": IOException: " + e.getMessage());
    }
    return summary;
  }

  private void removeOldInMemoryLogs(LogList summary) {
    ArrayList<GameLog> inmemory_logs = new ArrayList<GameLog>();
    for (GameLog log : summary.logs.values()) {
      if (log.path() == null) inmemory_logs.add(log);
    }
    if (inmemory_logs.size() > MAX_IN_MEMORY_LOGS) {
      Collections.sort(inmemory_logs, GameLog.SORT_BY_DATE);
      for (int i = 0; i < inmemory_logs.size() - MAX_IN_MEMORY_LOGS; ++i) {
        summary.logs.remove(inmemory_logs.get(i).digest());
      }
    }
  }

  private void writeSummary(Context context, LogList summary) {
    removeOldInMemoryLogs(summary);
    writeSummary(context, summary, SUMMARY_PATH);
  }
  
  private void writeSummary(Context context, LogList summary, String path) {
    FileOutputStream fout = null;
    try {
      try {
        fout = context.openFileOutput(path, Context.MODE_PRIVATE);
        ObjectOutputStream oout = new ObjectOutputStream(fout);
        oout.writeObject(summary);
        oout.close();
      } finally {
        fout.close();
      }
    } catch (FileNotFoundException e) {
      ;
    } catch (IOException e) {
      Log.d(TAG, SUMMARY_PATH + ": IOException: " + e.getMessage());
      ;
    }
  }

  private static boolean isHtml(String basename) {
    return basename.endsWith(".html") || basename.endsWith(".htm");
  }

  private static boolean isKif(String basename) {
    return basename.endsWith(".kif");
  }

  /**
   * Delete the given log in the background. Show a toast when done.
   */
  public synchronized UndoToken deleteLog(
      Activity activity,
      GameLog log) {
    String error = null;
    File path = log.path();
    if (path == null) {
      // The log hasn't been saved to the sdcard.
    } else {
      File trashPath = getTrashPath(activity, log);
      if (!path.renameTo(trashPath)) {
        trashPath.getParentFile().mkdirs();
        trashPath.delete();
        if (!path.renameTo(trashPath)) {
          error = "Could not delete " + path.getAbsolutePath();
          showToast(activity, error);
          return null;
        }
      }
    }
      
    LogList summary = readSummary(activity);
    if (summary == null) summary = new LogList();
    summary.logs.remove(log.digest());
    writeSummary(activity, summary);
    if (path == null) {
      showToast(activity, activity.getResources().getString(R.string.deleted_log_in_memory));
    } else {
      showToast(activity, String.format(activity.getResources().getString(R.string.deleted_log_in_sdcard), path.getAbsolutePath()));
    }
    return new UndoToken(UndoToken.Op.DELETED, log);
  }
  
  public synchronized void undo(
      Activity activity,
      UndoToken undo) {
    String error = null;
    File path = undo.log.path();
    if (path == null) {
      // The log hasn't been saved to the sdcard.
    } else {
      File trashPath = getTrashPath(activity, undo.log);
      if (!trashPath.renameTo(path)) {
        error = "Could not restore " + path.getAbsolutePath();
        showToast(activity, error);
        return;
      } 
    }
      
    LogList summary = readSummary(activity);
    if (summary == null) summary = new LogList();
    summary.logs.put(undo.log.digest(), undo.log);
    writeSummary(activity, summary);
    showToast(activity, activity.getResources().getString(R.string.restored_log));
  }

  private static File getTrashPath(Context context, GameLog mLog) {
    return new File(new File(getLogDir(context), "trash"), mLog.digest());
  }
  
  private static File getLogDir(Context context) {
    SharedPreferences prefs = PreferenceManager.getDefaultSharedPreferences(context);
    return new File(prefs.getString("game_log_dir", "/sdcard/ShogiGameLog"));
  }

  static private void showToast(Activity activity, String message) {
    class ShowToast implements Runnable {
      private final Activity mActivity;
      private final String mMessage;
      ShowToast(Activity a, String m) {
        mActivity = a;
        mMessage = m;
      }
      @Override
      public void run() {
        Toast.makeText(mActivity, mMessage, Toast.LENGTH_SHORT).show();
      }
    }
    activity.runOnUiThread(new ShowToast(activity, message));
  }
  

  // 
  // Implementation details
  //
  private static final String TAG = "ShogiLogLister";

  @SuppressWarnings("serial") 
  private static class LogList implements Serializable {
    public LogList() {
      lastScanTimeMs = -1;
      logs = new HashMap<String, GameLog>();
    }

    // The last walltime the filesystem was scanned.
    public long lastScanTimeMs;

    // maps gamelog digest -> gamelog
    public final HashMap<String, GameLog> logs;
  }
}
