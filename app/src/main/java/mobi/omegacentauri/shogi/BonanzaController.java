package mobi.omegacentauri.shogi;

import java.io.Serializable;

import android.os.Bundle;
import android.os.Handler;
import android.os.HandlerThread;
import android.os.Message;
import android.util.Log;

/**
 * 
 * An asynchronous interface for running Bonanza. 
 * 
 * Each public method in this class, except abort(), is asynchronous. 
 * It starts the request in a separate thread. The method itself returns
 * immediately. When the request completes, the result is communicated
 * via the Handler interface.
 */
public class BonanzaController {
  private static final String TAG = "BonanzaController";
  private final int mComputerDifficulty;
  private Handler mOutputHandler;  // for reporting status to the caller
  private Handler mInputHandler;   // for sending commands to the controller thread 
  private HandlerThread mThread;

  private int mInstanceId;
  
  private static final int C_START = 0;
  private static final int C_HUMAN_PLAY = 1;
  private static final int C_COMPUTER_PLAY = 2;
  private static final int C_UNDO = 3;
  private static final int C_DESTROY = 4;
  
  public BonanzaController(Handler handler, int difficulty) {
    mOutputHandler = handler;
    mComputerDifficulty = difficulty;
    mInstanceId = -1;
    mThread = new HandlerThread("BonanzaController");
    mThread.start();
    mInputHandler = new Handler(mThread.getLooper()) {
      @Override
      public void handleMessage(Message msg) {
        int command = msg.getData().getInt("command");
        switch (command) {
          case C_START:
            doStart(msg.getData().getInt("resume_instance_id"),
                (Board)msg.getData().get("initial_board"),
                (Player)msg.getData().get("next_player"));
            break;
          case C_HUMAN_PLAY:
            doHumanPlay(
                (Player)msg.getData().get("player"),
                (Play)msg.getData().get("play"));
            break;  
          case C_COMPUTER_PLAY:
            doComputerPlay((Player)msg.getData().get("player"));
            break;
          case C_UNDO:
            doUndo(
                (Player)msg.getData().get("player"),
                msg.getData().getInt("cookie1"),
                msg.getData().getInt("cookie2"));
            break;
          case C_DESTROY:
            doDestroy();
            break; 
          default:
            throw new AssertionError("Invalid command: " + command);
        }
      }
    };
  }

  public final void saveInstanceState(Bundle bundle) {
    if (mInstanceId != 0) {
      bundle.putInt("bonanza_instance_id", mInstanceId);
    }
  }

  public final void start(Bundle bundle, Board board, Player nextPlayer) {
    Bundle b = new Bundle();
    
    if (bundle != null) {
      final int instanceId = bundle.getInt("bonanza_instance_id", 0);
      b.putInt("resume_instance_id", instanceId);
    }
    b.putSerializable("initial_board", board);
    b.putSerializable("next_player", nextPlayer);
    sendInputMessage(C_START, b);
  }

  /** 
   * Stop the background thread that controls Bonanza. Must be called once before
   * abandonding this object.
   */
  public final void destroy() {
    sendInputMessage(C_DESTROY, new Bundle());
  }

  /** 
   * Tell Bonanza that the human player has made @p move. Bonanza will
   * asynchronously ack through the mOutputHandler.
   *
   * @param player is the player that has made the @p move. It is used only to
   *  report back Result.nextPlayer.
   */   
  public final void humanPlay(Player player, Play play) {
    Bundle b = new Bundle();
    b.putSerializable("player", player);
    b.putSerializable("play", play);    
    sendInputMessage(C_HUMAN_PLAY, b);
  }
  
  /** 
   * Ask Bonanza to make a move. Bonanza will asynchronously report its move
   * through the mOutputHandler.
   * 
   * @param player the identity of the computer player. It is used only to 
   * report back Result.nextPlayer.
   */
  public final void computerMove(Player player) {
    Bundle b = new Bundle();
    b.putSerializable("player", player);
    sendInputMessage(C_COMPUTER_PLAY, b);
  }

  /**
   * Undo the last move. 
   * @param player the player who made the last move.
   * @param cookie The last move made in the game.
   */
  public final void undo1(Player player, int cookie) {
    Bundle b = new Bundle();
    b.putSerializable("player", player);
    b.putSerializable("cookie1", cookie);
    sendInputMessage(C_UNDO, b);
  }
  
  /**
   * Undo the last two moves.
   * @param player the player who made the move cookie2.
   * @param cookie1 the last move made in the game.
   * @param cookie2 the penultimate move made in the game.
   */
  public final void undo2(Player player, int cookie1, int cookie2) {
    Bundle b = new Bundle();
    b.putSerializable("player", player);
    b.putSerializable("cookie1", cookie1);
    b.putSerializable("cookie2", cookie2);    
    sendInputMessage(C_UNDO, b);
  }
  
  /**
   *  The result of each asynchronous request. Packed in the "result" part of 
   *  the Message.getData() bundle.
   */
  @SuppressWarnings("serial") 
  public static class Result implements Serializable {
    // The new state of the board
    public Board board;
    
    // The following three fields describe the last move made. 
    // lastMoveCookie is an opaque token summarizing lastMove. It is passed
    // as a parameter to undo() if the caller wants to undo this move.
    //
    // Possible combinations of values are:
    //
    // 1. all the values are null or 0. This happens when the last
    //    operation resulted in an error.
    //
    // 2. lastMove!=null && lastMoveCookie > 0 && undoMoves == 0.
    //    this happens after successful completion of humanMove or 
    //    computerMove.
    //
    // 3. lastMove==null && lastMoveCookie == 0 && undoMoves > 0
    //    this happens after successful completion of undo1 or undo2. 
    public Play lastMove;       // the move made by the request.
    public int lastMoveCookie;  // cookie for lastMove. for future undos.
    public int undoMoves;       // number of moves to be rolled back
    
