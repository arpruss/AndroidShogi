package mobi.omegacentauri.shogi;

import java.io.Serializable;
import java.util.ArrayList;
import java.util.Calendar;
import java.util.Collection;
import java.util.Comparator;
import java.util.GregorianCalendar;

import android.app.ActionBar;
import android.app.Activity;
import android.app.AlertDialog;
import android.app.Dialog;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.SharedPreferences;
import android.os.AsyncTask;
import android.os.Bundle;
import android.preference.PreferenceManager;
import android.util.Log;
import android.view.ContextMenu;
import android.view.ContextMenu.ContextMenuInfo;
import android.view.Menu;
import android.view.MenuInflater;
import android.view.MenuItem;
import android.view.View;
import android.widget.AdapterView;
import android.widget.AdapterView.AdapterContextMenuInfo;
import android.widget.ListView;
import android.widget.Toolbar;

/**
 * Activity that lists available game logs.
 *
 */
public class GameLogListActivity extends GenericListActivity<GameLog> {
  private static final String TAG = "PickLog";
  private GameLogListManager mGameLogList;

  private GameLogListManager.Mode mMode;
  private SharedPreferences mPrefs;

  @Override
  public void onCreate(Bundle savedInstanceState) {
    super.onCreate(savedInstanceState);
    mGameLogList = GameLogListManager.getInstance();

/*    if (android.os.Build.VERSION.SDK_INT >= android.os.Build.VERSION_CODES.HONEYCOMB) {
      ActionBar actionBar = getActionBar();
      if (actionBar != null) {
        //
      }
    } */

    initialize(
          null, /* no cache */
          0,    /* no cache */
          getResources().getString(R.string.game_logs),
          new GameLog[0]);

    mPrefs = PreferenceManager.getDefaultSharedPreferences(this);
    setSorter(GameLog.COMPARATORS[mPrefs.getInt("log_sorter", 0)]);

    ListView listView = (ListView)findViewById(android.R.id.list);
    listView.setStackFromBottom(true);
    registerForContextMenu(listView);

    startListing(GameLogListManager.Mode.READ_SDCARD_SUMMARY);
  }

  @Override
  public void setSorter(Comparator<GameLog> c) {
    super.setSorter(c);
    for (int i=0; i<GameLog.COMPARATORS.length; i++) {
      if (GameLog.COMPARATORS[i] == c) {
        mPrefs.edit().putInt("log_sorter", i).commit();
        return;
      }
    }
  }
  
  private void startListing(GameLogListManager.Mode mode) {
    mMode = mode;
    startListing(GenericListActivity.MAY_READ_FROM_CACHE);
  }

    
  private final GregorianCalendar mTmpCalendar = new GregorianCalendar();

  @Override
  public boolean onCreateOptionsMenu(Menu menu) {
    MenuInflater inflater = getMenuInflater();
    inflater.inflate(R.menu.game_log_list_menu, menu);
    return true;
  }
  @Override
  public String getListLabel(GameLog log) {
    StringBuilder b = new StringBuilder();
    long date = log.getDate();
    if (date > 0) {
      mTmpCalendar.setTimeInMillis(date);
      b.append(String.format("%04d/%02d/%02d ", 
          mTmpCalendar.get(Calendar.YEAR),
          mTmpCalendar.get(Calendar.MONTH) - Calendar.JANUARY + 1,
          mTmpCalendar.get(Calendar.DAY_OF_MONTH)));
    }
    if (log.path() != null) {
      b.append("[ext] ");
    } 
    String v = log.attr(GameLog.ATTR_BLACK_PLAYER);
    if (v != null) b.append(v);
    b.append("/");
    v = log.attr(GameLog.ATTR_WHITE_PLAYER);
    if (v != null) b.append(v);
    v = log.attr(GameLog.ATTR_TOURNAMENT);
    if (v != null) {
      b.append("/").append(v);
    } else {
      v = log.attr(GameLog.ATTR_TITLE);
      if (v != null) {
        b.append("/").append(v);
      }
    }
    b.append("/").append(log.numPlays());
    b.append(getResources().getString(R.string.plays_suffix));
    return b.toString();
  }
  
  @Override
  public int numStreams() { return 1; }

  @Override
  public GameLog[] readNthStream(int index) throws Throwable {
    Collection<GameLog> list = mGameLogList.listLogs(this, mMode);
    return list.toArray(new GameLog[0]);
  }
  
  @Override 
  public void onListItemClick(ListView l, View v, int position, long id) {
    replayGame(position);
  }
  
  private void replayGame(int position) {
    GameLog log = getObjectAtPosition(position);
    if (log == null) {
      Log.d(TAG, "Invalid item click: " + position);
      return;
    }
    Intent intent = new Intent(this, ReplayGameActivity.class);
    Serializable ss = log;
    intent.putExtra("gameLog", ss);
    startActivity(intent);
  }

  @Override protected Dialog onCreateDialog(int id) {
    switch (id) {
    case DIALOG_LOG_PROPERTIES:
    default:
      return null;
    }
  }
  
