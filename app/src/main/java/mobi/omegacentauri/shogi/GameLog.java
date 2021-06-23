package mobi.omegacentauri.shogi;

import java.io.BufferedReader;
import java.io.File;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.io.OutputStreamWriter;
import java.io.Reader;
import java.io.Serializable;
import java.io.StringReader;
import java.io.Writer;
import java.security.MessageDigest;
import java.security.NoSuchAlgorithmException;
import java.util.ArrayList;
import java.util.Calendar;
import java.util.Comparator;
import java.util.GregorianCalendar;
import java.util.Map;
import java.util.NoSuchElementException;
import java.util.Scanner;
import java.util.Set;
import java.util.TreeMap;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

import android.util.Log;

/**
 * GameLog stores the log of a single game, along with its attributes, such as the date and time of the game 
 * and the names of the players.
 */
@SuppressWarnings("serial") 
public class GameLog implements Serializable {
  private static final String TAG = "GameLog";
  
  // Common mAttrs keys. They are also the standard KIF headers.
  public static final String ATTR_TITLE = "表題";
  public static final String ATTR_LOCATION = "場所";
  public static final String ATTR_TIME_LIMIT =  "持ち時間";
  public static final String ATTR_TOURNAMENT = "棋戦";
  public static final String ATTR_BLACK_PLAYER = "先手";
  public static final String ATTR_WHITE_PLAYER = "後手";
  public static final String ATTR_HANDICAP = "手合割";    
  
  /**
   * List of attributes names (ATTR_TITLE, etc) and their values. This object
   * must be an ordered map so that digest() can compute a deterministic value.
   */
  private TreeMap<String, String> mAttrs;
  
  private long mStartTimeMs;  // UTC in millisec
  private ArrayList<Play> mPlays;
  private String mDigest;  // cached value of getDigest().
  private File mPath;  // the path on sdcard. null in the log is only in memory
  
  private GameLog() {
    mAttrs = new TreeMap<String, String>();
    mPlays = new ArrayList<Play>();
    mPath = null;
  }
  
  public String attrsToString() {
    StringBuilder b = new StringBuilder();
    for (Map.Entry<String, String> e: mAttrs.entrySet()) {
      b.append(e.getKey());
      b.append(": ");
      b.append(e.getValue());
    }
    return b.toString();
  }

  @Override public boolean equals(Object o) {
    if (o instanceof GameLog) {
      return digest().equals(((GameLog)o).digest());
    } else {
      return false;
    }
  }
  
  @Override public int hashCode() {
    return digest().hashCode();
  }

  /**
   * Get the value of attribute "key". Return null if the attr is not set.
   */
  public final String attr(String key) {
    return mAttrs.get(key);
  }

  /** 
   * Return the local sdcard path in which the log is saved. Returns null if
   * the log is not in sdcard. 
   */
  public final File path() { return mPath; }
  
  /**
   * Get the list of <attr_key, attr_value> pairs.
   */
  public final Set<Map.Entry<String, String>> attrs() {
    return mAttrs.entrySet();
  }

  /**
   * Get the SHA-1 digest of this object.
   */
  public String digest() {
    if (mDigest == null) {
      try {
        MessageDigest digest = MessageDigest.getInstance("SHA-1");
        for (Map.Entry<String, String> e : mAttrs.entrySet()) {
          digest.update(e.getKey().getBytes());
          digest.update(e.getValue().getBytes());
        }
        for (int i = 0; i < mPlays.size(); ++i) {
          digest.update(mPlays.get(i).toString().getBytes());
        }
        mDigest = Util.bytesToHexText(digest.digest());
      } catch (NoSuchAlgorithmException e) {
        throw new AssertionError("MessageDigest.NoSuchAlgorithmException: " + e.getMessage());
      }
    }
    return mDigest;
  }
  
  public final long getDate() { return mStartTimeMs; }
  public final String dateString() { return toKifDateString(mStartTimeMs); }
  
  public final Handicap handicap() {
    String handicapString = mAttrs.get(GameLog.ATTR_HANDICAP);
    if (handicapString != null) {
      return Handicap.parseJapaneseString(handicapString);
    }
    return Handicap.NONE;
  }
  
