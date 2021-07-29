// Copyright 2010 Google Inc. All Rights Reserved.

package mobi.omegacentauri.shogi;

import java.io.Serializable;
import java.util.ArrayList;
import java.util.Map;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

/**
 * A class representing a single play
 */
@SuppressWarnings("serial")
public class Play implements Serializable {
  // Japanese move display support
  public static final String japaneseNumbers[] = {
    null, "一", "二", "三", "四", "五", "六", "七", "八", "九",    
  };
  public static final String japaneseRomanNumbers[] = {
    null, "１", "２", "３", "４", "５", "６", "７", "８", "９",    
  };
  
  // The piece to move. The value is positive if player==BLACK, negative if player==WHITE.
  // The absolute value of mPiece is one of the constants define in Piece (e.g., Piece.FU).
  private final int mPiece;

  // The source and destination coordinates. When moving a piece on the board, each value is in range
  // [0, Board.DIM). When dropping a captured piece on the board, fromX = fromY = -1.
  private final int mFromX, mFromY, mToX, mToY;

  private long mStartTimeInMs; // player's start time
  private long mEndTimeInMs;  // player's end time

  public Play(int p, int fx, int fy, int tx, int ty) {
    mPiece = p;
    mFromX = fx;
    mFromY = fy;
    mToX = tx;
    mToY = ty;
    mStartTimeInMs = -1;
    mEndTimeInMs = -1;
  }

  public final boolean isDroppingPiece() { return mFromX < 0; }
  public final int piece() { return mPiece; }
  public final int fromX() { return mFromX; }
  public final int fromY() { return mFromY; }  
  public final int toX() { return mToX; }
  public final int toY() { return mToY; }
  public final long startTime() { return mStartTimeInMs; }
  public final long endTime() { return mEndTimeInMs; }
  public final long playTime() { return mEndTimeInMs-mStartTimeInMs; }

  public final void setTime(long startTimeInMs, long endTimeInMs) {
    mStartTimeInMs = startTimeInMs;
    mEndTimeInMs = endTimeInMs;
  }

  public Player player() {
    return mPiece < 0 ? Player.WHITE : Player.BLACK;
  }

  @Override public boolean equals(Object o) {
    if (o instanceof Play) {
      Play m = (Play)o;
      return (
          m.mPiece == mPiece &&
          m.mFromX == mFromX && m.mFromY == mFromY &&
          m.mToX == mToX && m.mToY == mToY);
    } else {
      return false;
    }
  }
  
  @Override public int hashCode() {
    return mPiece + mFromX + mFromY + mToX;
  }
  
  @Override public String toString() {
    if (mStartTimeInMs >= 0) {
      int start = (int)((mStartTimeInMs + 500l) / 1000l);
      int startSec = start % 60;
      int startMin = start / 60;
      int play = (int)((playTime() + 500l) / 1000l);
      int playSec = play % 60;
      play /= 60;
      int playMin = play % 60;
      play /= 60;
      int playHr = play;
      return String.format("%d%d%d%d:%d (%2d:%02d/%2d:%02d:%02d)", mFromX, mFromY, mToX, mToY, mPiece, startMin, startSec, playHr, playMin, playSec);
    }
    else {
      return String.format("%d%d%d%d:%d", mFromX, mFromY, mToX, mToY, mPiece);
    }
  }
  
  // Return the CSA-format string for this move. 
  public final String toCsaString() {
    // This program uses defines the upper left corner of the board to be <0,0>, whereas CSA defines
    // the upper right corner to be <1, 1>. 
    // Translate the coordinate accordingly.
    int cFromX = 0;
    int cFromY = 0;
    if (mFromX >= 0) {
      // Moving a piece on board
      cFromX = 9 - mFromX;
      cFromY = mFromY + 1;
    } else {
      // Dropping a captured piece
    }
    int cToX = 9 - mToX;
    int cToY = mToY + 1;
    int p = mPiece >= 0 ? mPiece : -mPiece;
    return String.format("%d%d%d%d%s", 
        cFromX, cFromY, cToX, cToY,
        Piece.csaNames[p]);
  }
  