  @Override
  public void onCreateContextMenu(
      ContextMenu menu, 
      View v,
      ContextMenuInfo menuInfo) {
    super.onCreateContextMenu(menu, v, menuInfo);
    MenuInflater inflater = getMenuInflater();
    inflater.inflate(R.menu.game_log_list_context_menu, menu);
    GameLog log = getObjectAtPosition(((AdapterView.AdapterContextMenuInfo)menuInfo).position);
    
    if (log.path() != null) {
      menu.findItem(R.id.game_log_list_save_in_sdcard).setEnabled(false);
    }
  }  


  static final int DIALOG_LOG_PROPERTIES = 1;
  
  private class DeleteLogTask extends AsyncTask<GameLog, String, GameLogListManager.UndoToken> {
    private final Activity mActivity;
    DeleteLogTask(Activity a) { mActivity = a; }
    
    @Override
    protected GameLogListManager.UndoToken doInBackground(GameLog... log) {
      return mGameLogList.deleteLog(mActivity, log[0]);
    }
    
    @Override
    protected void onPostExecute(GameLogListManager.UndoToken undo) {
      if (undo != null) { // no error happened
        startListing(GameLogListManager.Mode.READ_SDCARD_SUMMARY);
      }
    }
  }

  private class SaveLogInSdcardTask extends AsyncTask<GameLog, String, String> {
    private final Activity mActivity;
    SaveLogInSdcardTask(Activity a) { mActivity = a; }
    
    @Override
    protected String doInBackground(GameLog... log) {
      mGameLogList.saveLogInSdcard(mActivity, log[0]);
      return null;
    }
    
    @Override
    protected void onPostExecute(String unused) {
      startListing(GameLogListManager.Mode.READ_SDCARD_SUMMARY);
    }
  }

  private class RemoveAllInMemoryTask extends AsyncTask<Integer, String, String> {
    @Override
    protected String doInBackground(Integer... age) {
      mGameLogList.removeLogsInMemory(GameLogListActivity.this, age[0]);
      return null;
    }

    @Override
    protected void onPostExecute(String unused) {
      startListing(GameLogListManager.Mode.READ_SDCARD_SUMMARY);
    }
  }

  private class UndoTask extends AsyncTask<GameLogListManager.UndoToken, String, String> {
    private final Activity mActivity;
    UndoTask(Activity a) { mActivity = a; }
    
    @Override
    protected String doInBackground(GameLogListManager.UndoToken... token) {
      mGameLogList.undo(mActivity, token[0]);
      return null;
    }
    
    @Override
    protected void onPostExecute(String unused) {
      startListing(GameLogListManager.Mode.READ_SDCARD_SUMMARY);
    }
  }

  @Override
  public boolean onContextItemSelected(MenuItem item) {
    AdapterContextMenuInfo info = (AdapterContextMenuInfo) item.getMenuInfo();
    switch (item.getItemId()) {
    case R.id.game_log_list_replay:
      replayGame(info.position);
      return true;
    case R.id.game_log_list_delete_log: {
      GameLog log = getObjectAtPosition(info.position);
      if (log != null) new DeleteLogTask(this).execute(log);
      return true;
    }
    case R.id.game_log_list_save_in_sdcard: {
      GameLog log = getObjectAtPosition(info.position);
      if (log != null) new SaveLogInSdcardTask(this).execute(log);
      return true;
    }
    case R.id.game_log_list_properties: {
      GameLog log = getObjectAtPosition(info.position);
      if (log != null) {
        final GameLogPropertiesView view = new GameLogPropertiesView(this);
        view.initialize(log);
        new AlertDialog.Builder(this)
        .setTitle(R.string.game_log_properties)
        .setView(view)
        .setPositiveButton(android.R.string.ok, new DialogInterface.OnClickListener() {
          @Override public void onClick(DialogInterface dialog, int whichButton) { }
        }).create().show();
      }
      return true;
    }
    default:
      return super.onContextItemSelected(item);
    }
  }

  @Override public boolean onOptionsItemSelected(MenuItem item) {
    switch (item.getItemId()) {
        case R.id.delete_old:
            new RemoveAllInMemoryTask().execute(14);
            return true;
        case R.id.delete_all:
          removeInMemory();
        return true;
      case R.id.menu_sort_by_date:
        setSorter(GameLog.SORT_BY_DATE);
        return true;
      case R.id.menu_sort_by_date_reversed:
        setSorter(GameLog.SORT_BY_DATE_REVERSED);
        return true;
      case R.id.menu_sort_by_black_player:
        setSorter(GameLog.SORT_BY_BLACK_PLAYER);
        return true;
      case R.id.menu_sort_by_white_player:
        setSorter(GameLog.SORT_BY_WHITE_PLAYER);
        return true;
      default:
        return super.onOptionsItemSelected(item);
    }
  }

  // delete all in-memory entries
  private void removeInMemory() {
    AlertDialog.Builder b = new AlertDialog.Builder(this);
    b.setTitle(R.string.remove_check);
    b.setMessage(R.string.remove_check_message);
    b.setPositiveButton(android.R.string.yes, new DialogInterface.OnClickListener() {
      @Override
      public void onClick(DialogInterface dialogInterface, int i) {
        new RemoveAllInMemoryTask().execute(0);
      }
    });
    b.show();
  }
}