  public final Play play(int n) { return mPlays.get(n); }
  public final int numPlays() { return mPlays.size(); }
  public final ArrayList<Play> plays() { return mPlays; }

  public final String getPlayer(String playerAttr) {
    String name = mAttrs.get(playerAttr);
    if (name == null) name = "";
    return name;
  }

  /**
   * A Comparator to sort GameLog by date (oldest first)
   */
  public static final Comparator<GameLog> SORT_BY_DATE = new Comparator<GameLog>() {
    public int compare(GameLog g1, GameLog g2) { 
      if (g1.getDate() < g2.getDate()) return -1;
      if (g1.getDate() > g2.getDate()) return 1;      

      // Use player name, then digest as a tiebreaker.
      
      // int x = BY_PLAYERS.compare(g1, g2);
      // if (x != 0) return x;
      return g1.digest().compareTo(g2.digest());
    }
    @Override 
    public boolean equals(Object o) { return o == this; }
  };
  
  /**
   * A comparator to sort GameLog by the player names
   */
  public static final Comparator<GameLog> SORT_BY_BLACK_PLAYER = new Comparator<GameLog>() {
    public int compare(GameLog g1, GameLog g2) { 
      String p1 = g1.getPlayer(GameLog.ATTR_BLACK_PLAYER);
      String p2 = g2.getPlayer(GameLog.ATTR_BLACK_PLAYER);      
      int cmp = p1.compareTo(p2);
      return (cmp != 0) ? cmp : SORT_BY_DATE.compare(g1, g2); 
    }
    @Override
    public boolean equals(Object o) { return o == this; }
  };

  public static final Comparator<GameLog> SORT_BY_WHITE_PLAYER = new Comparator<GameLog>() {
    public int compare(GameLog g1, GameLog g2) { 
      String p1 = g1.getPlayer(GameLog.ATTR_WHITE_PLAYER);
      String p2 = g2.getPlayer(GameLog.ATTR_WHITE_PLAYER);      
      int cmp = p1.compareTo(p2);
      return (cmp != 0) ? cmp : SORT_BY_DATE.compare(g1, g2); 
    }
    @Override
    public boolean equals(Object o) { return o == this; }
  };
  
  //
  // Methods to parse .kif and .html files into a GameLog object
  //
  private static final Pattern DATE_PATTERN = Pattern.compile("開始日時[：:](.*)");
  private static final Pattern PLAY_PATTERN = Pattern.compile("\\s*[0-9]+\\s+(.*)");  
  private static final Pattern OPTIONAL_DAY_OF_WEEK_PATTERN = Pattern.compile("[(（][月火水木金土日][）)]");
  private static final Pattern UNKNOWN_ATTR_PATTERN = Pattern.compile("(\\S+)[：:](.+)");
  
  private static class AttrPattern {
    public String attr;
    public Pattern pattern;
  }
  private static final AttrPattern[] mAttrPatterns;
  
  public static final String STANDARD_ATTR_NAMES[] = {
    ATTR_BLACK_PLAYER, ATTR_WHITE_PLAYER, ATTR_HANDICAP,
    ATTR_TITLE, ATTR_LOCATION, ATTR_TIME_LIMIT, ATTR_TOURNAMENT,
  };
  
  static {
    mAttrPatterns = new AttrPattern[STANDARD_ATTR_NAMES.length];
    for (int i = 0; i < STANDARD_ATTR_NAMES.length; ++i) {
      mAttrPatterns[i] = new AttrPattern();
      mAttrPatterns[i].attr = STANDARD_ATTR_NAMES[i];
      mAttrPatterns[i].pattern= Pattern.compile(STANDARD_ATTR_NAMES[i] + "[：:]\\s*(.*)");
    }
  }

  private static final Pattern HTML_KIF_START_PATTERN = Pattern.compile(".*>\\s*((開始日時|棋戦|場所|表題|手合割|先手|後手)[:：].*)");
  private static final Pattern HTML_KIF_END_PATTERN = Pattern.compile("([^<]*)<.*");

