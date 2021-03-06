// Copyright 2010 Google Inc. All Rights Reserved.

package mobi.omegacentauri.shogi;

/**
 * @author saito@google.com (Your Name Here)
 *
 */
public enum GameState {
  ACTIVE,     // game ongoing
  WHITE_WON,  // white player won
  BLACK_WON,  // black player won
  DRAW,       // draw (sennichite)
  FATAL_ERROR
}
