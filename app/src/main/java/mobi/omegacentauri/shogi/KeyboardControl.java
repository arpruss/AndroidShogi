package mobi.omegacentauri.shogi;

// TODO: watch for leaks in case views change!

import android.os.SystemClock;
import android.util.Log;
import android.view.HapticFeedbackConstants;
import android.view.KeyEvent;
import android.view.MotionEvent;
import android.view.View;

import java.util.ArrayList;

// TODO: ignore invisible views

public class KeyboardControl {
    public long mTouchedAt = -1;

    CursorPosition mCurrent = null;
    CursorPosition mStartMove = null;
    ArrayList<CursorPosition> mPositions;
    static final KeyboardRespondent defaultRespondent = new KeyboardRespondent() {
        @Override
        public void showCursor(CursorPosition cp) {
        }

        @Override
        public void hideCursor() {
        }

        @Override
        public boolean isValid(CursorPosition cp) {
            try {
                return cp.mView.getVisibility() == View.VISIBLE;
            }
            catch(Exception e) {
                return false;
            }
        }
    };

    public KeyboardControl() {
        mPositions = new ArrayList<CursorPosition>();
    }

    public void clearTouch() {
        if (mCurrent != null && mTouchedAt >= 0) {
            if (mCurrent.mTouch != null) {
                Log.v("shogilog", "Clearing touch");
                MotionEvent m = MotionEvent.obtain(mTouchedAt, SystemClock.uptimeMillis(), MotionEvent.ACTION_CANCEL, mCurrent.mX, mCurrent.mY, 0);
                mCurrent.mTouch.onTouch(mCurrent.mView, m);
            }
            hide();
        }
        mTouchedAt = -1;
        mStartMove = null;
    }

    public void press() {
        if (mCurrent != null)
            mCurrent.mView.performHapticFeedback(HapticFeedbackConstants.VIRTUAL_KEY);
        if (mTouchedAt >= 0) {
            if (mCurrent != null && mCurrent.mTouch != null) {
                MotionEvent m = MotionEvent.obtain(mTouchedAt, SystemClock.uptimeMillis(), MotionEvent.ACTION_UP, mCurrent.mX, mCurrent.mY, 0);
                mCurrent.mTouch.onTouch(mCurrent.mView, m);
                mStartMove = null;
                mTouchedAt = -1;
            }
        }
        else if (mCurrent != null) {
            if (mCurrent.mTouch != null) {
                Log.v("shogilog", "pressing");
                mTouchedAt = SystemClock.uptimeMillis();
                MotionEvent m = MotionEvent.obtain(mTouchedAt, SystemClock.uptimeMillis(), MotionEvent.ACTION_DOWN, mCurrent.mX, mCurrent.mY, 0);
                mCurrent.mTouch.onTouch(mCurrent.mView, m);
                mStartMove = mCurrent;
            }
            else
                mCurrent.mView.performClick();
        }
    }

    public void clear() {
        mPositions.clear();
        mStartMove = null;
        mCurrent = null;
    }

    public void clearForView(View w) {
        if (mCurrent != null && mCurrent.mView == w) {
            hide();
            if (mTouchedAt >= 0) {
                MotionEvent m = MotionEvent.obtain(mTouchedAt, SystemClock.uptimeMillis(), MotionEvent.ACTION_CANCEL, mCurrent.mX, mCurrent.mY, 0);
                mTouchedAt = -1;
                mStartMove = null;
                mCurrent.mTouch.onTouch(mCurrent.mView, m);
            }
            mCurrent = null;
        }
        for (int i = mPositions.size() - 1 ; i >= 0 ; i--) {
            if (mPositions.get(i).mView == w)
                mPositions.remove(i);
        }
        if (mStartMove != null && mStartMove.mView == w)
            mStartMove = null;
    }

    public void add(CursorPosition cp) {
        mPositions.add(cp);
    }

    public void right() {
        if (mCurrent == null) {
            center();
            return;
        }
        int nx = mCurrent.getX();
        int ny = mCurrent.getY();
        int bestNX = Integer.MAX_VALUE;
        for (CursorPosition cp : mPositions) {
            if (! cp.mRespondent.isValid(cp))
                continue;
            if (cp.getY() == ny) {
                int x = cp.getX();
                if (x > nx)
                    bestNX = Math.min(bestNX, x);
            }
        }
        if (bestNX == Integer.MAX_VALUE)
            for (CursorPosition cp : mPositions) {
                if (! cp.mRespondent.isValid(cp))
                    continue;
                int x = cp.getX();
                if (x > nx)
                    bestNX = Math.min(bestNX, x);
            }
        if (bestNX == Integer.MAX_VALUE)
            return;
        closest(bestNX, mCurrent.getY(), 0);
        if (mCurrent != null)
            mCurrent.mView.performHapticFeedback(HapticFeedbackConstants.VIRTUAL_KEY);
    }

