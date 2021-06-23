package com.ysaito.shogi.test;

import java.io.BufferedReader;
import java.io.ByteArrayInputStream;
import java.io.ByteArrayOutputStream;
import java.io.File;
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.util.Calendar;
import java.util.GregorianCalendar;
import java.util.Map;

import com.ysaito.shogi.GameLog;
import com.ysaito.shogi.ParseException;

import android.content.res.Resources;
import android.test.InstrumentationTestCase;
import android.util.Log;

public class GameLogTest extends InstrumentationTestCase {
  static final String TAG = "MoveLogTest";
  private GameLog openKifFile(int id) throws ParseException, IOException {
    Resources resources = getInstrumentation().getContext().getResources();
    InputStream in = resources.openRawResource(id);
    GameLog log = GameLog.parseKif(new File("/nonexistent/path"), in); 
    in.close();
    return log;
  }
  
  private GameLog openHtmlFile(int id) throws ParseException, IOException {
    Resources resources = getInstrumentation().getContext().getResources();
    InputStream in = resources.openRawResource(id);
    GameLog log = GameLog.parseHtml(new File("/nonexistent/path"), in);
    in.close();
    return log;
  }
  
  public void testParseDate() throws ParseException {
    GregorianCalendar c = new GregorianCalendar();
    c.setTimeInMillis(GameLog.TEST_parseDate("2011/1/10"));
    assertEquals(c.get(Calendar.YEAR), 2011);
    assertEquals(c.get(Calendar.MONTH), Calendar.JANUARY);
    assertEquals(c.get(Calendar.DAY_OF_MONTH), 10);
    
    c.setTimeInMillis(GameLog.TEST_parseDate("2011/1/10 11:23"));
    assertEquals(c.get(Calendar.HOUR_OF_DAY), 11);
    assertEquals(c.get(Calendar.MINUTE), 23);
    
    c.setTimeInMillis(GameLog.TEST_parseDate("2011/1/10 11:23:34"));
    assertEquals(c.get(Calendar.HOUR_OF_DAY), 11);
    assertEquals(c.get(Calendar.MINUTE), 23);
    Log.e(TAG, "EEE " + c.get(Calendar.SECOND) + "//" + c.get(Calendar.MILLISECOND));
    assertEquals(c.get(Calendar.SECOND), 34);    
  }
  
  public void testLoad() throws ParseException, IOException {
    Log.d(TAG, "StartTestLoad3");
    GameLog log = openKifFile(R.raw.kifu2);
    assertEquals(log.attr(GameLog.ATTR_TITLE), "第60回NHK杯3回戦第6局");
    assertEquals(log.attr(GameLog.ATTR_BLACK_PLAYER), "佐藤康光");
    assertEquals(log.attr(GameLog.ATTR_WHITE_PLAYER), "久保利明");
    assertEquals(log.attr(GameLog.ATTR_HANDICAP), "平手");
    assertEquals(log.attr(GameLog.ATTR_TOURNAMENT), "ＮＨＫ杯");
    assertEquals(log.play(4).toCsaString(), "2625FU");
    assertEquals(log.play(94).toCsaString(), "7765KE");
    assertEquals(log.numPlays(), 157);
 }
  
  public void testLoad3() throws ParseException, IOException {
    GameLog log = openKifFile(R.raw.kifu3);
  }
  
  public void testLoad4() throws ParseException, IOException {
    GameLog log = openKifFile(R.raw.kifu4);
  }
  
  public void testHtml() throws ParseException, IOException {
    GameLog log = openHtmlFile(R.raw.download1);
    assertEquals(log.attr(GameLog.ATTR_WHITE_PLAYER), "早川俊");
    assertEquals(log.play(125).toCsaString(), "0085KY");
    assertEquals(log.numPlays(), 134);
  }
  
  public void testHtml2() throws ParseException, IOException {
    GameLog log = openHtmlFile(R.raw.download2);
    assertEquals(log.attr(GameLog.ATTR_WHITE_PLAYER), "上野裕和");
    assertEquals(log.attr(GameLog.ATTR_BLACK_PLAYER), "稲葉陽");    
    assertEquals(log.play(64).toCsaString(), "7785KE");
    assertEquals(log.numPlays(), 121);
  }
  
  public void testToKif_Basic() throws ParseException, IOException {
    GameLog log = openKifFile(R.raw.kifu5);
    ByteArrayOutputStream out = new ByteArrayOutputStream();
    log.toKif(out, "kif_utf8");
    
    BufferedReader in = new BufferedReader(new InputStreamReader(new ByteArrayInputStream(out.toByteArray())));
    assertEquals(in.readLine(), "先手：Player2");
    assertEquals(in.readLine(), "後手：Player1");
    assertEquals(in.readLine(), "手数----指手---------消費時間--");
    assertEquals(in.readLine(), "   1 ７六歩(77)");
    assertEquals(in.readLine(), "   2 ３四歩(33)");
    assertEquals(in.readLine(), "   3 ７五歩(76)");        
    assertEquals(in.readLine(), "   4 ８四歩(83)");        
    assertEquals(in.readLine(), "   5 ８八角成(22)");      
    assertEquals(in.readLine(), "   6 ８八銀(79)");         
    assertEquals(in.readLine(), null);
    String line;
    while ((line = in.readLine()) != null) {
      Log.d(TAG, String.format("BASIC: " + line));
    }
  }
  
  public void testToKif_UTF8() throws ParseException, IOException {
    GameLog log = openKifFile(R.raw.kifu4);
    ByteArrayOutputStream out = new ByteArrayOutputStream();
    log.toKif(out, "kif_utf8");
    
    ByteArrayInputStream in = new ByteArrayInputStream(out.toByteArray());
    GameLog log2 = GameLog.parseKif(new File("/nonexistent/path"), in);
    assertLogEquals(log, log2);
  }

  public void testToKif_MSDOS() throws ParseException, IOException {
    GameLog log = openKifFile(R.raw.kifu4);
    ByteArrayOutputStream out = new ByteArrayOutputStream();
    log.toKif(out, "kif_dos");
    
    ByteArrayInputStream in = new ByteArrayInputStream(out.toByteArray());
    GameLog log2 = GameLog.parseKif(new File("/nonexistent/path"), in);
    assertLogEquals(log, log2);
  }
  
  private void assertLogEquals(GameLog l1, GameLog l2) {
    assertEquals(l1.attrs().size(), l2.attrs().size());
    
    for (Map.Entry<String, String> e: l1.attrs()) {
      assertEquals(l2.attr(e.getKey()), e.getValue());
    }
    
    assertEquals(l1.numPlays(), l2.numPlays());
    for (int i = 0; i < l1.numPlays(); ++i) {
      Log.d(TAG, String.format("MOVE: %d %s %s",
          i, l1.play(i).toString(),
          l2.play(i).toString()));
      assertTrue(l1.play(i).equals(l2.play(i)));
    }
  }
}