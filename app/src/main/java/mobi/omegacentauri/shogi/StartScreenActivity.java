package mobi.omegacentauri.shogi;

import android.app.Activity;
import android.app.AlertDialog;
import android.app.Dialog;
import android.app.ProgressDialog;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.SharedPreferences;
import android.os.AsyncTask;
import android.os.Bundle;
import android.preference.PreferenceManager;
import android.util.Log;
import android.view.View;
import android.widget.Button;
import android.widget.Toast;

import java.io.BufferedInputStream;
import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.net.MalformedURLException;
import java.net.URL;
import java.util.zip.ZipEntry;
import java.util.zip.ZipInputStream;

/**
 * The activity launched when the Shogi application starts
 *
 */
public class StartScreenActivity extends Activity {
  static final String TAG = "ShogiStart";
  static final int DIALOG_NEW_GAME = 1233;
  static final int DIALOG_INSTALL_SHOGI_DATA = 1234;
  static final int DIALOG_START_SHOGI_DATA = 1235;
  static final int DIALOG_FATAL_ERROR = 1236;
  private File mExternalDir;
  private SharedPreferences mPrefs;
  private String mErrorMessage;
  private Button newGameButton;
  private Button downloadButton;
  private BonanzaInitializeThread bonanzaInitializeThread = null;

  @Override
  public void onCreate(Bundle savedInstanceState) {
    super.onCreate(savedInstanceState);
    setContentView(R.layout.start_screen);
    mExternalDir = getExternalFilesDir(null);
    if (mExternalDir == null) {
      FatalError("Please mount the sdcard on the device");
      finish();
      return;
    }
    else if (!hasRequiredFiles(mExternalDir)) {
      downloadData();
    }

    mPrefs = PreferenceManager.getDefaultSharedPreferences(getBaseContext());

    newGameButton = (Button)findViewById(R.id.new_game_button);
    newGameButton.setOnClickListener(new Button.OnClickListener() {
      public void onClick(View v) { newGame(); }
    });

    downloadButton = (Button)findViewById(R.id.data_download_button);
    downloadButton.setOnClickListener(new Button.OnClickListener() {
      public void onClick(View v) { downloadData(); }
    });

    Button pickLogButton = (Button)findViewById(R.id.pick_log_button);
    pickLogButton.setOnClickListener(new Button.OnClickListener() {
      public void onClick(View v) {
        startActivity(new Intent(v.getContext(), GameLogListActivity.class));
      }
    });

    Button optionsButton = (Button)findViewById(R.id.options_button);
    optionsButton.setOnClickListener(new Button.OnClickListener() {
      public void onClick(View v) {
        startActivity(new Intent(v.getContext(), ShogiPreferenceActivity.class));
      }
    });

    Button helpButton = (Button)findViewById(R.id.help_button);
    helpButton.setOnClickListener(new Button.OnClickListener() {
      public void onClick(View v) {
        startActivity(new Intent(v.getContext(), HelpActivity.class));
      }
    });


    checkIfReady();
  }

  public void downloadData() {
    ZipDownloader zd = new ZipDownloader(this, mExternalDir, REQUIRED_FILES);
    try {
      Log.v("Shogi", "downloading");
      zd.execute(new URL("https://github.com/arpruss/AndroidShogiData/blob/master/assets/shogi-data.zip?raw=true"));
    } catch (MalformedURLException e) {
      Log.v("Shogi", ""+e);
    }
  }

  void checkIfReady() {
    if (bonanzaInitializeThread != null)
      return;
    if (hasRequiredFiles(mExternalDir)) {
      downloadButton.setVisibility(View.GONE);
      newGameButton.setVisibility(View.VISIBLE);
      bonanzaInitializeThread = new BonanzaInitializeThread();
      bonanzaInitializeThread.start();
    }
    else {
      downloadButton.setVisibility(View.VISIBLE);
      newGameButton.setVisibility(View.GONE);
    }
  }

  @Override
  protected void onResume() {
    super.onResume();
    checkIfReady();
  }

  @Override
  protected Dialog onCreateDialog(int id) {
    switch (id) {
    case DIALOG_NEW_GAME: {
      StartGameDialog d = new StartGameDialog(
          this, 
          getResources().getString(R.string.new_game),
          new DialogInterface.OnClickListener() {
            public void onClick(DialogInterface dialog, int id) {
              newGame2();
            }}
          );
      return d.getDialog();
    }
    case DIALOG_FATAL_ERROR:
      return newFatalErrorDialog();
    default:    
      return null;
    }
  }

  @Override protected void onPrepareDialog(int id, Dialog d) {
    if (id == DIALOG_FATAL_ERROR) {
      ((AlertDialog)d).setMessage(mErrorMessage);
    }
  }

  private void FatalError(String message) {
    mErrorMessage = message;
    showDialog(DIALOG_FATAL_ERROR);
  }

  private Dialog newFatalErrorDialog() {
    DialogInterface.OnClickListener cb = new DialogInterface.OnClickListener() {
      public void onClick(DialogInterface dialog, int id) {  };
    };
    AlertDialog.Builder builder = new AlertDialog.Builder(this);
    // The dialog message will be set in onPrepareDialog
    builder.setMessage("???") 
    .setCancelable(false)
    .setPositiveButton("Ok", cb);
    return builder.create();
  }

