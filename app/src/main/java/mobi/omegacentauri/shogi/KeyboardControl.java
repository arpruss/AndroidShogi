package mobi.omegacentauri.shogi;

// TODO: watch for leaks in case views change!

import android.os.SystemClock;
import android.util.Log;
import android.view.KeyEvent;
import android.view.MotionEvent;
import android.view.View;

import java.util.ArrayList;
import java.util.List;

// TODO: ignore invisible views

public class KeyboardControl {
    public long mTouchedAt = -1;

    CursorPosition mCurrent = null;
    ArrayList<CursorPosition> mPositions;

    public KeyboardControl() {
        mPositions = new ArrayList<CursorPosition>();
    }

    public void clearTouch() {
        if (mCurrent != null && mTouchedAt >= 0) {
            if (mCurrent.mTouch != null) {
                MotionEvent m = MotionEvent.obtain(mTouchedAt, SystemClock.uptimeMillis(), MotionEvent.ACTION_UP, mCurrent.mX, mCurrent.mY, 0);
                mCurrent.mTouch.onTouch(mCurrent.mView, m);
            }
            hide();
        }
        mTouchedAt = -1;
    }

    public void press() {
        if (mTouchedAt >= 0) {
            clearTouch();
        }
        else if (mCurrent != null) {
            if (mCurrent.mTouch != null) {
                mTouchedAt = SystemClock.uptimeMillis();
                MotionEvent m = MotionEvent.obtain(mTouchedAt, SystemClock.uptimeMillis(), MotionEvent.ACTION_DOWN, mCurrent.mX, mCurrent.mY, 0);
                mCurrent.mTouch.onTouch(mCurrent.mView, m);
            }
            else if (mCurrent.mView != null)
                mCurrent.mView.performClick();
        }
    }

    public void clear() {
        mPositions.clear();
    }

    public void clearForView(View w) {
        if (mCurrent != null && mCurrent.mView == w) {
            hide();
            if (mTouchedAt >= 0) {
                MotionEvent m = MotionEvent.obtain(mTouchedAt, SystemClock.uptimeMillis(), MotionEvent.ACTION_CANCEL, mCurrent.mX, mCurrent.mY, 0);
                mTouchedAt = -1;
                mCurrent.mTouch.onTouch(mCurrent.mView, m);
            }
            mCurrent = null;
        }
        for (int i = mPositions.size() - 1 ; i >= 0 ; i--) {
            if (mPositions.get(i).mView == w)
                mPositions.remove(i);
        }
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
            if (cp.getY() == ny) {
                int x = cp.getX();
                if (x > nx)
                    bestNX = Math.min(bestNX, x);
            }
        }
        if (bestNX == Integer.MAX_VALUE)
            for (CursorPosition cp : mPositions) {
                int x = cp.getX();
                if (x > nx)
                    bestNX = Math.min(bestNX, x);
            }
        if (bestNX == Integer.MAX_VALUE)
            return;
        closest(bestNX, mCurrent.getY(), 0);
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
            if (cp.getY() == ny) {
                int x = cp.getX();
                if (x < nx)
                    bestNX = Math.max(bestNX, x);
            }
        }
        if (bestNX == Integer.MIN_VALUE)
            for (CursorPosition cp : mPositions) {
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
            if (cp.getX() == nx) {
                int y = cp.getY();
                if (y > ny)
                    bestNY = Math.min(bestNY, y);
            }
        }
        if (bestNY == Integer.MAX_VALUE)
            for (CursorPosition cp : mPositions) {
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
            if (nx == cp.getX()) {
                int y = cp.getY();
                if (y < ny)
                    bestNY = Math.max(bestNY, y);
            }
        }
        if (bestNY == Integer.MIN_VALUE)
            for (CursorPosition cp : mPositions) {
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

        Log.v("shogilog", "bestXY "+best.getX()+" "+best.getY());

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
    }

    public void center() {
        hide();
        int minX = Integer.MAX_VALUE;
        int minY = Integer.MAX_VALUE;
        int maxX = Integer.MIN_VALUE;
        int maxY = Integer.MIN_VALUE;
        for (CursorPosition cp : mPositions) {
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
            double d = Math.hypot(cp.getX()-cx, cp.getY()-cy);
            if (d < bestDistance) {
                bestDistance = d;
                best = cp;
            }
        }
        mCurrent = best;
        show();
    }

    private void show() {
        if (mCurrent != null) {
            if (mCurrent.mShow != null)
                mCurrent.mShow.showCursor(mCurrent);
            if (mCurrent.mView != null) {
                mCurrent.mView.requestFocus();
            }
        }
    }

    private void hide() {
        if (mCurrent != null) {
            if (mCurrent.mShow != null)
                mCurrent.mShow.hideCursor();
            if (mCurrent.mView != null)
                mCurrent.mView.clearFocus();
        }
    }

    public boolean onKeyDown(int keyCode, KeyEvent event) {

        Log.v("shogilog", "key "+keyCode);
        switch (keyCode) {
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
        }
        return false;
    }

    public void reset() {
        hide();
        clearTouch();
        mCurrent = null;
    }

    static public class CursorPosition {
        private final View mView;
        private final ShowCursor mShow;
        private final View.OnTouchListener mTouch;
        public final int mX;
        public final int mY;
        public final Object mExtras;

        public CursorPosition(View view, ShowCursor show, View.OnTouchListener touch, int x, int y, Object extras) {
            mView = view;
            mShow = show;
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

    public interface ShowCursor {
        public void showCursor(CursorPosition cp);
        public void hideCursor();
    }
}
