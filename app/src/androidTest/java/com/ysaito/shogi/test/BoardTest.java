package com.ysaito.shogi.test;

import com.ysaito.shogi.Board;
import com.ysaito.shogi.Piece;

import android.test.AndroidTestCase;

public class BoardTest extends AndroidTestCase {
  void testPromote() {
    assertEquals(Board.promote(Piece.FU), Piece.TO);
    assertEquals(Board.promote(-Piece.FU), -Piece.TO);
    assertEquals(Board.unpromote(Piece.TO), Piece.FU);
    assertEquals(Board.unpromote(-Piece.TO), -Piece.FU);
    
    assertEquals(Board.promote(Piece.HI), Piece.RYU);
    assertEquals(Board.promote(-Piece.HI), -Piece.RYU);
    assertEquals(Board.unpromote(Piece.RYU), Piece.HI);
    assertEquals(Board.unpromote(-Piece.RYU), -Piece.HI);    
  }
}
