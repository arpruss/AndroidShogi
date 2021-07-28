// Copyright 2011 Google Inc. All Rights Reserved.

package mobi.omegacentauri.shogi;

import org.mozilla.universalchardet.UniversalDetector;

import android.app.AlertDialog;
import android.content.Context;
import android.content.DialogInterface;
import android.os.Build;

import java.io.BufferedReader;
import java.io.ByteArrayInputStream;
import java.io.ByteArrayOutputStream;
import java.io.File;
import java.io.FileFilter;
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.io.Reader;
import java.util.ArrayList;
import java.util.regex.Pattern;

/**
 * @author saito@google.com (Yaz Saito)
 *
 */
public class Util {
  private static ThreadLocal<byte[]> mBuf = new ThreadLocal<byte[]>() {
    @Override protected synchronized byte[] initialValue() { 
      return new byte[8192];
    }
  };

  public static Reader inputStreamToReader(InputStream in, String defaultEncoding) throws IOException {
    byte[] contents = Util.streamToBytes(in);
    String encoding = Util.detectEncoding(contents, defaultEncoding);
    return new InputStreamReader(new ByteArrayInputStream(contents), encoding);
  }
  
  /** Read the contents of @p into a byte array */
  public static byte[] streamToBytes(InputStream in) throws IOException {
    byte[] tmpBuf = mBuf.get();
    ByteArrayOutputStream out = new ByteArrayOutputStream();
    int n;
    while ((n = in.read(tmpBuf)) > 0) {
      out.write(tmpBuf, 0, n);  
    }
    return out.toByteArray();
  }
  
  /** Detect character encoding of @p contents. Return null on error */
  public static String detectEncoding(
      byte[] contents,
      String defaultEncoding) {
    UniversalDetector encodingDetector = new UniversalDetector(null);
    
    encodingDetector.reset();
    encodingDetector.handleData(contents, 0, contents.length);
    encodingDetector.dataEnd();
    String encoding = encodingDetector.getDetectedCharset();
    if (encoding == null) {
      encoding = defaultEncoding;
      if (encoding == null) encoding = "SHIFT-JIS";
    }
    return encoding;
  }
  
  public static String bytesToHexText(byte[] b) {
    StringBuffer hex = new StringBuffer();
    for (int i = 0;i < b.length; i++) {
      hex.append(Integer.toHexString(b[i] & 0xff));
    }        
    return hex.toString();
  }
  
  public static String throwableToString(Throwable e) {
    StringBuilder b = new StringBuilder();
    b.append(e.toString()).append("\n");
    for (StackTraceElement elem : e.getStackTrace()) {
      b.append(elem.toString()).append("\n");
    }
    return b.toString();
  }
  
  public static void showErrorDialog(Context context, String error) {
    AlertDialog.Builder b = new AlertDialog.Builder(context);
    b.setMessage(error)
    .setCancelable(false)
    .setPositiveButton("Ok", new DialogInterface.OnClickListener() {
      public void onClick(DialogInterface dialog, int id) {  };
    });
    b.create().show();
  }

  public static void deleteFilesFromDir(File externalFilesDir) {
    try {
      File[] fs = externalFilesDir.listFiles();
      for (File f : fs) {
        try {
          if (f.isFile())
            f.delete();
        }
        catch(Exception e) {
        }
      }
    }
    catch(Exception e) {
    }
  }

  public static int numberOfCores() {
    if (Build.VERSION.SDK_INT >= 17)
      return Runtime.getRuntime().availableProcessors();
    else {
      try {
        return new File("/sys/devices/system/cpu").listFiles(new FileFilter() {
          @Override
          public boolean accept(File file) {
            return Pattern.matches("cpu[0-9]+", file.getName());
          }
        }).length;
      }
      catch(Exception e) {
        return 1;
      }
    }
  }

  static private String getStreamFile(InputStream stream) {
    BufferedReader reader;
    try {
      reader = new BufferedReader(new InputStreamReader(stream));

      String text = "";
      String line;
      while (null != (line=reader.readLine()))
        text = text + line;
      return text;
    } catch (IOException e) {
      // TODO Auto-generated catch block
      return "";
    }
  }

  static public String getAssetFile(Context context, String assetName) {
    try {
      return getStreamFile(context.getAssets().open(assetName));
    } catch (IOException e) {
      // TODO Auto-generated catch block
      return "";
    }
  }

  static public boolean nameIsHuman(Context c, String name) {
    return null == computerLevelFromName(c, name);
  }

  static public String computerLevelFromName(Context c, String level) {
    String levels[] = c.getResources().getStringArray(R.array.computer_level_names);

    for (int i = 0 ; i < levels.length ; i++)
      if (levels[i].equals(level)) {
        return c.getResources().getStringArray(R.array.computer_level_values)[i];
      }

    return null;
  }

  // if human name looks like a computer level number, add " (H)"
  static public String humanSafeName(String name) {
    if (name.startsWith("Lv ")) {
      try {
        Integer.parseInt(name.substring(3));
        return name + " (H)";
      }
      catch(Exception e) {
      }
    }
    return name;
  }

  public static int PlayerTypesToInt(String s) {
    if (s.equals("HC")) return 0;
    if (s.equals("CH")) return 1;
    if (s.equals("HH")) return 2;
    if (s.equals("CC")) return 3;
    return 0;
  }

  public static String IntToPlayerTypes(int v) {
    switch (v) {
      case 1:
        return "CH";
      case 2:
        return "HH";
      case 3:
        return "CC";
      default:
        return "HC";
    }
  }

    public static void getTimesFromPlays(ArrayList<Play> plays, int size, long[] thinkTimeMs) {
      thinkTimeMs[0] = 0;
      thinkTimeMs[1] = 0;

      if (size == 0) {
        return;
      }

      if (size == 1) {
        thinkTimeMs[Player.BLACK.toIndex()] = plays.get(0).endTime();
      }
      else {
        thinkTimeMs[(size - 1) % 2] = plays.get(size - 1).endTime();
        thinkTimeMs[(size - 2) % 2] = plays.get(size - 2).endTime();
      }

      for (int i=0;i<2;i++)
        if (thinkTimeMs[i]<0)
          thinkTimeMs[i] = 0;
    }
}
