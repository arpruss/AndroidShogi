package mobi.omegacentauri.shogi;

import java.security.InvalidParameterException;

public enum Handicap {
  NONE,
  KYO,
  KAKU,
  HI,
  HI_KYO,
  HI_KAKU,
  FOUR,
  SIX;
  
  public final String toJapaneseString() {
    if (this == KYO) return "香落";
    if (this == KAKU) return "角落";
    if (this == HI) return "飛落";
    if (this == HI_KYO) return "飛香落";
    if (this == HI_KAKU) return "飛角落";
    if (this == FOUR) return "四枚落";
    if (this == SIX) return "六枚落";
    return "平手";
  }
  
  static Handicap parseJapaneseString(String s) throws InvalidParameterException {
    if (s.startsWith("平手")) return NONE;
    if (s.startsWith("香落")) return KYO;
    if (s.startsWith("右香落")) return KYO;
    if (s.startsWith("角落")) return KAKU;
    if (s.startsWith("飛落") || s.startsWith("飛車落")) return HI;
    if (s.startsWith("飛香落")) return HI_KYO;       
    if (s.startsWith("飛角落") || s.startsWith("二枚落")) return HI_KAKU;
    if (s.startsWith("四枚落")) return FOUR;
    if (s.startsWith("六枚落")) return SIX;    
    throw new InvalidParameterException("Failed to parse " + s + " as Shogi handicap");
  }
  
  static Handicap parseInt(int i) {
    switch (i) {
    case 0: return NONE;
    case 1: return KYO;
    case 2: return KAKU;
    case 3: return HI;
    case 4: return HI_KYO;
    case 5: return HI_KAKU;
    case 6: return FOUR;
    case 7: return SIX;
    default: return NONE;
    }
  }
}
