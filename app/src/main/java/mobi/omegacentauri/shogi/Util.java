// Copyright 2011 Google Inc. All Rights Reserved.

package mobi.omegacentauri.shogi;

import org.mozilla.universalchardet.UniversalDetector;

import android.app.AlertDialog;
import android.content.Context;
import android.content.DialogInterface;
import android.os.Build;

import java.io.ByteArrayInputStream;
import java.io.ByteArrayOutputStream;
import java.io.File;
import java.io.FileFilter;
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.io.Reader;
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

}
