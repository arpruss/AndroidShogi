package mobi.omegacentauri.shogi;
import mobi.omegacentauri.shogi.Board;

// JNI interface for Bonanza
public class BonanzaJNI {
  static {  
    System.loadLibrary("bonanza-jni");  
  }

  // TODO(saito) don't write to games.csa
  
  //
  // Return values of HumanMove and ComputerMove
  //
  public static final int R_OK = 0;
  // Human made an illegal move.
  public static final int R_ILLEGAL_MOVE = -1;
  // Checkmate. Computer won
  public static final int R_CHECKMATE = -2;
  // Computer lost
  public static final int R_RESIGNED = -3;
  // Sennichite
  public static final int R_DRAW = -4;
  
  // Another instance of the game started
  public static final int R_INSTANCE_DELETED = -5;
  
  // Initialization error (e.g., required DB files not found).
  public static final int R_INITIALIZATION_ERROR = -6;
  
  static public final class Result {
    public Result() { board = new Board(); }
    
    // One of R_XXX constants
    public int status;
    
    // Error message, if any.
    public String error;
    
    // The new state of the board. 
    public final Board board;
    
    // The description of the move in CSA format.
    public String move;
    
    // A cookie used to undo the move in the future. In practice, the value
    // in the result of calling interpret_CSA_move(csaMove).
    public int moveCookie;
  }
  
  /**
   * Initialize the C module. This should be called once on process startup.
   * Repeated calls are idempotent. Any error in this method will be reported
   * via subsequent startGame calls.
   * 
   * @param externalStorageDir The SD card directory that stores the Bonanza
   * fv.bin, hash.bin, book.bin files.
   */
  static public native void initialize(
      String externalStorageDir);
  
  /** 
   * Start or resume a game.
   * 
   * @param resumeInstanceId if != 0, resume the game specified by this value.
   * If this game is not active any more, start a new game.
   * 
   * @param initialBoard Initial state of the board.
   *
   * @param next_turn If 0, Black plays the first turn. If 1, the WHITE plays the first turn
   *   in the game. This param is non-zero only when resuming a saved game midway.
   *
   * @param difficulty 0==weak, 4==strong
   * @param result (output)  filled with the initial board configuration.
   * @return The game's instance ID. When the game has resumed 
   * resumeInstanceId, the method return its value. Otherwise it returns a newly
   * allocated integer that's different from any prior ID (the scope is this process).
   */
  static public native int startGame(
      int resumeInstanceId,
      Board initialBoard,
      int next_turn,
      int difficulty,
      int total_think_time_secs,    
      int per_turn_think_time_secs,
      Result result);
  
  /**
   *  Inform Bonanza that the human player made a move.
   *   
   * @param move CSA-format string, such as "7776FU".
   * @param result (output)
   */
  static public native void humanMove(
      int instanceId,
      String move,
      Result result);

  /**
   * Have the computer compute the next move. 

   * @param result (output) store the move made by the computer
   */
  static public native void computerMove(
      int instanceId,
      Result result);
  
  /**
   * Undo up to two past moves. 
   * 
   * REQUIRES: cookie1 and cookie2 are values of MoveResult.cookie of 
   * past moves. cookie1 must be the last move made in the game (either computer or 
   * human). If cookie2 > 0, it must be the penultimate move in the game.
   *
   * @param result (output)
   */
  static public native void undo(
      int instanceId,
      int cookie1, int cookie2,
      Result result);
  
  // Abort the current game. This method can be called from any thread.
  // If another thread is running HumanMove or ComputerMove, it will see
  // an error. The state of the game will be undefined after this call.
  static public native void abort();
}