  public static Play fromCsaString(String csa, Player player) {
    int tmp = csa.charAt(0) - '0';
    int fromX, fromY;
    if (tmp > 0) {
      // Moving a piece on board
      fromX = 9 - tmp;
      fromY = csa.charAt(1) - '0' - 1;
    } else {
      // Dropping a captured piece
      fromX = fromY = -1;
    }
    int toX = 9 - (csa.charAt(2) - '0');
    int toY = csa.charAt(3) - '0' - 1;
    
    int piece = Piece.fromCsaName(csa.substring(4));
    if (player == Player.WHITE) piece = -piece;
    
    return new Play(
        piece, fromX, fromY, toX, toY);
  }
  
  
  //private static final Pattern KIF_MOVE_PATTERN = Pattern.compile("([1-9１-９])([一二三四五六七八九])(.+)\\((.)(.)\\)");
  private static final Pattern KIF_MOVE_PATTERN = Pattern.compile("([1-9１-９])(.)(.+)\\((.)(.)\\)\\s*");
  private static final Pattern KIF_MOVE2_PATTERN = Pattern.compile("同[　\\s]*(.+)\\((.)(.)\\)\\s*");
  private static final Pattern KIF_DROP_PATTERN = Pattern.compile("([1-9１-９])([一二三四五六七八九])(.*)");
  private static final Pattern KIF_TIME_PATTERN = Pattern.compile("\\s+\\(\\s*([0-9]+):([0-9]+)\\/\\s*([0-9]+):([0-9]+):([0-9]+)\\)");
  
  // Parse a KIF-format string. It looks like
  // "８四歩(83)" (move FU at 83 to 84). Returns null if the play is a noop.
  public static final Play fromKifString(Play prevMove, Player player, String kifMove) throws ParseException {
    Matcher m = KIF_MOVE_PATTERN.matcher(kifMove);
    try {
      m = KIF_TIME_PATTERN.matcher(kifMove);
      if (m.matches()) {
        try {

        }
        catch(NumberFormatException e) {
        }
      }
      Play p = null;
      if (m.matches()) {
        p = new Play(japaneseToPiece(player, m.group(3)),
            arabicToXCoord(m.group(4)), arabicToYCoord(m.group(5)),
            arabicToXCoord(m.group(1)), japaneseToYCoord(m.group(2)));
      }
      else if (prevMove != null) {
        m = KIF_MOVE2_PATTERN.matcher(kifMove);
        if (m.matches()) {
          p = new Play(japaneseToPiece(player, m.group(1)),
              arabicToXCoord(m.group(2)), arabicToYCoord(m.group(3)),
              prevMove.mToX, prevMove.mToY);
        }
      }
      if (p == null) {
        m = KIF_DROP_PATTERN.matcher(kifMove);
        if (m.matches()) {
          p = new Play(japaneseToPiece(player, m.group(3)),
                  -1, -1, arabicToXCoord(m.group(1)), japaneseToYCoord(m.group(2)));
        }
      }

      if (p != null) {
        m = KIF_TIME_PATTERN.matcher(kifMove);
        if (m.matches()) {
          long delta = (Long.parseLong(m.group(1)) * 60 + Long.parseLong(m.group(2))) * 1000l;
          long start = ((Long.parseLong(m.group(3)) * 60 + Long.parseLong(m.group(4))) * 60 + Long.parseLong(m.group(5))) * 1000l;
          p.setTime(start, start+delta);
        }
        return p;
      }

      if (kifMove.startsWith("千日手")) {
        // Ignore
        // TODO(saito) Display the game outcome
        return null;
      }
      throw new ParseException("Illegal play string: \"" + kifMove + "\"");
    } catch (NumberFormatException e) {
      throw new ParseException("Failed to parse " + kifMove + ": " + e.getMessage()); 
    }
  }

  private static int arabicToXCoord(String s) throws NumberFormatException {
    char ch = s.charAt(0);
    final int n = (ch >= '０' ? ch - '０' : ch - '0');
    return 9 - n;
  }

  private static int arabicToYCoord(String s) throws NumberFormatException {
    char ch = s.charAt(0);
    final int n = (ch >= '０' ? ch - '０' : ch - '0');
    return n - 1;
  }

  private static int japaneseToYCoord(String s) throws NumberFormatException {
    for (int i = 0; i < japaneseNumbers.length; ++i) {
      if (s.equals(japaneseNumbers[i])) return i - 1;
    }
    throw new NumberFormatException(s + ": is not a japanese numeric string");
  }

