package mobi.omegacentauri.shogi;

import java.util.ArrayList;

import android.app.ActionBar;
import android.app.Activity;
import android.app.AlertDialog;
import android.app.Dialog;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.SharedPreferences;
import android.os.AsyncTask;
import android.os.Build;
import android.os.Bundle;
import android.preference.PreferenceManager;
import android.util.Log;
import android.view.MotionEvent;
import android.view.View;
import android.view.WindowManager;
import android.widget.ImageButton;
import android.widget.SeekBar;
import android.widget.TextView;

/**
 * Activity for replaying a saved game
 */
public class ReplayGameActivity extends Activity {
    // View components
    private BoardView mBoardView;
    private GameStatusView mStatusView;
    private SeekBar mSeekBar;

    private Activity mActivity;
    private GameLogListManager mGameLogList;

    // Game preferences
    private boolean mFlipScreen;

    // State of the game
    private Board mBoard;            // current state of the board
    private ArrayList<Play> mPlays;  // plays made up to mBoard.
    private Player mNextPlayer;   // the next player to make a move
    private GameState mGameState;    // is the game is active or finished?

    private GameLog mLog;

    // Number of moves made so far. 0 means the beginning of the game.
    private int mNextPlay;

    private SharedPreferences mPrefs;

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        mPrefs = PreferenceManager.getDefaultSharedPreferences(
                getBaseContext());

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
        setContentView(R.layout.replay_game);
        initializeInstanceState(savedInstanceState);

        mLog = (GameLog) getIntent().getSerializableExtra("gameLog");
        Assert.isTrue(mLog.numPlays() > 0);

        mGameState = GameState.ACTIVE;
        mNextPlay = 0;
        mBoard = new Board();
        mPlays = new ArrayList<Play>();
        mBoard.initialize(mLog.handicap());
        mNextPlayer = Player.BLACK;

        mStatusView = (GameStatusView) findViewById(R.id.gamestatusview);
        mStatusView.initialize(
                mLog.attr(GameLog.ATTR_BLACK_PLAYER),
                mLog.attr(GameLog.ATTR_WHITE_PLAYER), mFlipScreen);

        mBoardView = (BoardView) findViewById(R.id.boardview);
        mBoardView.initialize(mViewListener,
                new ArrayList<Player>(),  // Disallow board manipulation by the user
                mFlipScreen, null);
        ImageButton b;
        b = (ImageButton) findViewById(R.id.replay_prev_button);
        b.setOnClickListener(new ImageButton.OnClickListener() {
            public void onClick(View v) {
                if (mNextPlay > 0) replayUpTo(mNextPlay - 1);
            }
        });
        b = (ImageButton) findViewById(R.id.replay_next_button);
        b.setOnClickListener(new ImageButton.OnClickListener() {
            public void onClick(View v) {
                if (mNextPlay < mLog.numPlays()) {
                    replayUpTo(mNextPlay + 1);
                }
            }
        });
        TextView tb = (TextView)findViewById(R.id.flip_text_button);
        tb.setOnTouchListener(new View.OnTouchListener() {
            @Override
            public boolean onTouch(View view, MotionEvent motionEvent) {
                if (motionEvent.getAction() == MotionEvent.ACTION_UP) {
                    view.performClick();
                    view.clearFocus();
                    return true;
                }
                return true;
            }
        });
        tb.clearFocus();
        mBoardView.requestFocus();

        View undo = findViewById(R.id.undo_text_button);
        if (undo != null)
            undo.setVisibility(View.GONE);

        mSeekBar = (SeekBar) findViewById(R.id.replay_seek_bar);
        mSeekBar.setMax(mLog.numPlays());
        mSeekBar.setProgress(0);
        mSeekBar.setOnSeekBarChangeListener(new SeekBar.OnSeekBarChangeListener() {
            public void onProgressChanged(SeekBar seekBar, int progress, boolean fromTouch) {
                if (fromTouch) {
                    replayUpTo(progress < mLog.numPlays() ? progress : mLog.numPlays());
                }
            }

            public void onStartTrackingTouch(SeekBar seekBar) {
            }

            public void onStopTrackingTouch(SeekBar seekBar) {
            }
        });

