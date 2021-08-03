package mobi.omegacentauri.shogi;

// TODO: don't use engine for undo?

import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.util.ArrayList;
import java.util.TreeMap;

import android.app.ActionBar;
import android.app.Activity;
import android.app.AlertDialog;
import android.app.Dialog;
import android.content.Context;
import android.content.DialogInterface;
import android.content.SharedPreferences;
import android.os.AsyncTask;
import android.os.Build;
import android.os.Bundle;
import android.os.Handler;
import android.os.Message;
import android.os.Parcel;
import android.os.SystemClock;
import android.preference.PreferenceManager;
import android.util.Log;
import android.view.ContextMenu;
import android.view.KeyEvent;
import android.view.MenuInflater;
import android.view.MenuItem;
import android.view.MotionEvent;
import android.view.View;
import android.view.WindowManager;
import android.widget.AdapterView;
import android.widget.TextView;
import android.widget.Toast;

/**
 * The main activity that controls game play
 */
public class GameActivity extends Activity {
  private static final String TAG = "Shogi";

  private static final boolean NEW_SAVES = true; // not yet production ready

  private static final String SAVE_BUNDLE = "save.bundle";
  private static final int SAVE_BUNDLE_VERSION = 0x12340003;
  private static final int DIALOG_PROMOTE = 1235;
  private static final int DIALOG_CONFIRM_QUIT = 1236;

  // Config parameters
  //
  // List of players played by humans. The list size is usually one, when one side is 
  // played by Human and the other side by the computer.
  private ArrayList<Player> mHumanPlayers;

  // Number of undos remaining.
  //
  // TODO(saito) This works only when there's only one human player in the game.
  // Make this field per-player attribute.
  private int mUndosRemaining;

  // Constants after onCreate().
  private Activity mActivity; 
  private GameLogListManager mGameLogList;
  
  // View components
  private AlertDialog mPromoteDialog;
  private BonanzaController mController;
  private BoardView mBoardView;
  private GameStatusView mStatusView;

  // Game preferences
  private int mComputerLevel;      // 0 .. 4
  private String mPlayerTypes;     // "HC", "CH", "HH", "CC"
  private boolean mFlipScreen;
  private Handicap mHandicap;
  
  // State of the game
  private long mStartTimeMs;       // Time the game started (UTC millisec)
  private Board mBoard;            // current state of the board
  private Player mNextPlayer;   // the next player to make a move 
  private GameState mGameState;    // is the game is active or finished?

  private long[] mThinkTimeMs = {-1L,-1L}; // cumulative think time for each player
  private long[] mThinkStartMs = {-1L,-1L}; // start of current thinking
  private boolean mDestroyed;      // onDestroy called?
  private boolean mDidHumanMove;

  // History of plays made in the game. Even (resp. odd) entries are 
  // moves by the black (resp. white) player.
  private ArrayList<Play> mPlays;
  private ArrayList<Integer> mMoveCookies;
  private SharedPreferences mPrefs;
  private KeyboardControl mKeyboardControl;
  private Board mInitialBoard;

  @Override
  public void onCreate(Bundle savedInstanceState) {
    super.onCreate(savedInstanceState);
// this is usually unnecessary, but just in case something got pushed out of memory...
    BonanzaJNI.initialize(StartScreenActivity.getExternalDir(this).getAbsolutePath());

    mActivity = this;
    mGameLogList = GameLogListManager.getInstance();
    if (Build.VERSION.SDK_INT < 16) {
      getWindow().setFlags(WindowManager.LayoutParams.FLAG_FULLSCREEN,
              WindowManager.LayoutParams.FLAG_FULLSCREEN);
    }
    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.HONEYCOMB) {
      ActionBar a = getActionBar();
      if (a != null)
        a.hide();
    }
    setContentView(R.layout.game);
    initializeInstanceState(savedInstanceState);
    mDidHumanMove = false;
    mStatusView = (GameStatusView)findViewById(R.id.gamestatusview);
    mStatusView.initialize(
        playerName(mPlayerTypes.charAt(0), mComputerLevel),
        playerName(mPlayerTypes.charAt(1), mComputerLevel),
            mFlipScreen);

