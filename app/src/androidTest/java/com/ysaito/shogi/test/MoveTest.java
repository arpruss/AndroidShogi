/**
 * @author saito
 *
 */

package com.ysaito.shogi.test;

import java.util.ArrayList;

import com.ysaito.shogi.Board;
import com.ysaito.shogi.Play;
import com.ysaito.shogi.ParseException;
import com.ysaito.shogi.Piece;
import com.ysaito.shogi.Player;

import android.test.AndroidTestCase;
import android.util.Log;

public class MoveTest extends AndroidTestCase {
  private static final String TAG = "MoveTest";
  
  private Player mPlayer;
  
  @Override public void setUp() {
  }
  
  public void testToCsaString() {
    Play move = new Play(Piece.FU, 1, 1, 1, 2);
    assertEquals(move.toCsaString(), "8283FU");
  }

  public void testTraditionalNotation() {
    Play m = newMove(Piece.FU, 7, 7, 7, 6); 
    assertEquals(toTraditionalNotation(m, newBoard(), null), "76FU");
    
    m = newMove(-Piece.FU, 7, 6, 7, 7); 
    assertEquals(toTraditionalNotation(m, newBoard(), null), "77FU");

    Board b = newBoard(7, 4, Piece.FU);
    m = newMove(Piece.TO, 7, 4, 7, 3); 
    assertEquals(toTraditionalNotation(m, b, null), "73FU/PROMOTE");
    
    b = newBoard();
    m = newMove(Piece.FU, -1, -1, 7, 3); 
    assertEquals(toTraditionalNotation(m, newBoard(), null), "73FU");
  }
  
  public void testTraditionalNotation_BlackPlayer() {
    mPlayer = Player.BLACK;
    Board b = newBoard(2, 5, Piece.KIN, -1, -1, Piece.KIN);
    Play m = newMove(Piece.KIN, 2, 5, 2, 4);
    assertEquals(toTraditionalNotation(m, b, null), "24KI/FORWARD");
  }
  
  public void testTraditionalNotation_WhitePlayer() {
    mPlayer = Player.WHITE;
    Board b = newBoard(8, 5, -Piece.KIN, -1, -1, -Piece.KIN);
    Play m = newMove(-Piece.KIN, 8, 5, 8, 6);
    Log.d(TAG, "KINSTART");
    assertEquals(toTraditionalNotation(m, b, null), "86KI/FORWARD");
  }
  
  public void testTraditionalNotation_Fu_BlackPlayer() {
    mPlayer = Player.BLACK;
    Board b = newBoard(2, 5, Piece.FU, -1, -1, Piece.FU);
    Play m = newMove(Piece.FU, 2, 5, 2, 4);
    assertEquals(toTraditionalNotation(m, b, null), "24FU");
  }

  public void testTraditionalNotation_Fu_WhitePlayer() {
    mPlayer = Player.WHITE;
    Board b = newBoard(8, 5, -Piece.FU, -1, -1, -Piece.FU);
    Play m = newMove(-Piece.FU, 8, 5, 8, 6);
    assertEquals(toTraditionalNotation(m, b, null), "86FU");
  }
  
  public void testTraditionalNotation2() {
    mPlayer = Player.BLACK;
    Board b = newBoard(7,9, Piece.KYO);
    Play m = newMove(Piece.KYO, -1, -1, 7, 8);
    assertEquals(toTraditionalNotation(m, b, null), "78KY/DROP");
    
    b = newBoard(-1, -1, Piece.KYO, 7, 9, Piece.KYO);
    m = newMove(Piece.KYO, 7, 9, 7, 8);
    Log.d(TAG, "BLAHBLAH: " + m.toCsaString());
    assertEquals(toTraditionalNotation(m, b, null), "78KY/FORWARD");
    
    m = newMove(Piece.NARI_KYO, 7, 9, 7, 1);
    assertEquals(toTraditionalNotation(m, b, null), "71KY/PROMOTE");
  }
  
  public void testTraditionalNotation4() {
    mPlayer = Player.BLACK;
    //         KIN
    //     KIN 
    Board b = newBoard(1, 2, -Piece.KIN, 2, 3, -Piece.KIN, -1, -1, -Piece.KIN);
    
    Play m = newMove(-Piece.KIN, -1, -1, 2, 2);
    assertEquals(toTraditionalNotation(m, b, null), "22KI/DROP");
  }
  
