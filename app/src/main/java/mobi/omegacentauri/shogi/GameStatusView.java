package mobi.omegacentauri.shogi;

import android.content.Context;
import android.text.SpannableString;
import android.text.style.UnderlineSpan;
import android.util.AttributeSet;
import android.widget.LinearLayout;
import android.widget.TextView;
import android.widget.Toast;

import java.util.ArrayList;
import java.util.Locale;

/**
 * Widget for displaying game status, such as elapsed time per player and 
 * last moves.
 */
public class GameStatusView extends LinearLayout {
    private String mBaseBlackPlayerName;
    private String mBaseWhitePlayerName;
  private Player mCurrentPlayer;

  class Timer {
    public Timer(TextView v) { mView = v; mLastThinkTimeSeconds = -1; }
    public void update(long thinkTimeMs) {
      long t = thinkTimeMs / 1000;  // convent millisecs -> seconds
      if (mLastThinkTimeSeconds != t) {
        mLastThinkTimeSeconds = t;
        long seconds = t % 60;
        long minutes = t / 60;
        mView.setText(String.format("%3d:%02d  ", minutes, seconds));
      }
    }
    public TextView getView() { return mView; }
    
    private final TextView mView;
    private long mLastThinkTimeSeconds;
  }

  private TextView mGameStatus;
  private TextView mPlayHistory;
  private Timer mBlackTime;
  private TextView mBlackStatus;
  private Timer mWhiteTime;
  private TextView mWhiteStatus;
  private String mBlackPlayerName;
  private String mWhitePlayerName;
  private boolean mFlipScreen;
  
  // List of past moves, in display format.
  private ArrayList<String> mPlayList;
  
  public GameStatusView(Context context, AttributeSet attrs) {
    super(context, attrs);
  }
  public GameStatusView(Context context) {
    super(context);
  }
  
  public final void initialize(
          String blackPlayerName,
          String whitePlayerName, boolean flipScreen) {
    mGameStatus = (TextView)findViewById(R.id.status_game_status);
    mPlayHistory = (TextView)findViewById(R.id.status_play_history);
    mPlayHistory.setHorizontallyScrolling(true);
    mBlackTime = new Timer((TextView)findViewById(R.id.status_black_time));
    mBlackStatus = (TextView)findViewById(R.id.status_black_player_name);
    mWhiteTime = new Timer((TextView)findViewById(R.id.status_white_time));
    mWhiteStatus = (TextView)findViewById(R.id.status_white_player_name);
    mPlayList = new ArrayList<String>();
    mFlipScreen = flipScreen;
    mBaseBlackPlayerName = blackPlayerName;
    mBaseWhitePlayerName = whitePlayerName;

    setPlayerNames();
    mBlackStatus.setText(mBlackPlayerName);
    mWhiteStatus.setText(mWhitePlayerName);    
  }

    private void setPlayerNames() {
        mBlackPlayerName = (mFlipScreen ? "▼" : "▲") + mBaseBlackPlayerName;
        mWhitePlayerName = (mFlipScreen ? "△" : "▽") + mBaseWhitePlayerName;
    }

    public void setFlipScreen(boolean flipScreen) {
      mFlipScreen = flipScreen;
      setPlayerNames();
      showPlayerNames();
    }

    /**
   * Update the state of the game and redraw widgets.
   * 
   * @param gameState
   * @param lastBoard State of the board before the last move
   * @param board The uptodate state of the board
   * @param plays The list of moves leading up to "board"
   * @param currentPlayer The player to hold the next turn. 
   * @param errorMessage
   */
  public final void update(
      GameState gameState,
      Board lastBoard,
      Board board,
      ArrayList<Play> plays,
      Player currentPlayer,
      String errorMessage) {

    mCurrentPlayer = currentPlayer;
    showPlayerNames();

    while (plays.size() > mPlayList.size()) {
      // Generally, moves is just one larger than mMoveList, in which case
      // we can use "lastBoard" to compute the display string of the last move.
      // If moves.size() > mMovesList.size() + 1, then moves other than the last
      // may be inaccurately displayed since "lastBoard" may not correspond to the
      // state before these plays are made.
      Play thisPlay = plays.get(mPlayList.size());
      Play prevPlay = (mPlayList.size() > 0 ? plays.get(mPlayList.size() - 1) : null);
      mPlayList.add(traditionalPlayNotation(lastBoard, thisPlay, prevPlay));
    }
    
    // Handle undos
    while (plays.size() < mPlayList.size()) {
      mPlayList.remove(mPlayList.size() - 1);
    }
    
    if (mPlayList.size() > 0) {
      // Display the last six plies. The TextView is right-justified, so if the view isn't wide enough, earlier plays
      // will be shown truncated.
      int n = Math.min(mPlayList.size(), 6);
      StringBuilder b = new StringBuilder();
      boolean first = true;
      for (int i = mPlayList.size() - n; i < mPlayList.size(); ++i) {
        if (!first) b.append(", ");
        b.append(i + 1).append(":");
        b.append((i % 2 == 0) ? "◆" : "◇");
        b.append(mPlayList.get(i));
        first = false;
      }
      mPlayHistory.setText(b.toString());
    }
    String endGameMessage = null;
    if (gameState == GameState.ACTIVE) {
    } else if (gameState == GameState.WHITE_WON) {
      endGameMessage = getResources().getString(R.string.white_won); 
    } else if (gameState == GameState.BLACK_WON) {
      endGameMessage = getResources().getString(R.string.black_won);       
    } else if (gameState == GameState.DRAW) {
      endGameMessage = getResources().getString(R.string.draw);             
    } else {
      throw new AssertionError(gameState.toString());
    }
    if (endGameMessage != null) {
      Toast.makeText(getContext(), endGameMessage, Toast.LENGTH_LONG).show();
      mGameStatus.setText(endGameMessage);
    }
  }

  private void showPlayerNames() {
    if (mCurrentPlayer == Player.WHITE) {
      mBlackStatus.setText(mBlackPlayerName);
      SpannableString s = new SpannableString(mWhitePlayerName);
      s.setSpan(new UnderlineSpan(), 0, s.length(), 0);
      mWhiteStatus.setText(s);
    } else {
      SpannableString s = new SpannableString(mBlackPlayerName);
      s.setSpan(new UnderlineSpan(), 0, s.length(), 0);
      mBlackStatus.setText(s);
      mWhiteStatus.setText(mWhitePlayerName);
    }
  }

  public final void updateThinkTimes(long black, long white) {
    mBlackTime.update(black);
    mWhiteTime.update(white);
  }
  
  private final String traditionalPlayNotation(Board board, Play thisMove, Play prevMove) {
    if (Locale.getDefault().getLanguage().equals("ja")) {
      return thisMove.toTraditionalNotation(board, prevMove).toJapaneseString();
    } else {
      return thisMove.toCsaString();
    }
  }
}