  public static GameLog newLog(
      long startTimeMs, 
      Set<Map.Entry<String, String>> attrs,
      ArrayList<Play> plays,
      File path) {
    GameLog log = new GameLog();
    log.mStartTimeMs = startTimeMs;
    log.mAttrs = new TreeMap<String,String>();
    for (Map.Entry<String, String> e : attrs) {
      log.mAttrs.put(e.getKey(), e.getValue());
    }
    log.mPlays = new ArrayList<Play>(plays);
    log.mPath = path;
    return log;
  }

  /** 
   * Parse an embedded KIF file downloaded from http://wiki.optus.nu/.
   * Such a file can be created by saving a "テキスト表示" link directly to a file.
   *
   * @param path The path in which the file is stored. Can be null.
   */
  public static GameLog parseHtml(File path, InputStream stream) throws ParseException, IOException {
    BufferedReader reader = new BufferedReader(Util.inputStreamToReader(stream, "EUC-JP"));
    String line;
    StringBuilder output = new StringBuilder();
    boolean kifFound = false;
    while ((line = reader.readLine()) != null && !kifFound) {
      Matcher m = HTML_KIF_START_PATTERN.matcher(line);
      if (m.matches()) {
        output.append(m.group(1));
        output.append('\n');
        while ((line = reader.readLine()) != null) {
          m = HTML_KIF_END_PATTERN.matcher(line);
          if (m.matches()) {
            output.append(m.group(1));
            output.append('\n');
            kifFound = true;
            break;
          } else {
            output.append(line);
            output.append('\n');
          }
        }
        break;
      }
    }
    if (!kifFound) return null;
    return doParseKif(path, new StringReader(output.toString()));
  }
  
  static final boolean ASSUME_SANE_KIF_READER = false;
    
  
  /**
   * Print the contents of this object to "stream" in KIF format.
   * 
   * @param format One of @id array/log_save_format_values.
   * @throws IOException
   */
  public void toKif(OutputStream out, String format) throws IOException {
    Writer stream = null;
    String EOL = null;
    if (format.equals("kif_utf8")) {
      stream = new OutputStreamWriter(out, "UTF-8");
      EOL = "\n";
    } else {
      stream = new OutputStreamWriter(out, "SHIFT-JIS");
      EOL = "\r\n";
    }
    StringBuilder b = new StringBuilder();
    
    // Generate header lines
    if (mStartTimeMs > 0) {
      b.append("開始日時：").append(toKifDateString(mStartTimeMs)).append(EOL);
    }
    for (Map.Entry<String, String> e : mAttrs.entrySet()) {
      String japaneseName = e.getKey();
      b.append(japaneseName).append("：").append(e.getValue()).append(EOL);
    }
    b.append("手数----指手---------消費時間--").append(EOL);
    Board board = new Board();
    board.initialize(Handicap.NONE);
    Player player = Player.BLACK;
    for (int i = 0; i < mPlays.size(); ++i) {
      Play thisPlay = mPlays.get(i);
      Play prevPlay = (i > 0 ? mPlays.get(i - 1) : null);
      Play.TraditionalNotation n = thisPlay.toTraditionalNotation(board, prevPlay);
      b.append(String.format("%4d %s%s%s", 
          i + 1,
          Play.japaneseRomanNumbers[n.x],
          Play.japaneseNumbers[n.y],
          Piece.japaneseNames[Board.type(n.piece)]));
      if ((n.modifier & Play.PROMOTE) != 0) {
        b.append("成");
      }
      if (!thisPlay.isDroppingPiece()) {
        b.append(String.format("(%d%d)", 
            9 - thisPlay.fromX(), 1 + thisPlay.fromY()));
      } else {
        b.append("打");
      }
      b.append(EOL);
      board.applyPly(player, thisPlay);
      player = player.opponent();
    }
    stream.write(b.toString());
    stream.close();
  }
  
  /** 
   * Given a KIF file encoded in UTF-8, parse it. If this method doesn't throw 
   * an exception, it always return a non-null GameLog object.
   */
  public static GameLog parseKif(File path, InputStream in) throws ParseException, IOException {
    Reader stream = Util.inputStreamToReader(in, "SHIFT-JIS");
    return doParseKif(path, stream);
  }
  