  private static int japaneseToPiece(Player player, String s) throws NumberFormatException {
    final boolean promoted = (s.indexOf("成") >= 0);
    for (Map.Entry<String, String> e: Piece.alternateJapaneseNames.entrySet()) {
      if (e.getKey().equals(s)) {
        s = e.getValue();
        break;
      }
    }
    for (int i = 0; i < Piece.japaneseNames.length; ++i) {
      final String pieceName = Piece.japaneseNames[i];
      if (pieceName != null && s.indexOf(pieceName) >= 0) {
        int piece = i;
        if (promoted) {
          if (Board.isPromoted(piece)) {
            throw new NumberFormatException(s + ": already promoted");
          }
          piece = Board.promote(piece);
        }
        return (player == Player.BLACK) ? piece : -piece;
      }
    }
    throw new NumberFormatException(s + ": Failed to parse as a japanese Shogi piece name");
  }

  // Modifier bits. Used by TraditionalNotation.modifier.
  public static final int DROP = (1 << 0);    // dropping a captured piece
  public static final int PROMOTE = (1 << 1); // newly promoting a piece
  
  // Move direction bits
  public static final int FORWARD = (1 << 2);        
  public static final int BACKWARD = (1 << 3);
  public static final int SIDEWAYS = (1 << 4);

  // Location of this piece relative to other pieces that can move to
  // the same <toX, toY>.
  public static final int LEFT = (1 << 5);
  public static final int RIGHT = (1 << 6);    
  public static final int CENTER = (1 << 7);
  
  // This move has captured the piece involved in the last move.
  // In Japanese, "同".
  public static final int CAPTURED_PREVIOUS_PIECE = (1 << 8);
  
  public static class TraditionalNotation {
    public TraditionalNotation(int p, int xx, int yy, int m) {
      piece = p;
      x = xx;
      y = yy;
      modifier = m;
    }
    
    @Override public String toString() {
      return String.format("%d <%d,%d>/%x", piece, x, y, modifier);
    }

    public String toJapaneseString() {
      if ((modifier & Play.CAPTURED_PREVIOUS_PIECE) != 0) {
        return String.format("同%s%s",
            Piece.japaneseNames[Board.type(piece)],
            modifiersToJapanese(modifier));
      } else {
        return String.format("%s%s%s%s",
            Play.japaneseRomanNumbers[x], 
            Play.japaneseNumbers[y], 
            Piece.japaneseNames[Board.type(piece)],
            modifiersToJapanese(modifier));
      }
    }
  
    private static final String modifiersToJapanese(int modifier) {
      String s = "";
      if ((modifier & Play.DROP) != 0) s += "打";
      if ((modifier & Play.PROMOTE) != 0) s += "成";    
      if ((modifier & Play.FORWARD) != 0) s += "上";        
      if ((modifier & Play.BACKWARD) != 0) s += "引";            
      if ((modifier & Play.SIDEWAYS) != 0) s += "寄";          
      if ((modifier & Play.RIGHT) != 0) s += "右";                    
      if ((modifier & Play.LEFT) != 0) s += "左";                        
      if ((modifier & Play.CENTER) != 0) s += "直";
      return s;
    }
    
    // Piece type (see Piece.XXX constants). If modifier & PROMOTE, "piece" stores the piece type before promotion.
    // The value is negative for Player.WHITE. 
    public final int piece;
    public final int x, y;  // destination square. Upper left corner is (x=9, y=1).
    public int modifier;    // bitstring, see above.
  }
  