        mBoardView.update(
                mGameState, mBoard, mBoard,
                Player.INVALID, // Disallow board manipulation by the user
                null, false);
    }

    /**
     * Play the game up to "numMoves" moves. numMoves==0 will initialize the board, and
     * numMoves==mLog.numMoves-1 will recreate the final game state.
     */
    private final void replayUpTo(int numPlays) {
        mBoard.initialize(mLog.handicap());
        mNextPlayer = Player.BLACK;
        mPlays.clear();

        Play play = null;
        Board lastBoard = mBoard;

        // Compute the state of the game @ numMoves
        for (int i = 0; i < numPlays; ++i) {
            play = mLog.play(i);
            if (i == numPlays - 1) {
                lastBoard = new Board(mBoard);
            }
            mBoard.applyPly(mNextPlayer, play);
            mNextPlayer = mNextPlayer.opponent();
            mPlays.add(play);
        }
        mNextPlay = numPlays;
        mStatusView.update(mGameState, lastBoard, mBoard, mPlays, mNextPlayer, null);
        mBoardView.update(mGameState, lastBoard, mBoard,
                Player.INVALID,  // Disallow board manipluation by the user
                play, false);
        mSeekBar.setProgress(mNextPlay);
    }

    private static final int DIALOG_RESUME_GAME = 1;
    private static final int DIALOG_LOG_PROPERTIES = 2;
    private StartGameDialog mStartGameDialog;

    void resumeGame(boolean skipDialog) {
        Intent intent = new Intent(this, GameActivity.class);
        intent.putExtra("initial_board", mBoard);
        intent.putExtra("moves", mPlays);
        intent.putExtra("next_player", mNextPlayer);
        //intent.putExtra("replaying_saved_game", true);
        intent.putExtra("skip_dialog", skipDialog);

        Handicap h = mLog.handicap();
        if (h != Handicap.NONE) intent.putExtra("handicap", h);
        startActivity(intent);
    }

    @Override
    protected Dialog onCreateDialog(int id) {
        switch (id) {
            case DIALOG_RESUME_GAME: {
                mStartGameDialog = new StartGameDialog(
                        this, "Resume Game",
                        new DialogInterface.OnClickListener() {
                            public void onClick(DialogInterface dialog, int id) {
                                resumeGame(false);
                            }
                        }
                );
                return mStartGameDialog.getDialog();
            }
            case DIALOG_LOG_PROPERTIES:
                final GameLogPropertiesView view = new GameLogPropertiesView(this);
                view.initialize(mLog);

                return new AlertDialog.Builder(this)
                        .setTitle(R.string.game_log_properties)
                        .setView(view)
                        .setPositiveButton(android.R.string.ok, new DialogInterface.OnClickListener() {
                            public void onClick(DialogInterface dialog, int whichButton) {
                            }
                        }).create();
            default:
                return null;
        }
    }

    private final void initializeInstanceState(Bundle b) {
        SharedPreferences prefs = PreferenceManager.getDefaultSharedPreferences(
                getBaseContext());

        mFlipScreen = false;
    }

    private void flipScreen() {
        mFlipScreen = !mFlipScreen;
        mBoardView.setFlipScreen(mFlipScreen);
        mStatusView.setFlipScreen(mFlipScreen);
    }

    private final BoardView.EventListener mViewListener = new BoardView.EventListener() {
        public void onHumanPlay(Player player, Play play) {
            // Replay screen doesn't allow for human play.
        }
    };

    public void flipClick(View view) {
        flipScreen();
        mBoardView.requestFocus();
    }

    public void infoClick(View view) {
        showDialog(DIALOG_LOG_PROPERTIES);
    }

    public void play(View view) {
        showDialog(DIALOG_RESUME_GAME);
    }

    public void toSD(View view) {
        new AsyncTask<GameLog, String, String>() {
            @Override
            protected String doInBackground(GameLog... logs) {
                mGameLogList.saveLogInSdcard(mActivity, logs[0]);
                return null;
            }
        }.execute(mLog);
    }
}