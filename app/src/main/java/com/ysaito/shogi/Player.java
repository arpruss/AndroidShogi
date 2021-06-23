// Copyright 2010 Google Inc. All Rights Reserved.

package com.ysaito.shogi;

/**
 * Type of the player. "BLACK" is "sente(先手)" in japanese. BLACK makes the first move
 * of the game. "WHITE" is "gote(後手)" in japanese.
 */
public enum Player {
  INVALID,
  BLACK,
  WHITE;

  public final Player opponent() {
    if (this == BLACK) return WHITE;
    if (this == WHITE) return BLACK;
    throw new AssertionError("Invalid player: " + toString());
  }
}
