package com.ysaito.shogi;

import java.util.HashMap;

/**
 * Shogi piece types. They are stored in Board.mSquares. 
 * 
 * Caution: These definitions are shared with Bonanza. Don't change!
 */ 
public class Piece {
  public static final int EMPTY = 0;  // placeholder for an unoccupied square
  public static final int FU = 1;
  public static final int KYO = 2;
  public static final int KEI = 3;
  public static final int GIN = 4;
  public static final int KIN = 5;
  public static final int KAKU = 6;
  public static final int HI = 7;
  public static final int OU = 8;
  public static final int TO = 9;
  public static final int NARI_KYO = 10;
  public static final int NARI_KEI = 11;
  public static final int NARI_GIN = 12;
  public static final int UMA = 14;
  public static final int RYU = 15;
  public static final int NUM_TYPES = 16;
  
  // CSA format string for each piece.
  public final static String[] csaNames = {
    null, "FU", "KY", "KE", "GI", "KI", "KA", "HI", "OU", 
          "TO", "NY", "NK", "NG", null, "UM", "RY"
  };
  
  public static final String japaneseNames[] = {
    null, "歩", "香","桂","銀","金","角","飛", "王",
        "と", "成香", "成桂", "成銀", null, "馬", "龍" };
  
  public static final HashMap<String, String> alternateJapaneseNames;
  static {
    alternateJapaneseNames = new HashMap<String, String>();
    alternateJapaneseNames.put("竜", "龍");
  }
  
  public static int fromCsaName(String s) {
    for (int i = 0; i < csaNames.length; ++i) {
      String n = csaNames[i];
      if (n != null && n.equals(s)) return i;
    }
    throw new AssertionError("Illegal piece name: " + s);
  }
  
}
