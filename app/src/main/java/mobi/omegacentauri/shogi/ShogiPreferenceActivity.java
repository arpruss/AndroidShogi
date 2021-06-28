// Copyright 2010 Google Inc. All Rights Reserved.

package mobi.omegacentauri.shogi;

import android.content.Intent;
import android.os.Bundle;
import android.preference.PreferenceActivity;
import android.preference.PreferenceManager;
import android.view.Menu;
import android.view.MenuInflater;
import android.view.MenuItem;

/**
 * @author saito@google.com (Your Name Here)
 *
 * Preference activity
 */
public class ShogiPreferenceActivity extends PreferenceActivity {
  @Override public void onCreate(Bundle savedInstanceState) {
    super.onCreate(savedInstanceState);
    addPreferencesFromResource(R.xml.preferences);
  }
}