  public void testTraditionalNotation5() {
    Board b = newBoard(1, 1, Piece.KIN);
    Play prev = newMove(-Piece.KIN, 1, 0, 1, 1);
    Play cur = newMove(Piece.GIN, 2, 2, 1, 1);
    assertEquals(toTraditionalNotation(cur, b, prev), "11GI/CAPTURED");
  }
  
  
  public void testTraditionalNotation3() {
    mPlayer = Player.BLACK;
    // KIN KIN KIN
    // KIN     KIN
    // KIN KIN KIN 
    Board b = newBoard(
        1, 1, Piece.KIN, 2, 1, Piece.KIN, 3, 1, Piece.KIN,                
        1, 2, Piece.KIN, 3, 2, Piece.KIN,
        1, 3, Piece.KIN, 2, 3, Piece.KIN, 3, 3, Piece.KIN);
        
    Play m = newMove(Piece.KIN, -1, -1, 2, 2);
    assertEquals(toTraditionalNotation(m, b, null), "22KI/DROP");
    
    m = newMove(Piece.KIN, 2, 1, 2, 2);
    assertEquals(toTraditionalNotation(m, b, null), "22KI/BACKWARD");
    
    m = newMove(Piece.KIN, 1, 2, 2, 2);
    assertEquals(toTraditionalNotation(m, b, null), "22KI/RIGHT/SIDE");

    m = newMove(Piece.KIN, 3, 2, 2, 2);
    assertEquals(toTraditionalNotation(m, b, null), "22KI/LEFT/SIDE");

    m = newMove(Piece.KIN, 1, 3, 2, 2);
    assertEquals(toTraditionalNotation(m, b, null), "22KI/RIGHT/FORWARD");
    
    m = newMove(Piece.KIN, 2, 3, 2, 2);
    assertEquals(toTraditionalNotation(m, b, null), "22KI/CENTER/FORWARD");

    m = newMove(Piece.KIN, 3, 3, 2, 2);
    assertEquals(toTraditionalNotation(m, b, null), "22KI/LEFT/FORWARD");
  }

  public void testKif1() throws ParseException {
    assertEquals(parseKifString(null, Player.BLACK, "8四歩(83)"), "8384FU");
    assertEquals(parseKifString(null, Player.BLACK, "８四歩(83)"), "8384FU");    
    assertEquals(parseKifString(null, Player.BLACK, "8四歩"), "0084FU");
    assertEquals(parseKifString(null, Player.BLACK, "8四歩打"), "0084FU");
    assertEquals(parseKifString(null, Player.BLACK, "8一香成(89)"), "8981NY");
    assertEquals(parseKifString(null, Player.BLACK, "8一金上直(82)"), "8281KI");    
  }
  
  public void testKif2() throws ParseException {
    Play prev = newMove(-Piece.FU, -1, -1, 8, 3);
    assertEquals(parseKifString(prev, Player.BLACK, "同香(89)"), "8983KY");    
    assertEquals(parseKifString(prev, Player.BLACK, "同 香(89)"), "8983KY");    
    assertEquals(parseKifString(prev, Player.BLACK, "同 香成(89)"), "8983NY");    
  }
  

  private static String parseKifString(Play prevMove, Player p, String s) throws ParseException {
    return Play.fromKifString(prevMove, p, s).toCsaString();
  }
  
  private String toTraditionalNotation(Play m, Board b, Play prevMove) {  
    Play.TraditionalNotation n = m.toTraditionalNotation(b, prevMove);
    String s = String.format("%d%d%s", n.x, n.y, Piece.csaNames[Board.type(n.piece)]);
    if ((n.modifier & Play.PROMOTE) != 0) s += "/PROMOTE";
    if ((n.modifier & Play.DROP) != 0) s += "/DROP";
    if ((n.modifier & Play.RIGHT) != 0) s += "/RIGHT";
    if ((n.modifier & Play.LEFT) != 0) s += "/LEFT";    
    if ((n.modifier & Play.CENTER) != 0) s += "/CENTER";        
    if ((n.modifier & Play.FORWARD) != 0) s += "/FORWARD";
    if ((n.modifier & Play.BACKWARD) != 0) s += "/BACKWARD";
    if ((n.modifier & Play.SIDEWAYS) != 0) s += "/SIDE";
    if ((n.modifier & Play.CAPTURED_PREVIOUS_PIECE) != 0) s += "/CAPTURED";
    return s;
  }
  
  // The arg is a variadic list of <x, y, piece>.
  // <x, y> are in traditional coordinate to simplify testing.
  // That is, the upper-right corner is <1,1>. 
  private Board newBoard(int... values) { 
    Board b = new Board();
    assertEquals(values.length % 3, 0);
    ArrayList<Board.CapturedPiece> captured = new ArrayList<Board.CapturedPiece>();
    for (int i = 0; i < values.length; i += 3) {
      int x = values[i];
      int y = values[i + 1];
      int piece = values[i + 2];
      if (x >= 0) {
        b.setPiece(9 - x, y - 1, piece);
      } else {
        captured.add(new Board.CapturedPiece(piece, 1));
      }
    }
    if (captured.size() > 0) {
      b.setCapturedPieces(mPlayer, captured);
    }
    return b;
  }
  
  // <fx, fy> and <tx, ty> are in traditional coordinate to simplify testing.
  // That is, the upper-right corner is <1,1>. 
  private Play newMove(int piece, int fx, int fy, int tx, int ty) {
    if (fx < 0) {  // dropping caputured piece
      return new Play(piece, -1, -1, 9 - tx, ty - 1);
    } else {
      return new Play(piece, 9 - fx, fy - 1, 9 - tx, ty - 1);
    }
  }
}