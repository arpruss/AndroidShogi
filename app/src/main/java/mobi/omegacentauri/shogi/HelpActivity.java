package mobi.omegacentauri.shogi;

import java.io.InputStream;
import java.io.InputStreamReader;

import android.app.Activity;
import android.content.res.Resources;
import android.os.Bundle;
import android.webkit.WebView;

public class HelpActivity extends Activity {
  @Override public void onCreate(Bundle savedInstanceState) {
    super.onCreate(savedInstanceState);
    setContentView(R.layout.help);
    WebView browser = (WebView)findViewById(R.id.helpview);
    browser.loadData(helpString(), "text/html", "utf-8");
  }

  private final String helpString() {
    Resources resources = getResources();
    InputStream in = resources.openRawResource(R.raw.help_html);
    StringBuilder b = new StringBuilder();
    try {
      InputStreamReader reader = new InputStreamReader(resources.openRawResource(R.raw.help_html), "utf-8");
      char[] buf = new char[16384];
      int n;
      while ((n = reader.read(buf)) >= 0) {
        for (int i = 0; i < n; ++i) {
          b.append(buf[i] == '\n' ? ' ' : buf[i]);
        }
      }
      in.close();
    } catch (Exception e) {
      b.append("Error: " + e.getMessage());
    } 
    return b.toString();
  }
}