    mKeyboardControl = new KeyboardControl();
    mBoardView = (BoardView)findViewById(R.id.boardview);
    mBoardView.initialize(mViewListener, mHumanPlayers, mFlipScreen, mKeyboardControl);
    mBoardView.update(mGameState, null, mBoard, mNextPlayer, null, false);
    mBoardView.requestFocus();
    mStatusView.update(mGameState,
            mBoard, mBoard,
            mPlays, mNextPlayer, null);
    mStatusView.updateThinkTimes(mThinkTimeMs);
    int numCores = Math.min(Util.numberOfCores(), Integer.parseInt(mPrefs.getString("cores","4")));
    mController = new BonanzaController(mEventHandler, mComputerLevel, numCores);
    if (mGameState == GameState.ACTIVE) {
      if (/*! NEW_SAVES ||*/ savedInstanceState != null)
        mController.start(savedInstanceState, mBoard, mNextPlayer, null, 0, null);
      else
        mController.start(savedInstanceState, mInitialBoard, Player.BLACK, mPlays, mPlays.size(), mThinkTimeMs);
    }

    registerForContextMenu(findViewById(R.id.menu_button));

    mKeyboardControl.add(new KeyboardControl.CursorPosition(findViewById(R.id.menu_button),null, null, 0,0, null));