    public void left() {
        if (mCurrent == null) {
            center();
            return;
        }
        int nx = mCurrent.getX();
        int ny = mCurrent.getY();
        int bestNX = Integer.MIN_VALUE;
        for (CursorPosition cp : mPositions) {
            if (! cp.mRespondent.isValid(cp))
                continue;
            if (cp.getY() == ny) {
                int x = cp.getX();
                if (x < nx)
                    bestNX = Math.max(bestNX, x);
            }
        }
        if (bestNX == Integer.MIN_VALUE)
            for (CursorPosition cp : mPositions) {
                if (! cp.mRespondent.isValid(cp))
                    continue;
                int x = cp.getX();
                if (x < nx)
                    bestNX = Math.max(bestNX, x);
            }
        if (bestNX == Integer.MIN_VALUE)
            return;
        closest(bestNX, mCurrent.getY(), 0);
    }

    public void down() {
        if (mCurrent == null) {
            center();
            return;
        }
        int nx = mCurrent.getX();
        int ny = mCurrent.getY();
        int bestNY = Integer.MAX_VALUE;
        for (CursorPosition cp : mPositions) {
            if (! cp.mRespondent.isValid(cp))
                continue;
            if (cp.getX() == nx) {
                int y = cp.getY();
                if (y > ny)
                    bestNY = Math.min(bestNY, y);
            }
        }
        if (bestNY == Integer.MAX_VALUE)
            for (CursorPosition cp : mPositions) {
                if (! cp.mRespondent.isValid(cp))
                    continue;
                int y = cp.getY();
                if (y > ny)
                    bestNY = Math.min(bestNY, y);
            }
        if (bestNY == Integer.MAX_VALUE)
            return;
        closest(mCurrent.getX(), bestNY, 1);
    }

    public void up() {
        if (mCurrent == null) {
            center();
            return;
        }
        int ny = mCurrent.getY();
        int nx = mCurrent.getX();
        int bestNY = Integer.MIN_VALUE;
        for (CursorPosition cp : mPositions) {
            if (! cp.mRespondent.isValid(cp))
                continue;
            if (nx == cp.getX()) {
                int y = cp.getY();
                if (y < ny)
                    bestNY = Math.max(bestNY, y);
            }
        }
        if (bestNY == Integer.MIN_VALUE)
            for (CursorPosition cp : mPositions) {
                if (! cp.mRespondent.isValid(cp))
                    continue;
                int y = cp.getY();
                if (y < ny)
                    bestNY = Math.max(bestNY, y);
            }
        if (bestNY == Integer.MIN_VALUE)
            return;
        closest(mCurrent.getX(), bestNY, 1);
    }

    private void closest(int x, int y, int coordinateToMove) {
        hide();
        int bestDistance = Integer.MAX_VALUE;
        CursorPosition best = null;
        for (CursorPosition cp : mPositions) {
            if (! cp.mRespondent.isValid(cp))
                continue;
            if (coordinateToMove == 0) {
                int cpX = cp.getX();
                int dY = Math.abs(y-cp.getY());
                if (cpX == x && dY <= bestDistance) {
                    best = cp;
                    bestDistance = dY;
                }
            }
            else {
                int cpY = cp.getY();
                int dX = Math.abs(x-cp.getX());
                if (cpY == y && dX <= bestDistance) {
                    best = cp;
                    bestDistance = dX;
                }
            }
        }

        if (mCurrent != best && best != null && mCurrent != null && mTouchedAt >= 0) {
            MotionEvent m;
            if (mCurrent.mView == best.mView) {
                m = MotionEvent.obtain(mTouchedAt, SystemClock.uptimeMillis(), MotionEvent.ACTION_MOVE, best.mX, best.mY, 0);
            }
            else {
                m = MotionEvent.obtain(mTouchedAt, SystemClock.uptimeMillis(), MotionEvent.ACTION_CANCEL, mCurrent.mX, mCurrent.mY, 0);
                mTouchedAt = -1;
            }
            mCurrent.mTouch.onTouch(mCurrent.mView, m);
        }
        if (best != null)
            mCurrent = best;

        show();
        if (mCurrent != null)
            mCurrent.mView.performHapticFeedback(HapticFeedbackConstants.VIRTUAL_KEY);
    }

