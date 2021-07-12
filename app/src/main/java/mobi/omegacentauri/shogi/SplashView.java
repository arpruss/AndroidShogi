package mobi.omegacentauri.shogi;

import android.content.Context;
import android.content.SharedPreferences;
import android.content.res.Resources;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.graphics.Canvas;
import android.graphics.LightingColorFilter;
import android.graphics.Matrix;
import android.graphics.Paint;
import android.graphics.Paint.Style;
import android.graphics.RadialGradient;
import android.graphics.Rect;
import android.graphics.RectF;
import android.graphics.Shader;
import android.graphics.Typeface;
import android.graphics.drawable.BitmapDrawable;
import android.preference.PreferenceManager;
import android.util.AttributeSet;
import android.util.Log;
import android.util.TypedValue;
import android.view.MotionEvent;
import android.view.View;
import android.widget.FrameLayout;

import java.util.ArrayList;

public class SplashView extends View {
    static public final String TAG = "SplashView";

    private final Context mContext;
    private final SharedPreferences mPrefs;
    private static final String pieces[][] = new String[][] {
                    {"hi", "kaku"},
                    {"ryu", "uma"}
    };

    public SplashView(Context context, AttributeSet attrs) {
        super(context, attrs);
        Log.v("splash", "init");
        mContext = context;
        mPrefs = PreferenceManager.getDefaultSharedPreferences(context);
    }


    //
    // Screen drawing
    //
    @Override
    public void onDraw(Canvas canvas) {
        Log.v("splash", "canvas "+canvas.getWidth()+" "+canvas.getHeight());
        final String prefix = mPrefs.getString("piece_style", "kanji_light_threedim");
        final String boardName = mPrefs.getString("board", "board_rich_brown");
        Paint p = new Paint();
        Rect boardRect = new Rect(0,0,canvas.getWidth(),canvas.getHeight());

        if (boardName.equals("plain")) {
            p.setColor(BoardView.BOARD_PLAIN_COLOR);
            canvas.drawRect(boardRect, p);
        }
        else {
            int id = BoardView.getBoardDrawable(mContext, boardName);
            if (id == 0) {
                p.setColor(BoardView.BOARD_PLAIN_COLOR);
                canvas.drawRect(boardRect, p);
            }
            else {
                Bitmap b = Bitmap.createScaledBitmap(BitmapFactory.decodeResource(getResources(), id), boardRect.width(), boardRect.height(), true);
                canvas.drawBitmap(b, 0,0, p);
                b.recycle();
            }
        }
        int pieceDim = (int)(boardRect.width()*0.6f/2f);
        int x = (int)(boardRect.centerX()-pieceDim);
        int y = (int)(boardRect.centerY()-pieceDim);

        Resources r = getResources();

        for (int i=0;i<2;i++)
            for (int j=0;j<2;j++) {
                Rect pieceRect = new Rect(x+pieceDim*i, y+pieceDim*j, x+pieceDim*(i+1), y+pieceDim*(j+1));
                int id = r.getIdentifier(String.format("@mobi.omegacentauri.shogi:drawable/%s_%s", prefix, pieces[i][j]), null, null);
                if (id != 0) {
                    Bitmap piece = BitmapFactory.decodeResource(r, id);
                    canvas.drawBitmap(piece, null, pieceRect, null);
                }
            }
    }


    @Override protected void onMeasure (int widthMeasureSpec, int heightMeasureSpec) {
        int originalWidth = MeasureSpec.getSize(widthMeasureSpec);
        int originalHeight = MeasureSpec.getSize(heightMeasureSpec);
        int dimension = Math.min(originalWidth, originalHeight);
        Log.v("splash", "dim "+dimension);
        super.onMeasure(
                MeasureSpec.makeMeasureSpec(dimension, MeasureSpec.EXACTLY),
                MeasureSpec.makeMeasureSpec(dimension, MeasureSpec.EXACTLY));
    }
}
