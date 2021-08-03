// Copyright 2010 Google Inc. All Rights Reserved.

package mobi.omegacentauri.shogi;

import android.app.AlertDialog;
import android.content.DialogInterface;
import android.content.Intent;
import android.os.Bundle;
import android.preference.Preference;
import android.preference.PreferenceActivity;
import android.preference.PreferenceManager;
import android.text.Html;
import android.view.Menu;
import android.view.MenuInflater;
import android.view.MenuItem;

/**
 * @author saito@google.com (Your Name Here)
 *
 * Preference activity
 */
public class ShogiPreferenceActivity extends PreferenceActivity {
    public static final String DEFAULT_BOARD = "board_rich_brown";
    public static final String DEFAULT_PIECES = "kanji_light_threedim";

    @Override public void onCreate(Bundle savedInstanceState) {
    super.onCreate(savedInstanceState);
    addPreferencesFromResource(R.xml.preferences);
    findPreference("licenses").setOnPreferenceClickListener(new Preference.OnPreferenceClickListener() {
      @Override
      public boolean onPreferenceClick(Preference preference) {
          AlertDialog alertDialog = new AlertDialog.Builder(ShogiPreferenceActivity.this).create();
          alertDialog.setTitle("Licenses and copyrights");
          alertDialog.setMessage(Html.fromHtml(Util.getAssetFile(ShogiPreferenceActivity.this, "license.html")));
          alertDialog.setButton(DialogInterface.BUTTON_POSITIVE, "OK",
                  new DialogInterface.OnClickListener() {
                    public void onClick(DialogInterface dialog, int which) {} });
          alertDialog.setOnCancelListener(new DialogInterface.OnCancelListener() {
            public void onCancel(DialogInterface dialog) {} });
          alertDialog.show();
        return false;
      }
    });
  }
}