    public void nextView() {
        if (mCurrent == null) {
            center();
            return;
        }
        clearTouch();
        hide();
        int n = mPositions.size();
        for (int i=0; i<n; i++) {
            if (mPositions.get(i) == mCurrent) {
                for (int j=1; j<=mPositions.size(); j++) {
                    CursorPosition cp = mPositions.get((i+j)%n);
                    if (cp.mRespondent.isValid(cp) && cp.mView != mCurrent.mView) {
                        mCurrent = cp;
                        mCurrent.mView.performHapticFeedback(HapticFeedbackConstants.VIRTUAL_KEY);
                        show();
                        return;
                    }
                }
            }
        }
    }

    public void center() {
        hide();
        int minX = Integer.MAX_VALUE;
        int minY = Integer.MAX_VALUE;
        int maxX = Integer.MIN_VALUE;
        int maxY = Integer.MIN_VALUE;
        for (CursorPosition cp : mPositions) {
            if (! cp.mRespondent.isValid(cp))
                continue;
            int x = cp.getX();
            int y = cp.getY();
            minX = Math.min(x,minX);
            minY = Math.min(y,minY);
            maxX = Math.max(x,maxX);
            maxY = Math.max(y,maxY);
        }
        CursorPosition best = null;
        double cx = (minX + maxX)/2.;
        double cy = (minY + maxY)/2.;
        double bestDistance = Float.MAX_VALUE;
        for (CursorPosition cp : mPositions) {
            if (! cp.mRespondent.isValid(cp))
                continue;
            double d = Math.hypot(cp.getX()-cx, cp.getY()-cy);
            if (d < bestDistance) {
                bestDistance = d;
                best = cp;
            }
        }
        mCurrent = best;
        show();
        if (mCurrent != null)
            mCurrent.mView.performHapticFeedback(HapticFeedbackConstants.VIRTUAL_KEY);
    }

    private void show() {
        if (mCurrent != null) {
            mCurrent.mRespondent.showCursor(mCurrent);
            mCurrent.mView.requestFocus();
        }
    }

    private void hide() {
        if (mCurrent != null) {
            mCurrent.mRespondent.hideCursor();
            mCurrent.mView.clearFocus();
        }
    }

    public boolean onKeyDown(int keyCode, KeyEvent event) {

        switch (keyCode) {
            case KeyEvent.KEYCODE_DPAD_DOWN_LEFT:
                down();
                left();
                return true;
            case KeyEvent.KEYCODE_DPAD_DOWN_RIGHT:
                down();
                right();
                return true;
            case KeyEvent.KEYCODE_DPAD_UP_LEFT:
                up();
                left();
                return true;
            case KeyEvent.KEYCODE_DPAD_UP_RIGHT:
                up();
                right();
                return true;
            case KeyEvent.KEYCODE_DPAD_LEFT:
                left();
                return true;
            case KeyEvent.KEYCODE_DPAD_RIGHT:
                right();
                return true;
            case KeyEvent.KEYCODE_DPAD_UP:
                up();
                return true;
            case KeyEvent.KEYCODE_DPAD_DOWN:
                down();
                return true;
            case KeyEvent.KEYCODE_DPAD_CENTER:
            case KeyEvent.KEYCODE_ENTER:
                press();
                return true;
            case KeyEvent.KEYCODE_ESCAPE:
                cancel();
                return true;
            case KeyEvent.KEYCODE_TAB:
                nextView();
                return true;
        }
        return false;
    }

    private void cancel() {
        if (mCurrent != null && mTouchedAt >= 0) {
            if (mCurrent.mTouch != null) {
                MotionEvent m = MotionEvent.obtain(mTouchedAt, SystemClock.uptimeMillis(), MotionEvent.ACTION_CANCEL, mCurrent.mX, mCurrent.mY, 0);
                mCurrent.mTouch.onTouch(mCurrent.mView, m);
            }
            mTouchedAt = -1;
            hide();
            mCurrent = mStartMove;
            mStartMove = null;
            show();
        }
    }

    public void reset() {
        hide();
        clearTouch();
        mCurrent = null;
    }

    static public class CursorPosition {
        private final View mView;
        private final KeyboardRespondent mRespondent;
        private final View.OnTouchListener mTouch;
        public final int mX;
        public final int mY;
        public final Object mExtras;

        public CursorPosition(View view, KeyboardRespondent respondent, View.OnTouchListener touch, int x, int y, Object extras) {
            mView = view;
            mRespondent = respondent == null ? defaultRespondent : respondent;
            mTouch = touch;
            mX = x;
            mY = y;
            mExtras = extras;
        }

        public int getX() {
            int[] pos = new int[2];
            mView.getLocationInWindow(pos);
            return pos[0] + mX;
        }

        public int getY() {
            int[] pos = new int[2];
            mView.getLocationInWindow(pos);
            return pos[1] + mY;
        }
    }

    public interface KeyboardRespondent {
        public void showCursor(CursorPosition cp);
        public void hideCursor();
        public boolean isValid(CursorPosition cp);
    }
}