    // The player that should play the next turn. May be Player.INVALID when the
    // the gameState != ACTIVE.
    public Player nextPlayer; 
    
    public GameState gameState;
    public String errorMessage;
    
    @Override
    public String toString() {
      String s = "state=" + gameState.toString() + 
            " next: " + nextPlayer.toString();
      if (errorMessage != null) s += " error: " + errorMessage;
      return s;
    }
    
    static public Result fromJNI(
        BonanzaJNI.Result jr,
        Player curPlayer) {
      Result r = new Result();
      r.board = jr.board;
      r.lastMove = (jr.move != null) ? Play.fromCsaString(jr.move, curPlayer) : null;
      r.lastMoveCookie = jr.moveCookie;
      r.errorMessage = jr.error;
      
      if (jr.status >= 0) {
	r.nextPlayer = curPlayer.opponent();
        r.gameState = GameState.ACTIVE;
      } else {
        switch (jr.status) {
          case BonanzaJNI.R_ILLEGAL_MOVE:
            r.nextPlayer = curPlayer;
            r.gameState = GameState.ACTIVE;
            r.lastMove = null;
            r.lastMoveCookie = -1;
            break;
          case BonanzaJNI.R_CHECKMATE:
            r.nextPlayer = Player.INVALID;
            r.gameState = (curPlayer == Player.BLACK) ?
                GameState.BLACK_WON : GameState.WHITE_WON;
            r.errorMessage = "Checkmate";
            break;
          case BonanzaJNI.R_RESIGNED:
            r.nextPlayer = Player.INVALID;
            r.gameState = (curPlayer == Player.BLACK) ?
                GameState.WHITE_WON : GameState.BLACK_WON;
            r.errorMessage = "Resigned";
            break;
          case BonanzaJNI.R_DRAW:
            r.nextPlayer = Player.INVALID;
            r.gameState = GameState.DRAW;
            r.errorMessage = "Draw";
            break;
          default:
            throw new AssertionError("Illegal jni_status: " + jr.status);
        }
      }
      return r;
    }
  }

  //
  // Implementation details
  //

  private final void sendInputMessage(int command, Bundle bundle) {
    bundle.putInt("command", command);
    Message msg = mInputHandler.obtainMessage();
    msg.setData(bundle);
    mInputHandler.sendMessage(msg);
  }
  
  private final void sendOutputMessage(Result result) {
    Message msg = mOutputHandler.obtainMessage();
    Bundle b = new Bundle();
    b.putSerializable("result", result);
    msg.setData(b);
    mOutputHandler.sendMessage(msg);
  }

  private final void doStart(int resumeInstanceId, Board board, Player nextPlayer) {
    BonanzaJNI.Result jr = new BonanzaJNI.Result();
    if (board==null) {
      throw new AssertionError("BOARD==null");
    }
    mInstanceId = BonanzaJNI.startGame(
        resumeInstanceId, board, (nextPlayer == Player.BLACK) ? 0 : 1, mComputerDifficulty, 60, 1, jr);
    if (jr.status != BonanzaJNI.R_OK) {
      throw new AssertionError(String.format("startGame failed: %d %s", jr.status, jr.error));
    }
    Result r = new Result();
    r.board = jr.board;
    r.nextPlayer = nextPlayer;
    r.gameState = GameState.ACTIVE;
    sendOutputMessage(r);
  }

  private final void doHumanPlay(Player player, Play move) {
    BonanzaJNI.Result jr = new BonanzaJNI.Result();
    BonanzaJNI.humanMove(mInstanceId, move.toCsaString(), jr);
    if (jr.status == BonanzaJNI.R_INSTANCE_DELETED) {
      Log.d(TAG, "Instance deleted");
      mThread.quit();
      return;
    }
    sendOutputMessage(Result.fromJNI(jr, player));
  }

  private final void doComputerPlay(Player player) {
    BonanzaJNI.Result jr = new BonanzaJNI.Result();
    BonanzaJNI.computerMove(mInstanceId, jr);
    if (jr.status == BonanzaJNI.R_INSTANCE_DELETED) {
      Log.d(TAG, "Instance deleted");
      mThread.quit();
      return;
    }
    sendOutputMessage(Result.fromJNI(jr, player));
  }

  private final void doUndo(Player player, int cookie1, int cookie2) {
    BonanzaJNI.Result jr = new BonanzaJNI.Result();
    Log.d(TAG, "Undo " + cookie1 + " " + cookie2);
    BonanzaJNI.undo(mInstanceId, cookie1, cookie2, jr);
    if (jr.status == BonanzaJNI.R_INSTANCE_DELETED) {
      Log.d(TAG, "Instance deleted");
      mThread.quit();
      return;
    }
    Result r;
    if (cookie2 < 0) {
      r = Result.fromJNI(jr, player);
      r.undoMoves = 1;
    } else {
      r = Result.fromJNI(jr, player.opponent());
      r.undoMoves = 2;
    }
    sendOutputMessage(r);
  }   
  
  private final void doDestroy() {
    Log.d(TAG, "Destroy");
    mThread.quit();
  }
}
