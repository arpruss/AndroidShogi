// Copyright 2010 Google Inc. All Rights Reserved.

package com.ysaito.shogi;

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
  
  @Override public boolean onCreateOptionsMenu(Menu menu) {
    MenuInflater inflater = getMenuInflater();
    inflater.inflate(R.menu.preferences_menu, menu);
    return true;
  }
  
  @Override public boolean onOptionsItemSelected(MenuItem item) {
    switch (item.getItemId()) {
      case R.id.reset_preferences:
        PreferenceManager.getDefaultSharedPreferences(getBaseContext())
          .edit().clear().commit();
        return true;
      case R.id.preferences_help_menu_id:
        startActivity(new Intent(this, HelpActivity.class));
        return true;
      default:    
        return super.onOptionsItemSelected(item);
    }
  }
}