    schedulePeriodicTimer();
    // mController will call back via mControllerHandler when Bonanza is 
    // initialized. mControllerHandler will cause mBoardView to start accepting
    // user inputs.
    //resetTime();
  }

  static void deleteSaveActiveGame(Context c) {
    if (! NEW_SAVES)
      return;
    try {
      c.deleteFile(SAVE_BUNDLE);
    }
    catch(Exception e) {
    }
  }

  void saveActiveGame() {
    final Bundle b = new Bundle();

    saveInstanceState(b);
    b.putInt("save_version", SAVE_BUNDLE_VERSION);

    Runnable background = new Runnable() {
      @Override
      public void run() {
        try {
          FileOutputStream out = openFileOutput(SAVE_BUNDLE, MODE_PRIVATE);
          Parcel p = Parcel.obtain();
          b.writeToParcel(p, 0);
          out.write(p.marshall());
          out.close();
        } catch (Exception e) {
        }
      }
    };
    new Thread(background).start();
  }

  @Override
  public void onSaveInstanceState(Bundle bundle) {
    saveInstanceState(bundle);
    mController.saveInstanceState(bundle);
  }

  @Override
  public boolean dispatchKeyEvent(KeyEvent event) {
    if (event.getAction() == KeyEvent.ACTION_DOWN && mKeyboardControl.onKeyDown(event.getKeyCode(),event))
      return true;
    return super.dispatchKeyEvent(event);
  }

  private final String playerName(char type, int level) {
    if (type == 'H') {
      SharedPreferences prefs = PreferenceManager.getDefaultSharedPreferences(this);
      return Util.humanSafeName(prefs.getString("human_player_name",
          (String) getResources().getText(R.string.default_human_player_name)));
    } else {
      return getResources().getStringArray(R.array.computer_level_names)[level];
    }
  }

  @Override public void onDestroy() {
    Log.d(TAG, "ShogiActivity destroyed");
    mController.destroy();
    super.onDestroy();
    mDestroyed = true;
  }

  private void tryQuitGame() {
    if (mGameState == GameState.ACTIVE && !mPlays.isEmpty()) {
      showDialog(DIALOG_CONFIRM_QUIT);
    } else {
      super.onBackPressed();
    }
  }
  
  @Override public void onBackPressed() {
    tryQuitGame();
  }

  @Override protected Dialog onCreateDialog(int id) {
    switch (id) {
    case DIALOG_PROMOTE: 
      mPromoteDialog = createPromoteDialog();
      return mPromoteDialog;
    case DIALOG_CONFIRM_QUIT:
      return createConfirmQuitDialog();
    default:    
      return null;
    }
  }

  private final boolean isComputerPlayer(Player p) {
    return p != Player.INVALID && !isHumanPlayer(p);
  }

  private final boolean isHumanPlayer(Player p) {
    return mHumanPlayers.contains(p);
  }

  private final void saveInstanceState(Bundle b) {
    b.putLong("shogi_undos_remaining", mUndosRemaining);
    b.putLong("shogi_black_think_time_ms", mThinkTimeMs[Player.BLACK.toIndex()]);
    b.putLong("shogi_white_think_time_ms", mThinkTimeMs[Player.WHITE.toIndex()]);
    b.putLong("shogi_black_think_start_ms", mThinkStartMs[Player.BLACK.toIndex()]);
    b.putLong("shogi_white_think_start_ms", mThinkStartMs[Player.WHITE.toIndex()]);
    b.putLong("shogi_start_time_ms", mStartTimeMs);
      Log.v("shogi", "save next player "+mNextPlayer);
    b.putLong("shogi_next_player", (mNextPlayer == Player.BLACK) ? 0 : 1);
    b.putSerializable("shogi_moves", mPlays);
    b.putSerializable("shogi_move_cookies", mMoveCookies);
    b.putSerializable("saved_board", mBoard);
    b.putSerializable("game_state", mGameState);
    b.putBoolean("flipped", mFlipScreen);
  }

  public static Bundle getSaveActiveGame(Context c) {
    if (! NEW_SAVES)
      return null;
    Bundle b;
    try {
      FileInputStream in = c.openFileInput(SAVE_BUNDLE);
      byte[] data = new byte[(int)in.getChannel().size()];
      in.read(data);
      in.close();
      Parcel p = Parcel.obtain();
      p.unmarshall(data, 0, data.length);
      p.setDataPosition(0);
      b = p.readBundle();
      if (b.getInt("save_version") == SAVE_BUNDLE_VERSION) {
        return b;
      }
      else {
        deleteSaveActiveGame(c);
        return null;
      }
    }
    catch(Exception e) {
      Log.v("shogilog", "Restoring error: "+e);
      return null;
    }

  }

  @SuppressWarnings(value="`unchecked")
  private final void initializeInstanceState(Bundle b) {
    boolean setTimeFromPlays = false;
    mPrefs = PreferenceManager.getDefaultSharedPreferences(
        getBaseContext());
    if (b == null) {
      b = getSaveActiveGame(this);
      if ( b != null ) {
        mDidHumanMove = true;
        setTimeFromPlays = true;
      }
    }
    mUndosRemaining = (int)initializeLong(b, "shogi_undos_remaining", mPrefs, "max_undos", 0);
    mThinkTimeMs[Player.BLACK.toIndex()] = initializeLong(b, "shogi_black_think_time_ms", null, null, 0);
    mThinkTimeMs[Player.WHITE.toIndex()] = initializeLong(b, "shogi_white_think_time_ms", null, null, 0);
    mThinkStartMs[Player.BLACK.toIndex()] = initializeLong(b, "shogi_black_think_start_ms", null, null, 0);
    mThinkStartMs[Player.WHITE.toIndex()] = initializeLong(b, "shogi_white_think_start_ms", null, null, 0);
    mStartTimeMs = initializeLong(b, "shogi_start_time_ms", null, null, System.currentTimeMillis());
    long nextPlayer = initializeLong(b, "shogi_next_player", null, null, -1);
    if (nextPlayer >= 0) {
      Log.v("shogi", "restore next player "+nextPlayer);
      mNextPlayer = (nextPlayer == 0 ? Player.BLACK : Player.WHITE);
    }
    mGameState = null;
    if (b != null) {
      mPlays = (ArrayList<Play>) b.getSerializable("shogi_moves");
      mMoveCookies = (ArrayList<Integer>) b.getSerializable("shogi_move_cookies");
      mGameState = (GameState) b.getSerializable("game_state");
    }
    if (mGameState == null)
      mGameState = GameState.ACTIVE;

    mPlayerTypes = mPrefs.getString("player_types", "HC");
    mHumanPlayers = new ArrayList<Player>();
    if (mPlayerTypes.charAt(0) == 'H') {
      mHumanPlayers.add(Player.BLACK);
    }
    if (mPlayerTypes.charAt(1) == 'H') {
      mHumanPlayers.add(Player.WHITE);      
    }
    mFlipScreen = mPlayerTypes.charAt(1) == 'H' && mPlayerTypes.charAt(0) != 'H';
    if (b != null)
      mFlipScreen = b.getBoolean("flipped", mFlipScreen);
    mComputerLevel = Integer.parseInt(mPrefs.getString("computer_difficulty", "1"));

    mHandicap = (Handicap)getIntent().getSerializableExtra("handicap");
    if (mHandicap == null) mHandicap = Handicap.NONE;
    boolean resetTime = getIntent().getBooleanExtra("reset_time",false);
    
    // The "initial_board" intent extra is always set (the handicap setting is reported here).
    //
    // Note: if we are resuming via saveInstanceState (e.g., screen rotation), the initial
    // value of mBoard is ultimately irrelevant. mController.start() will retrieve the board state
    // just before interruption and report it via the event listener. However, we use a saved
    // board state just in case the engine is thinking, so the user doesn't have to wait for the
    // update.
    mBoard = null;
    if (b != null)
      mBoard = (Board)b.getSerializable("saved_board");
    if (mBoard == null)
      mBoard = (Board)getIntent().getSerializableExtra("saved_board");
    mInitialBoard = (Board)getIntent().getSerializableExtra("initial_board");
    if (mBoard == null)
      mBoard = mInitialBoard;
    
    // Resuming a saved game will set "moves" and "next_player" intent extras.
    if (mNextPlayer == null) {
      mNextPlayer = (Player)getIntent().getSerializableExtra("next_player");
    }
    if (mPlays == null) {
      mPlays = (ArrayList<Play>)getIntent().getSerializableExtra("moves");
      if (mPlays != null) {
        mMoveCookies = new ArrayList<Integer>();
        for (int i = 0; i < mPlays.size(); ++i) mMoveCookies.add(null);
      }
    }
    // If we aren't replaying a saved game, and we aren't resuming via saveInstanceState (e.g., screen rotation),
    // then set the default board state.
    if (mNextPlayer == null) {
      mNextPlayer = Player.BLACK;
    }
    if (mPlays == null) {
      mPlays = new ArrayList<Play>();
      mMoveCookies = new ArrayList<Integer>();
    }

    BonanzaJNI.abort();

    // TODO: check
    if (resetTime) {
      mThinkStartMs[0] = mThinkStartMs[1] = -1;
      mThinkTimeMs[0] = mThinkTimeMs[1] = -1;
      for (Play p : mPlays)
        p.setTime(-1,-1);
    }
    else if (setTimeFromPlays || (mThinkStartMs[0] <= 0 && mThinkStartMs[1] <=0))
      setTimesFromPlays();
  }

  @Override
  protected void onResume() {
    super.onResume();
    if (mThinkStartMs[0] <= 0 && mThinkStartMs[1] <= 0)
      resetTime();
  }

  private void resetTime() {
      final long now = SystemClock.uptimeMillis();
      mThinkStartMs[0]  = mThinkStartMs[1] = 0;
      if (mNextPlayer != Player.INVALID) {
        mThinkStartMs[mNextPlayer.toIndex()] = now;
      }
  }

  private final long initializeLong(Bundle b, String bundle_key, SharedPreferences prefs, String pref_key, long dflt) {
    long v = dflt;
    if (b != null) {
      v = b.getLong(bundle_key, dflt);
      if (v != dflt) return v;
    }
    if (prefs != null) {
      return Integer.parseInt(prefs.getString(pref_key, String.valueOf(dflt)));
    } else {
      return v;
    }
  }

  // 
  // Periodic status update
  //
  private final Runnable mTimerHandler = new Runnable() {
    public void run() {
      if (mNextPlayer != Player.INVALID) {
        long now = SystemClock.uptimeMillis();
        long[] totals = { mThinkTimeMs[0], mThinkTimeMs[1] };
        totals[mNextPlayer.toIndex()] += (now - mThinkStartMs[mNextPlayer.toIndex()]);
        mStatusView.updateThinkTimes(totals);
      }
      else {
        mStatusView.updateThinkTimes(mThinkTimeMs);
      }
      if (!mDestroyed) schedulePeriodicTimer();
    }
  };

  private final void setTimesFromPlays() {
    Util.getTimesFromPlays(mPlays, mPlays.size(), mThinkTimeMs);
//    resetTime();
  }
  
  private final void setCurrentPlayer(Player p, Play lastMove) {
    // Register the time spent during the last move.
    final long now = SystemClock.uptimeMillis();
    long delta;

    if (lastMove != null) {
      int current = lastMove.player().toIndex();
      if (mThinkStartMs[current] > 0) {
        delta = now - mThinkStartMs[current];
        if (lastMove != null) {
          lastMove.setTime(mThinkTimeMs[current], mThinkTimeMs[current]+delta);
        }
        mThinkTimeMs[current] += delta;
      }
    }

    // Switch the player, and start its timer.
    mNextPlayer = p;
    resetTime();
  }

  private final void schedulePeriodicTimer() {
    mEventHandler.postDelayed(mTimerHandler, 1000);
  }
  //
  // Undo
  //
  private final void undo() {
    if (!isHumanPlayer(mNextPlayer)) {
      Toast.makeText(getBaseContext(), "Computer is thinking", 
          Toast.LENGTH_SHORT).show();
      return;
    } 
    if (mMoveCookies.size() < 2) return;
    Integer u1 = 0;//mMoveCookies.get(mMoveCookies.size() - 1);
    Integer u2 = 0;//mMoveCookies.get(mMoveCookies.size() - 2);
    //if (u1 == null || u2 == null) return;  // happens when resuming a saved game

    int lastMove = u1;
    int penultimateMove = u2;
    mController.undo2(mNextPlayer, lastMove, penultimateMove);
    setCurrentPlayer(Player.INVALID, null);
    --mUndosRemaining;
  }

  //
  // Handling results from the Bonanza controller thread
  //
  private final Handler mEventHandler = new Handler() {
    @Override public void handleMessage(Message msg) {
      BonanzaController.Result r = (BonanzaController.Result)(
          msg.getData().get("result"));

      if (r.gameState != GameState.ACTIVE) {
        deleteSaveActiveGame(GameActivity.this);
      }

      if (r.lastMove != null) {
        mPlays.add(r.lastMove);
        mMoveCookies.add(r.lastMoveCookie);
      }
      setCurrentPlayer(r.nextPlayer, r.lastMove);
      for (int i = 0; i < r.undoMoves; ++i) {
        Assert.isTrue(r.lastMove == null);
        mPlays.remove(mPlays.size() - 1);
        mMoveCookies.remove(mMoveCookies.size() - 1);
      }

      if (r.undoMoves>0) {
        setTimesFromPlays();
        resetTime();
        BonanzaJNI.resetTime((int) ((mThinkTimeMs[Player.BLACK.toIndex()] + 500) / 1000), (int) ((mThinkTimeMs[Player.WHITE.toIndex()] + 500) / 1000));
      }

      mBoardView.update(
          r.gameState, mBoard, r.board, r.nextPlayer, 
          r.lastMove,
          !isComputerPlayer(r.nextPlayer)); /* animate if lastMove was made by the computer player */
      mStatusView.update(r.gameState,
          mBoard, r.board,
          mPlays, r.nextPlayer, r.errorMessage);

      mGameState = r.gameState;
      mBoard = r.board;
      if (isComputerPlayer(r.nextPlayer)) {
        mController.computerMove(r.nextPlayer);
      }
      if (mGameState != GameState.ACTIVE && mPrefs.getBoolean("auto_log", false)) {
        maybeSaveGame();
      }
      if (isHumanPlayer(r.lastPlayer)) {
        mDidHumanMove = true;
        if (mGameState == GameState.ACTIVE)
          saveActiveGame();
      }
    }
  };

  //
  // Handling of move requests from BoardView
  //

  // state kept during the run of promotion dialog
  private Player mSavedPlayerForPromotion;
  private Play mSavedPlayForPromotion;    

  private final BoardView.EventListener mViewListener = new BoardView.EventListener() {
    public void onHumanPlay(Player player, Play play) {
      //setCurrentPlayer(Player.INVALID, play);
      mNextPlayer = Player.INVALID;
      if (PlayAllowsForPromotion(player, play)) {
        mSavedPlayerForPromotion = player;
        mSavedPlayForPromotion = play;
        showDialog(DIALOG_PROMOTE);
      } else {
        mController.humanPlay(player, play);
      }
    }
  };

  private void maybeSaveGame() {
    if (mDidHumanMove && mPlays.size() > 0) {
      TreeMap<String, String> attrs = new TreeMap<String, String>();
      attrs.put(GameLog.ATTR_BLACK_PLAYER, blackPlayerName());
      attrs.put(GameLog.ATTR_WHITE_PLAYER, whitePlayerName());
      if (mHandicap != Handicap.NONE) {
        attrs.put(GameLog.ATTR_HANDICAP, mHandicap.toJapaneseString());
      }

      new AsyncTask<GameLog, String, String>() {
        @Override
        protected String doInBackground(GameLog... logs) {
          mGameLogList.saveLogInMemory(mActivity, logs[0], true);
          return null;
        }
      }.execute(GameLog.newLog(mStartTimeMs, attrs.entrySet(), mPlays,  null /* not on sdcard yet */));
      mDidHumanMove = false;
    }
  }
  
  private String blackPlayerName() {
    return playerName(mPlayerTypes.charAt(0), mComputerLevel);
  }
  
  private String whitePlayerName() {
    return playerName(mPlayerTypes.charAt(1), mComputerLevel);
  }
  
  private final AlertDialog createPromoteDialog() {
    AlertDialog.Builder b = new AlertDialog.Builder(this);
    b.setTitle(R.string.promote_piece);
    b.setCancelable(true);
    b.setOnCancelListener(
        new DialogInterface.OnCancelListener() {
          public void onCancel(DialogInterface unused) {
            if (mSavedPlayForPromotion == null) {
              // Event delivered twice?
            } else {
              mBoardView.update(mGameState, null, mBoard, mNextPlayer, null, false);
              mSavedPlayForPromotion = null;
              mSavedPlayerForPromotion = null;
            }
          }
        });
    b.setItems(
        new CharSequence[] {
            getResources().getString(R.string.promote), 
            getResources().getString(R.string.do_not_promote) },
            new DialogInterface.OnClickListener() {
          public void onClick(DialogInterface d, int item) {
            if (mSavedPlayForPromotion == null) {
              // A click event delivered twice?
              return;
            }

            if (item == 0) {
              mSavedPlayForPromotion = new Play(
                  Board.promote(mSavedPlayForPromotion.piece()), 
                  mSavedPlayForPromotion.fromX(), mSavedPlayForPromotion.fromY(),
                  mSavedPlayForPromotion.toX(), mSavedPlayForPromotion.toY());
            }
            mController.humanPlay(mSavedPlayerForPromotion, mSavedPlayForPromotion);
            mSavedPlayForPromotion = null;
            mSavedPlayerForPromotion = null;
          }
        });
    return b.create();
  }

  private static final boolean PlayAllowsForPromotion(Player player, Play play) {
    if (Board.isPromoted(play.piece())) return false;  // already promoted

    final int type = Board.type(play.piece());
    if (type == Piece.KIN || type == Piece.OU) return false;

    if (play.isDroppingPiece()) return false;
    if (player == Player.WHITE && play.fromY() < 6 && play.toY() < 6) return false;
    if (player == Player.BLACK && play.fromY() >= 3 && play.toY() >= 3) return false;
    return true;
  }

  //
  // Confirm quitting the game ("BACK" button interceptor)
  //
  private final AlertDialog createConfirmQuitDialog() {
    AlertDialog.Builder builder = new AlertDialog.Builder(this);
    builder.setMessage(R.string.confirm_quit_game);
    builder.setCancelable(false);
    builder.setPositiveButton(android.R.string.yes, new DialogInterface.OnClickListener() {
      public void onClick(DialogInterface d, int id) {
        maybeSaveGame();
        deleteSaveActiveGame(GameActivity.this);
        finish();
      }
    });
    if (mDidHumanMove)
      builder.setNeutralButton(R.string.yes_no_save, new DialogInterface.OnClickListener() {
        public void onClick(DialogInterface d, int id) {
          deleteSaveActiveGame(GameActivity.this);
          finish();
        }
      });
    return builder.create();
  }

  @Override
  public void onCreateContextMenu(
          ContextMenu menu,
          View v,
          ContextMenu.ContextMenuInfo menuInfo) {
    super.onCreateContextMenu(menu, v, menuInfo);
    MenuInflater inflater = getMenuInflater();
    inflater.inflate(R.menu.game_context_menu, menu);
    menu.findItem(R.id.game_undo).setEnabled(mUndosRemaining > 0 && mHumanPlayers.size() > 0);
    menu.findItem(R.id.log).setEnabled(mDidHumanMove || mHumanPlayers.size() == 0);
  }

  @Override
  public boolean onContextItemSelected(MenuItem item) {
    // Handle item selection
    switch (item.getItemId()) {
      case R.id.game_flip:
        mFlipScreen = !mFlipScreen;
        mBoardView.setFlipScreen(mFlipScreen);
        mStatusView.setFlipScreen(mFlipScreen);
        return true;
      case R.id.game_undo:
        undo();
        return true;
      case R.id.log:
        maybeSaveGame();
        return true;
      default:
        return super.onOptionsItemSelected(item);
    }
  }

  public void menuButtonClick(View view) {
    openContextMenu(view);
  }

//  private class BonanzaInitializeThread extends Thread {
//    @Override public void run() {
//      BonanzaJNI.initialize(getExternalFilesDir(null).getAbsolutePath());
//    }
//  }

}