  /**
   * @param path The local sdcard path in which the kif data is stored. Should
   * be null if the data is not on sdcard.
   */
  private static GameLog doParseKif(File path, Reader stream) throws ParseException, IOException {
    GameLog l = new GameLog();
    l.mPath = path;
    
    Scanner scanner = new Scanner(stream);
    Play prevPlay = null;
    Player curPlayer = Player.BLACK;
    while (scanner.hasNextLine()) {
      String line = scanner.nextLine();
      boolean matched = false;
      for (int i = 0; i < mAttrPatterns.length; ++i) {
        AttrPattern attr = mAttrPatterns[i];
        Matcher matcher = attr.pattern.matcher(line);  
        if (matcher.matches()) {
          l.mAttrs.put(attr.attr, matcher.group(1));
          matched = true;
          break;
        }
      }
      if (matched) continue;
      Matcher matcher = DATE_PATTERN.matcher(line);
      if (matcher.matches()) {
        l.mStartTimeMs = parseDate(matcher.group(1));
        continue;
      }

      // Skip lines that don't contain information
      if (line.startsWith("手数")) continue;
      if (line.startsWith("まで")) continue;

      matcher = PLAY_PATTERN.matcher(line);
      if (matcher.matches()) {
        String playString = matcher.group(1);
        if (playString.startsWith("投了")) {
          continue;
        } else {
          Play m = Play.fromKifString(prevPlay, curPlayer, playString);
          if (m != null) {
            l.mPlays.add(m);
            prevPlay = m;
          }
          curPlayer = curPlayer.opponent();
        }
        continue;
      }
      
      // Parse unsupported attributes. They are just displayed as-is
      matcher = UNKNOWN_ATTR_PATTERN.matcher(line);
      if (matcher.matches()) {
        String attrName = matcher.group(1);
        String attrValue = matcher.group(2);
        l.mAttrs.put(attrName, attrValue);
        Log.d(TAG, "Found attr: " + attrName + "///" + attrValue);
        continue;
      }
      
      Log.e(TAG, line + ": ignoring line");
    }
    return l;
  }
  
  // Given a UTC in milliseconds, return a KIF-style date string.
  private static String toKifDateString(long dateMs) {
    Calendar c = new GregorianCalendar();
    c.setTimeInMillis(dateMs);
    return String.format("%04d/%02d/%02d %02d:%02d:%02d",
        c.get(Calendar.YEAR), 
        c.get(Calendar.MONTH) - Calendar.JANUARY + 1, 
        c.get(Calendar.DAY_OF_MONTH),
        c.get(Calendar.HOUR_OF_DAY), c.get(Calendar.MINUTE), c.get(Calendar.SECOND));
  }
  
  // Exposed for unittesting only.
  public static long TEST_parseDate(String s) throws ParseException {
    return parseDate(s);
  }
  
  // Parse Japanese date string found in KIF file, return UTC in milliseconds. 
  // Supported format is:
  //    YYYY/MM/DD [HH[:MM[:SS]]]
  private static long parseDate(String s) throws ParseException {
    Scanner scanner = new Scanner(s);
    scanner.useDelimiter("[/\\s　(（]");

    int year = 0;
    int month = 0;
    int day = 0;
    int hour = 0;
    int minute = 0;
    int sec = 0;
    
    try {
      if (!scanner.hasNext()) throw new ParseException(s + ": failed to parse year");
      year = Integer.parseInt(scanner.next());
      if (!scanner.hasNext()) throw new ParseException(s + ": failed to parse month");
      month = Integer.parseInt(scanner.next()) - 1;
      if (!scanner.hasNext()) throw new ParseException(s + ": failed to parse day");
      day = Integer.parseInt(scanner.next());
    
      try {
        scanner.skip(OPTIONAL_DAY_OF_WEEK_PATTERN);
      } catch (NoSuchElementException e) {
      }
      scanner.useDelimiter("[:\\s　]");    
      if (scanner.hasNext()) hour = Integer.parseInt(scanner.next());
      if (scanner.hasNext()) minute = Integer.parseInt(scanner.next());
      if (scanner.hasNext()) sec = Integer.parseInt(scanner.next());
    } catch (NumberFormatException e) {
      Log.e(TAG, "Failed to parse "+ s + ": " + e.getMessage());
    }
    // TODO(saito) support multiple timezones
    Calendar c = new GregorianCalendar();
    c.set(year, month, day, hour, minute, sec);
    return c.getTimeInMillis();
  }
}