  private void newGame() {
    if (!hasRequiredFiles(mExternalDir)) {
      // Note: this shouldn't happen, since the button is disabled if !hasRequiredFiles
      Util.showErrorDialog(
          getBaseContext(),
          "Please download the shogi database files first");
    } else {
      showDialog(DIALOG_NEW_GAME);
    }
  }

  private void newGame2() {
    Intent intent = new Intent(this, GameActivity.class);
    Board b = new Board();
    Handicap h = Handicap.parseInt(Integer.parseInt(mPrefs.getString("handicap", "0")));
    b.initialize(h);
    intent.putExtra("initial_board", b);
    intent.putExtra("handicap", h);
    startActivity(intent);
  }

  private class BonanzaInitializeThread extends Thread {
    @Override public void run() {
      BonanzaJNI.initialize(mExternalDir.getAbsolutePath());
    }
  }

  /**
   * See if all the files required to run Bonanza are present in externalDir.
   */
  private static final String[] REQUIRED_FILES = {
    "book.bin", "fv.bin", "hash.bin"
  };
  public static boolean hasRequiredFiles(File externalDir) {
    for (String basename: REQUIRED_FILES) {
      File file = new File(externalDir, basename);
      if (!file.exists()) {
        Log.d(TAG, file.getAbsolutePath() + " not found");
        return false;
      }
    }
    return true;
  }

  private class ZipDownloader extends AsyncTask<URL, Integer, Long> {
    long totalSize = 0;
    int files = 0;
    Activity mActivity;
    File mDir;
    String[] mFilesList;
    ProgressDialog progress;

    public ZipDownloader(Activity activity, File dir, String[] filesList) {
      mActivity = activity;
      mDir = dir;
      mFilesList = filesList;
    }

      @Override
      protected void onPreExecute()
      {
          super.onPreExecute();
          progress = new ProgressDialog(mActivity);
          progress.setMessage("Downloading engine data ...");
          progress.setProgressStyle(ProgressDialog.STYLE_HORIZONTAL);
          progress.setIndeterminate(false);
          progress.setCancelable(false);
          progress.setMax(mFilesList.length);
          progress.show();
      }

    @Override
    protected Long doInBackground(URL... urls) {
      int count = urls.length;
      long totalSize = 0;
      for (int i=0; i<urls.length; i++) {
        try {
          totalSize += downloadZip(urls[i].openStream());
        } catch (IOException e) {
          return (long)(-1);
        }
      }
      return files == mFilesList.length ? totalSize : -1;
    }

    private void clearFiles() {
      for (int i=0; i<mFilesList.length; i++) {
        try {
          new File(mDir + "/" + mFilesList[i]).delete();
        }
        catch (Exception e) {
        }
        try {
          new File(mDir + "/" + mFilesList[i]+".download").delete();
        }
        catch (Exception e) {
        }
      }
    }

    private long downloadZip(InputStream inputStream) throws IOException {
      clearFiles();

      File dir = getExternalFilesDir(null);
      ZipInputStream zis = new ZipInputStream(new BufferedInputStream(inputStream));
      long totalSize = 0;

      ZipEntry entry;
      while((entry = zis.getNextEntry()) != null) {
        publishProgress(files, 0);
        String name = entry.getName();
        int fileNum = -1;
        for (int i = 0 ; i < mFilesList.length; i++) {
          if (mFilesList[i].equals(name)) {
            fileNum = i;
            break;
          }
        }
        if (fileNum < 0)
          break;

        File outFile = new File(mDir + "/" + name);
        File tempFile = new File(mDir + "/" + name + ".download");
        try {
          tempFile.delete();
        }
        catch(Exception e) {
        }
        try {
          outFile.delete();
        }
        catch(Exception e) {
        }
        FileOutputStream out = new FileOutputStream(tempFile);
        byte[] buf = new byte[32768];
        int copied = 0;
        int n;
        while ((n=zis.read(buf)) > 0) {
          out.write(buf, 0, n);
          copied += n;
          publishProgress(files, copied);
        }
        out.close();
        try {
          tempFile.renameTo(outFile);
        }
        catch(Exception e) {
          tempFile.delete();
          throw(e);
        }
        files++;
        totalSize += copied;
      }
      return totalSize;
    }

    protected void onPostExecute(Long result) {
      progress.dismiss();
      if (result < 0) {
        Toast.makeText(mActivity, "Error downloading", Toast.LENGTH_LONG).show();
        clearFiles();
        finish();
      }
      checkIfReady();
    }

    protected void onProgressUpdate(Integer... pos) {
      progress.setProgress(pos[0]);
      if (pos[1] > 1024*1024) {
          int mb = pos[1] / (1024*1024);
          int decimal = (pos[1] * 10 / (1024*1024)) % 10;
          progress.setMessage("Downloading "+mb+"."+decimal+" mb");
      }
      else
        progress.setMessage("Downloading "+pos[1]+" bytes");
      progress.show();
    }

  }
}