  public final TraditionalNotation toTraditionalNotation(Board board, Play prevPlay) {
    int modifier = 0;

    if (prevPlay != null && prevPlay.toX() == mToX && prevPlay.toY() == mToY) {
      modifier |= CAPTURED_PREVIOUS_PIECE;
    }
    
    int pieceBeforeMove = mPiece;
    if (isNewlyPromoted(board)) {
      modifier |= PROMOTE;
      pieceBeforeMove = Board.unpromote(mPiece);
    }
    ArrayList<Board.Position> others = listOtherMoveSources(board);
    if (others.isEmpty()) {
      ;
    } else if (isDroppingCapturedPiece()) {
      modifier |= DROP;
    } else {
      int myMoveDir = moveDirection(board, mFromX, mFromY, mToX, mToY);
      modifier |= myMoveDir;
      
      boolean hasPieceWithSameMoveDir = false;
      for (Board.Position p: others) {
        int dir = moveDirection(board, p.x, p.y, mToX, mToY);
        if (dir == myMoveDir) {
          hasPieceWithSameMoveDir = true;
        } 
      }
      
      if (!hasPieceWithSameMoveDir) {
        // There's no piece that moves in the same direction to reach <tox,toy>.
        // Thus, the modifier can just specify the move direction of my piece to
        // disambiguate it from other legit moves.
      } else {
        // There are pieces that move in the same direction.
        // Determine the location of this piece relative to them.
        //
        // There are only three possibilites --- the piece is to the right,
        // to the left, or in the center of other pieces.
        int relPos = 0;
        for (Board.Position p: others) {
          if (moveDirection(board, p.x, p.y, mToX, mToY) == myMoveDir) {
            relPos |= relativePosition(mFromX, mFromY, p.x, p.y);
          }
        }
        if (relPos == (LEFT | RIGHT)) relPos = CENTER;
        modifier |= relPos;
      }
    }
    return new TraditionalNotation(pieceBeforeMove, 9 - mToX, mToY + 1, modifier);
  }
  
  private final boolean isDroppingCapturedPiece() { return mFromX < 0; }
  
  private final boolean isNewlyPromoted(Board board) {
    if (isDroppingCapturedPiece()) return false; 
    boolean fromPromoted = Board.isPromoted(board.getPiece(mFromX, mFromY)); 
    boolean toPromoted = Board.isPromoted(mPiece);
    return toPromoted && !fromPromoted;
  }
  
  private static final int moveDirection(Board b, int fx, int fy, int tx, int ty) {
    if (fx < 0) return DROP;
    if (fy == ty) return SIDEWAYS;
    final int piece = b.getPiece(fx, fy);
    if (Board.player(piece) == Player.BLACK) {
      return (ty < fy) ? FORWARD : BACKWARD; 
    } else {
      return (ty < fy) ? BACKWARD : FORWARD;
    }
  }

  private final int relativePosition(int x1, int y1, int x2, int y2) {
    if (Board.player(mPiece) == Player.BLACK) {
      return(x1 < x2) ? LEFT : RIGHT; 
    } else {
      return(x1 < x2) ? RIGHT : LEFT;
    }
  }
  
  // Find pieces other than the one at <fromX, fromX> 
  // that can move or can be dropped at<toX, toY>. 
  // For captured pieces, Position.x and Position.y
  // are both -1. 
  private final ArrayList<Board.Position> listOtherMoveSources(Board board) {
    ArrayList<Board.Position> list = new ArrayList<Board.Position>();
    for (int x = 0; x < Board.DIM; ++x) {
      for (int y = 0; y < Board.DIM; ++y) {
        if (x == mFromX && y == mFromY) continue;  // exclude this piece.
        int otherPiece = board.getPiece(x, y);

        // We need disambiguation only when there are two pieces of the same type that
        // can move to the same spot. 
        if (maybeUnpromote(otherPiece) != maybeUnpromote(mPiece)) continue;

        // If otherPiece can move to <fromX,  fromY>, then we need disambiguation
        for (Board.Position p : board.possibleMoveDestinations(x, y)) {
          if (p.x == mToX && p.y == mToY) {
            list.add(new Board.Position(x, y));
            break;
          }
        }
      }
    }
    
    if (board.getPiece(mToX, mToY) == 0 && !isDroppingCapturedPiece()) {
      // The destination is empty now, so we need to check if
      // there's a captured piece that can be dropped to <tox,toy>.
      Player me = Board.player(mPiece);
      for (Board.CapturedPiece cp: board.getCapturedPieces(me)) {
        boolean dropAllowed = true;
        if (Board.type(cp.piece) == Piece.FU) {
          // Don't allow double pawns
          for (int y = 0; y < Board.DIM; ++y) {
            int piece = board.getPiece(mToX, y);
            if (Board.player(piece) == me &&
                Board.type(piece) == Piece.FU) {
              dropAllowed = false;
              break;
            }
          }
        }
        if (dropAllowed && cp.piece == mPiece) {
          list.add(new Board.Position(-1, -1));
          break;
        }
      }
    }
    return list;
  }
  
  static final int maybeUnpromote(int piece) {
    if (Board.isPromoted(piece)) return Board.unpromote(piece);
    return piece;
  }
}
