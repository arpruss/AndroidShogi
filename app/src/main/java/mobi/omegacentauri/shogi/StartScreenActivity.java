package mobi.omegacentauri.shogi;

import android.app.Activity;
import android.app.AlertDialog;
import android.app.Dialog;
import android.app.ProgressDialog;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.SharedPreferences;
import android.content.pm.ActivityInfo;
import android.os.AsyncTask;
import android.os.Build;
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

import org.tukaani.xz.XZInputStream;

/**
 * The activity launched when the Shogi application starts
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
    private Button pickLogButton;
    static XZDataFile[] dataFiles = new XZDataFile[]{
            new XZDataFile("https://github.com/arpruss/AndroidShogi/raw/95aa56279e60b1485f6f326add8bc06b3ce73867/data/book.bin.xz",
                    "book.bin", 426536),
            new XZDataFile("https://github.com/arpruss/AndroidShogi/raw/95aa56279e60b1485f6f326add8bc06b3ce73867/data/fv.bin.xz",
                    "fv.bin", 186268248),
    };

    static File getExternalDir(Context c) {
        File dir = new File(c.getExternalFilesDir(null)+"/6");
        try {
            dir.mkdir();
        }
        catch(Exception e) {
        }

        return dir;
    }


    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.start_screen);
        Util.deleteFilesFromDir(getExternalFilesDir(null));
        mExternalDir = getExternalDir(this);

        mPrefs = PreferenceManager.getDefaultSharedPreferences(getBaseContext());

        newGameButton = (Button) findViewById(R.id.new_game_button);
        newGameButton.setOnClickListener(new Button.OnClickListener() {
            public void onClick(View v) {
                newGame();
            }
        });

        downloadButton = (Button) findViewById(R.id.data_download_button);
        downloadButton.setOnClickListener(new Button.OnClickListener() {
            public void onClick(View v) {
                downloadData();
            }
        });

        pickLogButton = (Button) findViewById(R.id.pick_log_button);
        pickLogButton.setOnClickListener(new Button.OnClickListener() {
            public void onClick(View v) {
                startActivity(new Intent(v.getContext(), GameLogListActivity.class));
            }
        });

        Button optionsButton = (Button) findViewById(R.id.options_button);
        optionsButton.setOnClickListener(new Button.OnClickListener() {
            public void onClick(View v) {
                startActivity(new Intent(v.getContext(), ShogiPreferenceActivity.class));
            }
        });

        Button helpButton = (Button) findViewById(R.id.help_button);
        helpButton.setOnClickListener(new Button.OnClickListener() {
            public void onClick(View v) {
                startActivity(new Intent(v.getContext(), HelpActivity.class));
            }
        });

        final SharedPreferences prefs = PreferenceManager.getDefaultSharedPreferences(this);
        int id = BoardView.getBoardDrawable(this, prefs.getString("board", "board_rich_brown"));


        checkIfReady();
    }

    public void downloadData() {
        XZDownloader zd = new XZDownloader(this, mExternalDir, dataFiles);
        Log.v("Shogi", "downloading");
        zd.execute();
    }

    Boolean checkIfReady() {
        if (bonanzaInitializeThread != null)
            return true;
        if (hasRequiredFiles(mExternalDir)) {
            downloadButton.setVisibility(View.GONE);
            pickLogButton.setVisibility(View.VISIBLE);
            newGameButton.setVisibility(View.VISIBLE);
            bonanzaInitializeThread = new BonanzaInitializeThread();
            bonanzaInitializeThread.start();
            findViewById(R.id.download_message).setVisibility(View.GONE);
            return true;
        } else {
            downloadButton.setVisibility(View.VISIBLE);
            pickLogButton.setVisibility(View.GONE);
            newGameButton.setVisibility(View.GONE);
            findViewById(R.id.download_message).setVisibility(View.VISIBLE);
            return false;
        }
    }

    @Override
    protected void onResume() {
        super.onResume();
        ((SplashView)findViewById(R.id.splashview)).invalidate();
        if (checkIfReady() && null != GameActivity.getSaveActiveGame(this)) {
            newGame2();
        }
        else {
            GameActivity.deleteSaveActiveGame(this);
        }
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
                            }
                        }
                );
                return d.getDialog();
            }
            case DIALOG_FATAL_ERROR:
                return newFatalErrorDialog();
            default:
                return null;
        }
    }

    @Override
    protected void onPrepareDialog(int id, Dialog d) {
        if (id == DIALOG_FATAL_ERROR) {
            ((AlertDialog) d).setMessage(mErrorMessage);
        }
    }

    private void FatalError(String message) {
        mErrorMessage = message;
        showDialog(DIALOG_FATAL_ERROR);
    }

    private Dialog newFatalErrorDialog() {
        DialogInterface.OnClickListener cb = new DialogInterface.OnClickListener() {
            public void onClick(DialogInterface dialog, int id) {
            }

            ;
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
        @Override
        public void run() {
            BonanzaJNI.initialize(mExternalDir.getAbsolutePath());
        }
    }

    /**
     * See if all the files required to run Bonanza are present in externalDir.
     */
    public static boolean hasRequiredFiles(File externalDir) {
        for (XZDataFile df : dataFiles) {
            File file = new File(externalDir, df.filename);
            if (!file.exists()) {
                Log.d(TAG, file.getAbsolutePath() + " not found");
                return false;
            }
        }
        return true;
    }

    private class XZDownloader extends AsyncTask<Integer, Long, Long> {
        long downloadedSize;
        long totalSize;
        int downloadedFiles = 0;
        Activity mActivity;
        File mDir;
        ProgressDialog progress;
        private int mOldOrientation;
        long progressScale = 1;
        private XZDataFile[] mFiles;

        public XZDownloader(Activity activity, File dir, XZDataFile[] files) {
            mActivity = activity;
            mFiles = files;
            mDir = dir;
            mFiles = dataFiles;
            totalSize = 0;
            for (int i = 0; i < dataFiles.length; i++)
                totalSize += dataFiles[i].uncompressedSize;
        }

        @Override
        protected void onPreExecute() {
            super.onPreExecute();
            while (totalSize / progressScale > Integer.MAX_VALUE)
                progressScale *= 10;
            mOldOrientation = getRequestedOrientation();
            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.JELLY_BEAN_MR2) {
                setRequestedOrientation(ActivityInfo.SCREEN_ORIENTATION_LOCKED);
            } else {
                setRequestedOrientation(ActivityInfo.SCREEN_ORIENTATION_NOSENSOR);
            }
            progress = new ProgressDialog(mActivity);
            progress.setMessage("Downloading engine data ...");
            progress.setProgressStyle(ProgressDialog.STYLE_HORIZONTAL);
            progress.setIndeterminate(false);
            progress.setCancelable(false);
            progress.setMax((int) (totalSize / progressScale));
            progress.show();

        }

        @Override
        protected Long doInBackground(Integer... ignore) {
            downloadedSize = 0;
            downloadedFiles = 0;
            for (int i = 0; i < mFiles.length; i++) {
                try {
                    downloadXZ(mFiles[i]);
                    downloadedFiles++;
                } catch (IOException e) {
                    Log.e("shogilog", "" + e);
                    clean(mFiles[i]);
                    return (long) (-1);
                }
            }
            Log.v("shogilog", "dl " + downloadedSize + " of " + totalSize);
            return downloadedSize == totalSize ? totalSize : -1;
        }

        private void clean(XZDataFile dataFile) {
            File outFile = new File(mDir + "/" + dataFile.filename);
            File tempFile = new File(mDir + "/" + dataFile.filename + ".download");
            try {
                tempFile.delete();
            } catch (Exception e) {
            }
            try {
                outFile.delete();
            } catch (Exception e) {
            }
        }

        private long downloadXZ(XZDataFile dataFile) throws IOException {
            publishProgress((long) downloadedFiles, downloadedSize);
            File outFile = new File(mDir + "/" + dataFile.filename);
            try {
                if (outFile.exists() && outFile.length() == dataFile.uncompressedSize) {
                    downloadedSize += dataFile.uncompressedSize;
                    publishProgress((long) downloadedFiles, downloadedSize);
                    return dataFile.uncompressedSize;
                }
            } catch (Exception e) {
            }

            Log.v("shogilog", "opening " + dataFile.url);
            InputStream stream = new URL(dataFile.url).openStream();
            XZInputStream xs = new XZInputStream(stream);

            File tempFile = new File(mDir + "/" + dataFile.filename + ".download");
            clean(dataFile);
            FileOutputStream out = new FileOutputStream(tempFile);
            byte[] buf = new byte[32768];
            long copied = 0;
            while (copied < dataFile.uncompressedSize) {
                int n;
                int toRead = copied + 32768 <= dataFile.uncompressedSize ? 32768 : (int) (dataFile.uncompressedSize - copied);
                if (toRead != xs.read(buf, 0, toRead))
                    throw new IOException("cannot read");
                out.write(buf, 0, toRead);
                copied += toRead;
                downloadedSize += toRead;
                publishProgress((long) downloadedFiles, downloadedSize);
            }
            if (-1 != xs.read(buf, 0, 1))
                throw new IOException("error with final verification");
            out.close();
            tempFile.renameTo(outFile);

            return copied;
        }

        protected void onPostExecute(Long result) {
            progress.dismiss();
            if (result != totalSize) {
                Toast.makeText(mActivity, "Error downloading", Toast.LENGTH_LONG).show();
            }
            setRequestedOrientation(mOldOrientation);
            checkIfReady();
        }

        protected void onProgressUpdate(Long... pos) {
            progress.setProgress((int) (pos[1] / progressScale));
            progress.setMessage("Downloading file " + (pos[0] + 1) + " of " + mFiles.length);
            progress.show();
        }

    }

    static class XZDataFile {
        String url;
        String filename;
        long uncompressedSize;

        public XZDataFile(String url, String filename, long uncompressedSize) {
            this.url = url;
            this.filename = filename;
            this.uncompressedSize = uncompressedSize;
        }
    }
}


