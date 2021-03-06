package mobi.omegacentauri.shogi;

import java.util.ArrayList;

import android.content.Context;
import android.content.SharedPreferences;
import android.content.res.Resources;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.graphics.Canvas;
import android.graphics.ColorFilter;
import android.graphics.LightingColorFilter;
import android.graphics.Matrix;
import android.graphics.Paint;
import android.graphics.Paint.Style;
import android.graphics.RadialGradient;
import android.graphics.Rect;
import android.graphics.Shader;
import android.graphics.Typeface;
import android.graphics.drawable.BitmapDrawable;
import android.os.SystemClock;
import android.preference.PreferenceManager;
import android.util.AttributeSet;
import android.util.Log;
import android.util.TypedValue;
import android.view.MotionEvent;
import android.view.View;
import android.widget.FrameLayout;

public class BoardView extends FrameLayout implements View.OnTouchListener, KeyboardControl.KeyboardRespondent {
    static public final String TAG = "ShogiView";

    static final int CHALLENGING_KING = Piece.NUM_TYPES;
    public static final int BOARD_PLAIN_COLOR = 0xfff5deb3;

    static String mBoardName;
    private final Context mContext;
    private final SharedPreferences mPrefs;
    private boolean mExactPosition = true;
    private XY mCursor = null;
    private int[] fingerColors = new int[] {
            0x00008000,
            0x00008000,
            0xC0008000,
            0x00008000
    };
    private float[] fingerStops = new float[] {
        0f, .6f, .8f, 1f
    };
    private KeyboardControl mKeyboardControl;

    @Override
    public void showCursor(KeyboardControl.CursorPosition cp) {
        mCursor = (XY)cp.mExtras;
        invalidate();
    }

    @Override
    public void hideCursor() {
        mCursor = null;
        invalidate();
    }

    @Override
    public boolean isValid(KeyboardControl.CursorPosition cp) {
        XY xy = (XY)cp.mExtras;
        if (xy.y == XY.BLACK_CAPTURED) {
            return mCurrentPlayer.equals(Player.BLACK) && isHumanPlayer(Player.BLACK) &&
                xy.x < listCapturedPieces(mBoard, getScreenLayout(), Player.BLACK).size();
        }
        else if (xy.y == XY.WHITE_CAPTURED) {
            return mCurrentPlayer.equals(Player.WHITE) && isHumanPlayer(Player.WHITE) &&
                    xy.x < listCapturedPieces(mBoard, getScreenLayout(), Player.WHITE).size();
        }
        return true;
    }

    /**
     * Interface for communicating user moves to the owner of this view.
     * onHumanMove is called when a human player moves a piece, or drops a
     * captured piece.
     */
    public interface EventListener {
        void onHumanPlay(Player player, Play p);
    }

    public BoardView(Context context, AttributeSet attrs) {
        super(context, attrs);
        mContext = context;
        mPrefs = PreferenceManager.getDefaultSharedPreferences(context);
        mCurrentPlayer = Player.INVALID;
        mBoard = new Board();
        setOnTouchListener(this);
        mBoardName = mPrefs.getString("board", ShogiPreferenceActivity.DEFAULT_BOARD);
        mExactPosition = !mPrefs.getBoolean("fuzzy", false);
    }

    /**
     * Called once during object construction.
     */
    public final void initialize(
            EventListener listener,
            ArrayList<Player> humanPlayers,
            boolean flipScreen,
            KeyboardControl keyboardControl) {
        mListener = listener;
        mHumanPlayers = new ArrayList<Player>(humanPlayers);
        mFlipped = flipScreen;
        mKeyboardControl = keyboardControl;
    }

    /**
     * Flip the screen upside down.
     */
    public final void setFlipScreen(boolean value) {
        mFlipped = value;
        invalidate();
    }

    /**
     * Update the state of the board as well as players' turn.
     *
     * @param lastBoard     The previous state of the board
     * @param board         The new state of the board
     * @param currentPlayer The next player that's allowed to move a piece next.
     *                      Note that this may not be equal to the player with the next turn. Rather, it is
     *                      the player that can touch the screen
     *                      and make a move. Thus, for a computer-vs-computer game,
     *                      currentPlayer will always be Player.INVALID.
     */
    public final void update(
            GameState gameState,
            Board lastBoard,
            Board board,
            Player currentPlayer,
            Play lastMove,
            boolean animateMove) {

        animateMove = animateMove && mPrefs.getBoolean("animate", true);

        mCurrentPlayer = currentPlayer;
        mLastBoard = null;
        if (lastBoard != null) mLastBoard = new Board(lastBoard);
        mBoard = new Board(board);

        mLastMove = lastMove;
        mAnimationStartTime = mNextAnimationTime = -1;
        if (animateMove) {
            mAnimationStartTime = mNextAnimationTime = SystemClock.uptimeMillis();
        }
        invalidate();
    }

    //
    // onTouch processing
    //
    private static final int S_INVALID = 0;
    private static final int S_PIECE = 1;
    private static final int S_CAPTURED = 2;
    private static final int S_MOVE_DESTINATION = 3;

    /**
     * Helper class for finding a piece that the user is intending to move.
     */
    private class NearestSquareFinder {
        private final int mSquareDim;
        private final boolean mExactPosition;
        private ScreenLayout mLayout;

        // Screen pixel position of the touch event
        private float mSx;
        private float mSy;

        private double mMinDistance;
        private int mPx;
        private int mPy;
        private int mType;

        // sx and sy are screen location of the touch event, in pixels.
        public NearestSquareFinder(ScreenLayout layout, float sx, float sy, boolean exactPosition) {
            mLayout = layout;
            mSx = sx;
            mSy = sy;
            mExactPosition = exactPosition;

            mMinDistance = 9999;
            mPx = mPy = -1;
            mType = S_INVALID;
            mSquareDim = mLayout.getSquareDim();
        }

        // Find empty spots near <mSx, mSy> at which "player" can drop
        // "pieceToDrop". Remember the best spot in <mPx, mPy, mType>.
        public final void findNearestEmptySquareOnBoard(
                Board board, Player player, int pieceToDrop) {
            int px = mLayout.boardX(mSx);
            int py = mLayout.boardY(mSy);
            int delta = mExactPosition ? 0 : 1;
            for (int i = -delta; i <= delta; ++i) {
                for (int j = -delta; j <= delta; ++j) {
                    int x = px + i;
                    int y = py + j;
                    if (x >= 0 && x < Board.DIM && y >= 0 && y < Board.DIM) {
                        int piece = board.getPiece(x, y);
                        if (piece == 0) {
                            if (!isDoublePawn(board, pieceToDrop, x, y)) {
                                tryScreenPosition(mLayout.screenX(x), mLayout.screenY(y), x, y, S_PIECE);
                            }
                        }
                    }
                }
            }
        }

        // Find a piece that's near <mSx, mSy>, owned by "player", and can move elsewhere.
        // Remember the best piece in <mPx, mPy>.
        public final void findNearestPlayersPieceOnBoard(Board board, Player player) {
            int px = mLayout.boardX(mSx);
            int py = mLayout.boardY(mSy);
            int delta = mExactPosition ? 0 : 1;
            for (int i = -delta; i <= delta; ++i) {
                for (int j = -delta; j <= delta; ++j) {
                    int x = px + i;
                    int y = py + j;
                    if (x >= 0 && x < Board.DIM && y >= 0 && y < Board.DIM) {
                        if (Board.player(board.getPiece(x, y)) == player) {
                            ArrayList<Board.Position> dests = board.possibleMoveDestinations(x, y);
                            if (!dests.isEmpty()) {
                                tryScreenPosition(mLayout.screenX(x), mLayout.screenY(y), x, y, S_PIECE);
                            }
                        }
                    }
                }
            }
        }

        private final void tryScreenPosition(float sx, float sy,
                                             int px, int py, int type) {
            float centerX = sx + mSquareDim / 2;
            float centerY = sy + mSquareDim / 2;

            if (mExactPosition && (Math.abs(centerX - mSx) >= mSquareDim/2 || Math.abs(centerY - mSy) >= mSquareDim/2) ) {
                return;
            }

            double distance = Math.hypot(centerX - mSx, centerY - mSy);

            if (distance < mMinDistance && distance < mSquareDim) {
                mMinDistance = distance;
                mPx = px;
                mPy = py;
                mType = type;
            }
        }

        private final boolean isDoublePawn(Board board, int pieceToDrop, int px, int py) {
            if (Board.type(pieceToDrop) == Piece.FU) {
                for (int y = 0; y < Board.DIM; ++y) {
                    if (y != py && board.getPiece(px, y) == pieceToDrop) return true;
                }
            }
            return false;
        }

        public final int nearestType() {
            return mType;
        }

        public final int nearestX() {
            return mPx;
        }

        public final int nearestY() {
            return mPy;
        }

        public boolean checkHysteresis(PositionOnBoard moveTo) {
            if (moveTo != null && moveTo instanceof PositionOnBoard) {
                float centerMoveX = mLayout.screenX(moveTo.x) + mSquareDim / 2;
                float centerMoveY = mLayout.screenY(moveTo.y) + mSquareDim / 2;
                float distanceFromMove = Math.max(Math.abs(centerMoveX - mSx), Math.abs(centerMoveY - mSy));
                float threshold = 0.67f * mSquareDim;

                return distanceFromMove < threshold;
            }
            return false;
        }
    }

    public boolean onTouch(View v, MotionEvent event) {
        if (!isHumanPlayer(mCurrentPlayer)) return false;

        ScreenLayout layout = getScreenLayout();

        int action = event.getAction();

        if (action == MotionEvent.ACTION_DOWN) {
            // Start of touch operation
            mMoveFrom = null;
            mMoveTo = null;

            NearestSquareFinder finder = new NearestSquareFinder(layout, event.getX(), event.getY(), mExactPosition);
            finder.findNearestPlayersPieceOnBoard(mBoard, mCurrentPlayer);

            ArrayList<CapturedPiece> captured = listCapturedPieces(mBoard, layout, mCurrentPlayer);
            for (int i = 0; i < captured.size(); ++i) {
                CapturedPiece cp = captured.get(i);
                finder.tryScreenPosition(cp.sx, cp.sy, i, -1, S_CAPTURED);
            }
            if (finder.nearestType() == S_PIECE) {
                mMoveFrom = new PositionOnBoard(finder.nearestX(), finder.nearestY());
                mMoveTo = new PositionOnBoard(finder.nearestX(), finder.nearestY());
            } else if (finder.nearestType() == S_CAPTURED) {
                // Dropping a captured piece
                mMoveFrom = captured.get(finder.nearestX());
            } else {
                return false;
            }
            invalidate();
            return true;
        }

        boolean needInvalidation = false;
        if (mMoveFrom != null) {
            // User dragging a piece to move
            NearestSquareFinder finder = new NearestSquareFinder(layout, event.getX(), event.getY(), mExactPosition);
            if (mMoveFrom instanceof PositionOnBoard) {
                PositionOnBoard from = (PositionOnBoard) mMoveFrom;
                ArrayList<Board.Position> dests = mBoard.possibleMoveDestinations(from.x, from.y);
                for (Board.Position d : dests) {
                    finder.tryScreenPosition(
                            layout.screenX(d.x), layout.screenY(d.y),
                            d.x, d.y, S_MOVE_DESTINATION);
                }
                // Allow moving to the origin point to nullify the move.
                finder.tryScreenPosition(
                        layout.screenX(from.x), layout.screenY(from.y), from.x, from.y, S_MOVE_DESTINATION);
            } else {
                CapturedPiece p = (CapturedPiece) mMoveFrom;
                finder.findNearestEmptySquareOnBoard(mBoard, mCurrentPlayer, p.piece);
            }

            if (!finder.checkHysteresis(mMoveTo)) {
                if (finder.nearestType() != S_INVALID) {
                    final PositionOnBoard to = new PositionOnBoard(finder.nearestX(), finder.nearestY());
                    needInvalidation = !to.equals(mMoveTo);
                    mMoveTo = to;
                } else {
                    needInvalidation = (mMoveTo != null);
                    mMoveTo = null;
                }
            }
        }
        if (action == MotionEvent.ACTION_UP) {
            if (mMoveTo != null && !mMoveFrom.equals(mMoveTo)) {
                Play move = null;
                if (mMoveFrom instanceof PositionOnBoard) {
                    PositionOnBoard from = (PositionOnBoard) mMoveFrom;
                    move = new Play(
                            mBoard.getPiece(from.x, from.y),
                            from.x, from.y,
                            mMoveTo.x, mMoveTo.y);
                } else {
                    move = new Play(
                            ((CapturedPiece) mMoveFrom).piece,
                            -1, -1,
                            mMoveTo.x, mMoveTo.y);
                }
                mListener.onHumanPlay(mCurrentPlayer, move);
                mCurrentPlayer = Player.INVALID;
            }
            mMoveTo = null;
            mMoveFrom = null;
            needInvalidation = true;
        }

        if (action == MotionEvent.ACTION_CANCEL) {
            mMoveTo = null;
            mMoveFrom = null;
            needInvalidation = true;
        }
        if (needInvalidation) invalidate();
        return true;
    }

    private final class XY {
        int x;
        int y;
        static final int BLACK_CAPTURED = -1;
        static final int WHITE_CAPTURED = Board.DIM;

        public XY(int x, int y) {
            this.x = x;
            this.y = y;
        }

        float getScreenX() {
            if (this.y == BLACK_CAPTURED) {
                return getScreenLayout().capturedScreenX(Player.BLACK, this.x) + getScreenLayout().getSquareDim() / 2f;
            }
            else if (this.y == WHITE_CAPTURED) {
                return getScreenLayout().capturedScreenX(Player.WHITE, this.x) + getScreenLayout().getSquareDim() / 2f;
            }
            else {
                return getScreenLayout().screenX(x) + getScreenLayout().getSquareDim() / 2f;
            }
        }

        float getScreenY() {
            if (this.y == BLACK_CAPTURED) {
                return getScreenLayout().capturedScreenY(Player.BLACK, this.x) + getScreenLayout().getSquareDim() / 2f;
            }
            else if (this.y == WHITE_CAPTURED) {
                return getScreenLayout().capturedScreenY(Player.WHITE, this.x) + getScreenLayout().getSquareDim() / 2f;
            }
            else {
                return getScreenLayout().screenY(y) + getScreenLayout().getSquareDim() / 2f;
            }
        }
    }

    private void prepareGraphics(ScreenLayout layout) {
        Rect boardRect = layout.getBoard();
        if ( boardRect.width() == mBoardWidth &&
                boardRect.height() == mBoardHeight &&
                mFlipped == mBoardFlipped) {
            return;
        }

        initializePieceBitmaps(mContext);

        mBoardWidth = boardRect.width();
        mBoardHeight = boardRect.height();
        mBoardFlipped = mFlipped;

        if (mBoardBitmap != null) {
            mBoardBitmap.recycle();
            mBoardBitmap = null;
        }

        int id = mBoardName.equals("plain") ? 0 : getBoardDrawable(mContext, mBoardName);
        if (id == 0) {
            mBoardBitmap = null;
        }
        else {
            Bitmap base = BitmapFactory.decodeResource(getResources(), id);
            Matrix matrix = new Matrix();
            matrix.setScale((float)mBoardWidth/base.getWidth(),(float)mBoardHeight/base.getHeight());
            if (mFlipped) {
                matrix.postRotate(180);
            }

            mBoardBitmap = Bitmap.createBitmap(base, 0, 0, base.getWidth(), base.getHeight(), matrix, true);
        }

        if (mKeyboardControl != null) {
            mKeyboardControl.clearForView(this);
            for (int x=0; x<Board.DIM; x++)
                for (int y=0; y<Board.DIM; y++) {
                    XY xy = new XY(x,y);
                    mKeyboardControl.add(new KeyboardControl.CursorPosition(this,
                            this, this,
                            (int)(xy.getScreenX()), (int)(xy.getScreenY()),
                            xy));
                }


            for (int y=Math.min(XY.BLACK_CAPTURED,XY.WHITE_CAPTURED) ; y <= Math.max(XY.BLACK_CAPTURED,XY.WHITE_CAPTURED); y++) {
                /*if (y == XY.BLACK_CAPTURED && !isHumanPlayer(Player.BLACK))
                    continue;
                if (y == XY.WHITE_CAPTURED && !isHumanPlayer(Player.WHITE))
                    continue; */
                for (int x = 0; x < 7; x++) {
                    XY xy = new XY(x, y);
                    mKeyboardControl.add(new KeyboardControl.CursorPosition(this,
                            this, this,
                            (int) (xy.getScreenX()), (int) (xy.getScreenY()),
                            xy));
                }
            }
        }
    }

    private static final int ANIM_DRAW_LAST_BOARD = 1;
    private static final int ANIM_HIDE_PIECE_FROM = 2;
    private static final int ANIM_HIGHLIGHT_PIECE_TO = 8;

    public void drawSelectionCircle(Canvas canvas, float cx, float cy, float scale, ScreenLayout layout) {
        Paint cp = new Paint();
        float radius = layout.getSquareDim() * scale;
        cp.setShader(new RadialGradient(cx, cy, radius, fingerColors, fingerStops, Shader.TileMode.MIRROR));
        canvas.drawCircle(cx, cy, radius, cp);
    }

    //
    // Screen drawing
    //
    @Override
    public void onDraw(Canvas canvas) {
        if (mBoard == null) return;

        ScreenLayout layout = getScreenLayout();
        prepareGraphics(layout);

        int squareDim = layout.getSquareDim();

        drawEmptyBoard(canvas, layout);
        final long now = SystemClock.uptimeMillis();
        int animation = 0;
        double animationStage = -1;

        if (mLastMove != null && mNextAnimationTime >= 0 && mNextAnimationTime <= now) {
            animationStage = (double)(now - mAnimationStartTime) / ANIMATION_INTERVAL;
            int seq = (int) Math.floor(animationStage);
            switch (seq) {
                case 0:
                    animation |= ANIM_HIDE_PIECE_FROM | ANIM_DRAW_LAST_BOARD | ANIM_HIGHLIGHT_PIECE_TO;
                    break;
                case 1:
                    animation |= ANIM_HIGHLIGHT_PIECE_TO;
                    break;
//                case 1:
//                    animation |= ANIM_HIDE_PIECE_FROM | ANIM_DRAW_LAST_BOARD | ANIM_HIGHLIGHT_PIECE_TO;
//                    break;
/*                case 2:
                    animation |= ANIM_HIDE_PIECE_FROM | ANIM_DRAW_LAST_BOARD | ANIM_HIGHLIGHT_PIECE_TO;
                    break; */
            }
            if (animationStage < 0)
                animationStage = 0;
            if (animationStage > 1)
                animationStage = 1;
//            ++seq;
        }

        if (animation == 0 && mLastMove != null) {
            darkenSquare(canvas, layout, mLastMove.toX(), mLastMove.toY());
        }

        // Draw pieces
        final Board board = ((animation & ANIM_DRAW_LAST_BOARD) != 0 ? mLastBoard : mBoard);

        float fromScreenX = Integer.MIN_VALUE;
        float fromScreenY = Integer.MIN_VALUE;
        int movePiece = 0;
        Player movePlayer = Player.INVALID;

        if (animation != 0 && mLastMove != null) {
            movePiece = mLastMove.piece();
            movePlayer = movePiece < 0 ? Player.WHITE : Player.BLACK;

            if (mLastMove.isDroppingPiece()) {
                ArrayList<CapturedPiece> pieces = listCapturedPieces(board, layout, movePlayer);
                int index = -1;
                for (int i = 0; i < pieces.size() ; i++)
                    if (movePiece == pieces.get(i).piece) {
                        index = i;
                        break;
                    }
                if (index >= 0) {
                    fromScreenX = layout.capturedScreenX(movePlayer, index);
                    fromScreenY = layout.capturedScreenY(movePlayer, index);
                }
            }
            else {
                fromScreenX = layout.screenX(mLastMove.fromX());
                fromScreenY = layout.screenX(mLastMove.fromY());
            }
        }

        for (int y = 0; y < Board.DIM; ++y) {
            for (int x = 0; x < Board.DIM; ++x) {
                int piece = board.getPiece(x, y);
                if (piece == 0) continue;
                if ((animation & ANIM_HIDE_PIECE_FROM) != 0 && x == mLastMove.fromX() && y == mLastMove.fromY())
                    continue;
                int alpha = 255;

                float screenX = layout.screenX(x);
                float screenY = layout.screenY(y);

                // A piece that the user is trying to move will be draw with a bit of
                // transparency.
                if (mMoveFrom != null && mMoveFrom instanceof PositionOnBoard) {
                    PositionOnBoard p = (PositionOnBoard) mMoveFrom;
                    if (x == p.x && y == p.y) alpha = 64;
                }
                drawPiece(canvas, layout, piece, screenX, screenY, alpha);
            }
        }

        if ((animation & ANIM_HIDE_PIECE_FROM) != 0 && animationStage > 0 && Integer.MIN_VALUE != fromScreenX) {
            float screenX = (float) (layout.screenX(mLastMove.toX()) * animationStage + fromScreenX * (1-animationStage));
            float screenY = (float) (layout.screenY(mLastMove.toY()) * animationStage + fromScreenY * (1-animationStage));
            drawPiece(canvas, layout, movePiece, screenX, screenY, 255);
        }

        if ((animation & ANIM_HIGHLIGHT_PIECE_TO) != 0) {
            Paint cp = new Paint();
            float cx = layout.screenX(mLastMove.toX()) + squareDim / 2.0f;
            float cy = layout.screenY(mLastMove.toY()) + squareDim / 2.0f;
            float radius = squareDim * 0.9f;
            cp.setShader(new RadialGradient(cx, cy, squareDim * 0.2f, 0xffb8860b, 0x00b8860b, Shader.TileMode.MIRROR));
            canvas.drawCircle(cx, cy, radius, cp);
        }

        drawCapturedPieces(canvas, layout, Player.BLACK,
                ((movePlayer == Player.BLACK) && ((animation & ANIM_HIDE_PIECE_FROM) != 0)) ? movePiece : 0);
        drawCapturedPieces(canvas, layout, Player.WHITE,
                ((movePlayer == Player.WHITE) && ((animation & ANIM_HIDE_PIECE_FROM) != 0)) ? movePiece : 0);

        if (mMoveFrom != null) {
            if (mMoveFrom instanceof PositionOnBoard) {
                PositionOnBoard from = (PositionOnBoard) mMoveFrom;
                // Draw orange dots in each possible destination
                ArrayList<Board.Position> dests = mBoard.possibleMoveDestinations(from.x, from.y);

                Paint cp = new Paint();
                cp.setColor(0xc0ff8c00);
                cp.setStyle(Style.FILL);
                for (Board.Position dest : dests) {
                    float sx = layout.screenX(dest.x);
                    float sy = layout.screenY(dest.y);

                    sx += squareDim / 2.0f;
                    sy += squareDim / 2.0f;
                    canvas.drawCircle(sx, sy, 5, cp);
                }
            } else {
                // Dropping a captured piece. Nothing to do
            }
        }

        if (mMoveTo != null) {
            // Move the piece to be moved with 25% transparency.
            int pieceToMove = -1;
            if (mMoveFrom instanceof PositionOnBoard) {
                PositionOnBoard from = (PositionOnBoard) mMoveFrom;
                pieceToMove = mBoard.getPiece(from.x, from.y);
            } else {
                pieceToMove = ((CapturedPiece) mMoveFrom).piece;
                if (mCurrentPlayer == Player.WHITE) pieceToMove = -pieceToMove;
            }
            // draw a big circle around mMoveTo so that people
            // with chubby finger can still see the destination.
            float cx = layout.screenX(mMoveTo.x) + squareDim / 2.0f;
            float cy = layout.screenY(mMoveTo.y) + squareDim / 2.0f;
            if (mCursor == null) {
                drawSelectionCircle(canvas, cx, cy, 1.1f, layout);
            }
            drawPiece(canvas, layout, pieceToMove,
                    layout.screenX(mMoveTo.x),
                    layout.screenY(mMoveTo.y),
                    255);

        }

        if (mCursor != null) {
            if (isHumanPlayer(mCurrentPlayer))
                drawSelectionCircle(canvas, mCursor.getScreenX(), mCursor.getScreenY(), 0.75f, layout);
        }

        if (animation != 0) {
            postInvalidateDelayed(33);
        }
    }


    //
    // Implementation details
    //

    // Bitmap for all the pieces.
    //   First index is color (0=black, 1=white)
    private Bitmap mBitmaps[][];
    private BitmapDrawable board = null;

    private boolean mFlipped;        // if true, flip the board upside down.
    private Player mCurrentPlayer;   // Player currently holding the turn
    private Board mLastBoard;        // Last state of the board
    private Board mBoard;            // Current state of the board

    // Position represents a logical position of a piece. It is either a
    // coordinate on the board (PositionOnBoard), or a captured piece (CapturedPiece).
    private static class Position {
    }

    // Point to a coordinate on the board
    // @invariant (0,0) <= (mX,mY) < (9,9).
    private static class PositionOnBoard extends Position {
        public PositionOnBoard(int xx, int yy) {
            x = xx;
            y = yy;
        }

        @Override
        public boolean equals(Object o) {
            if (o instanceof PositionOnBoard) {
                PositionOnBoard p = (PositionOnBoard) o;
                return x == p.x && y == p.y;
            }
            return false;
        }

        public final int x, y;
    }

    @Override
    public boolean equals(Object o) {
        return this == o;
    }

    @Override
    public int hashCode() {
        // Really not used, but some phone apparently calls this function for no good reason.
        return 0;
    }

    // Draw a dark square at board position <px, py>.
    private void darkenSquare(Canvas canvas, ScreenLayout layout, int px, int py) {
        Paint paint = new Paint();
        float sx = layout.screenX(px);
        float sy = layout.screenY(py);
        paint.setColor(0x30000000);
        final int squareDim = layout.getSquareDim();
        canvas.drawRect(sx - .1f * squareDim, sy - .1f * squareDim, sx + squareDim * 1.1f, sy + squareDim * 1.1f, paint);
    }

    // Point to a captured piece
    private static class CapturedPiece extends Position {
        public CapturedPiece(int pos, int p, int i, float x, float y) {
            position = pos;
            piece = p;
            n = i;
            sx = x;
            sy = y;
        }

        @Override
        public boolean equals(Object o) {
            if (o instanceof CapturedPiece) {
                CapturedPiece other = (CapturedPiece) o;
                return other.position == position &&
                        other.piece == piece &&
                        other.n == n;
                // Note: sx and sy are derived from other fields, so they need not be compared.
            }
            return false;
        }

        @Override
        public int hashCode() {
            return position + piece + n;
        }

        public final int position;  // 0: leftmost piece on the screen, 1: 2nd from the left, etc
        public final int piece;     // Piece type, one of Piece.XX. The value is negative if owned by Player.WHITE.
        public final int n;         // Number of pieces owned of type "piece".
        public final float sx, sy;  // Screen location at which this piece should be drawn.
    }

    // A class describing the pixel-level layout of this view.
    private static class ScreenLayout {
        public ScreenLayout(int width, int height, boolean flipped) {
            mWidth = width;
            mHeight = height;
            mFlipped = flipped;

            // TODO(saito) this code doesn't handle a square screen
            int dim;
            int sep = 10; // # pixels between the board and captured pieces.
            if (width < height) {
                // Portrait layout. Captured pieces are shown at the top & bottom of the board
                dim = width;
                int bottom = dim * 12 / 10 + sep * 2;
                if (bottom > height) {
                    mWidth = width * height / bottom;
                    dim = mWidth;
                    bottom = dim * 12 / 10 + sep * 2;
                }
                mPortrait = true;
                mCapturedWhite = new Rect(0, 0, dim, dim / 10);
                mCapturedBlack = new Rect(0, dim * 11 / 10 + sep * 2, dim, bottom);
                mBoard = new Rect(0, dim / 10 + sep, dim, dim * 11 / 10 + sep);
                mHeight = mCapturedBlack.bottom;
            } else {
                // Landscape layout. Captured pieces are shown at the left & right of the board
                mPortrait = false;
                dim = height;
                int right = dim * 12 / 10 + sep * 2;
                if (right > width) {
                    mHeight = height * width / right;
                    dim = mHeight;
                    right = dim * 12 / 10 + sep * 2;
                }
                mCapturedWhite = new Rect(0, 0, dim / 10, dim);
                mCapturedBlack = new Rect(dim * 11 / 10 + sep * 2, 0, right, dim);
                mBoard = new Rect(dim * 14 / 100, 0, dim * 11 / 10 + sep, dim);
                mWidth = mCapturedBlack.right;
            }
            if (mFlipped) {
                Rect tmp = mCapturedWhite;
                mCapturedWhite = mCapturedBlack;
                mCapturedBlack = tmp;
            }
            mSquareDim = dim / Board.DIM;
        }

        // Get the screen dimension
        public final int getScreenWidth() {
            return mWidth;
        }

        public final int getScreenHeight() {
            return mHeight;
        }

        public final int getSquareDim() {
            return mSquareDim;
        }

        public final boolean getFlipped() {
            return mFlipped;
        }

        public final Rect getBoard() {
            return mBoard;
        }

        // Convert X screen coord to board position. May return a position
        // outside the range [0,Board.DIM).
        public final int boardX(float sx) {
            return maybeFlip((int) ((sx - mBoard.left) / mSquareDim));
        }

        // Convert X screen coord to board position. May return a position
        // outside the range [0,Board.DIM).
        public final int boardY(float sy) {
            return maybeFlip((int) ((sy - mBoard.top) / mSquareDim));
        }

        // Convert X board position to the position of the left edge on the screen.
        public final float screenX(int px) {
            px = maybeFlip(px);
            return mBoard.left + mBoard.width() * px / Board.DIM;
        }

        // Convert board Y position to the position of the top edge on the screen.
        public final float screenY(int py) {
            py = maybeFlip(py);
            return mBoard.top + mBoard.height() * py / Board.DIM;
        }

        private final int maybeFlip(int p) {
            if (mFlipped) {
                return 8 - p;
            } else {
                return p;
            }
        }

        // Return the screen location for displaying a captured piece.
        //
        // @p index an integer 0, 1, 2, ... that specifies the position of the
        // piece in captured list.
        private final int capturedScreenX(Player player, int index) {
            Rect r = (player == Player.BLACK ? mCapturedBlack : mCapturedWhite);
            if (mPortrait) {
                return r.left + mSquareDim * index * 4 / 3;
            } else {
                return r.left;
            }
        }

        private final int capturedScreenY(Player player, int index) {
            Rect r = (player == Player.BLACK ? mCapturedBlack : mCapturedWhite);
            if (mPortrait) {
                return r.top;
            } else {
                return r.top + mSquareDim * index * 4 / 3;
            }
        }

        private boolean mPortrait;    // is the screen in portrait mode?
        private int mWidth, mHeight;  // screen pixel size
        private int mSquareDim;       // pixel size of each square in the board
        private boolean mFlipped;     // draw the board upside down
        private Rect mBoard;          // display the board status
        private Rect mCapturedBlack;  // display pieces captured by black player
        private Rect mCapturedWhite;  // display pieces captured by white player
    }

    private ScreenLayout mCachedLayout;

    // If non-NULL, user is trying to move the piece from this square.
    // @invariant mMoveFrom == null || (0,0) <= mMoveFrom < (Board.DIM, Board.DIM)
    private Position mMoveFrom;

    // If non-NULL, user is trying to move the piece to this square.
    // @invariant mMoveTo== null || (0,0) <= mMoveTo < (Board.DIM, Board.DIM)
    private PositionOnBoard mMoveTo;

    private Play mLastMove;
    private long mAnimationStartTime;
    private long mNextAnimationTime;
    private final int ANIMATION_INTERVAL = 300;

    private EventListener mListener;
    private ArrayList<Player> mHumanPlayers;
    private Bitmap mBoardBitmap = null;
    private int mBoardWidth = -1;
    private int mBoardHeight = -1;
    private boolean mBoardFlipped = false;

    private final void drawEmptyBoard(Canvas canvas, ScreenLayout layout) {
        // Fill the board square
        Rect boardRect = layout.getBoard();
        Paint p = new Paint();

        if (mBoardBitmap == null) {
            p.setColor(BOARD_PLAIN_COLOR);
            canvas.drawRect(boardRect, p);
        }
        else {
            canvas.drawBitmap(mBoardBitmap,boardRect.left,boardRect.top,p);
        }

        // Draw the gridlines
        p.setColor(mBoardName.contains("black") ? 0xc0ffffff : 0xc0000000);
        p.setStrokeWidth(TypedValue.applyDimension(TypedValue.COMPLEX_UNIT_SP, 1, getResources().getDisplayMetrics()));
        for (int i = 0; i < Board.DIM; ++i) {
            final float sx = layout.screenX(i);
            final float sy = layout.screenY(i);
            canvas.drawLine(sx, boardRect.top, sx, boardRect.bottom, p);
            canvas.drawLine(boardRect.left, sy, boardRect.right, sy, p);
        }
    }

    static public int getBoardDrawable(Context c, String name) {
        if (name.equals("plain"))
            return 0;
        return c.getResources().getIdentifier(String.format("@mobi.omegacentauri.shogi:drawable/%s", name), null, null);
    }

    /**
     * List pieces captured by player.
     */
    private final ArrayList<CapturedPiece> listCapturedPieces(
            Board board, ScreenLayout layout,
            Player player) {
        int seq = 0;
        ArrayList<CapturedPiece> pieces = new ArrayList<CapturedPiece>();
        for (Board.CapturedPiece p : board.getCapturedPieces(player)) {
            pieces.add(new CapturedPiece(
                    seq, p.piece, p.n,
                    layout.capturedScreenX(player, seq),
                    layout.capturedScreenY(player, seq)));
            ++seq;
        }
        return pieces;
    }


    private final void drawCapturedPieces(
            Canvas canvas,
            ScreenLayout layout,
            Player player,
            int exceptPiece) {
        ArrayList<CapturedPiece> pieces = listCapturedPieces(mBoard, layout, player);
        for (int i = 0; i < pieces.size(); ++i) {
            CapturedPiece p = pieces.get(i);
            int alpha = 255;
            if (p.equals(mMoveFrom)) alpha = 64;
            // int piece = (player == Player.BLACK ? p.piece : -p.piece);
            if (p.piece == exceptPiece) {
                if (p.n > 1)
                    drawCapturedPiece(canvas, layout, p.piece, p.n-1, p.sx, p.sy, alpha);

            }
            else
                drawCapturedPiece(canvas, layout, p.piece, p.n, p.sx, p.sy, alpha);
        }
    }

    private final void drawPiece(
            Canvas canvas, ScreenLayout layout,
            int piece,
            float sx, float sy, int alpha) {
        boolean isBlack = (Board.player(piece) == Player.BLACK);
        if (isBlack && piece == Piece.OU)
            piece = CHALLENGING_KING;
        Bitmap[] bitmaps = mBitmaps[isBlack ? 0 : 1];
        Bitmap b = bitmaps[Board.type(piece)];
        Paint p = new Paint();
        p.setAlpha(alpha);
        canvas.drawBitmap(b, sx, sy, p);
    }

    private final void drawCapturedPiece(Canvas canvas,
                                         ScreenLayout layout,
                                         int piece, int n, float sx, float sy, int alpha) {
        drawPiece(canvas, layout, piece, sx, sy, alpha);
        if (n > 1) {
            int fontSize = (int)(.35f*layout.getSquareDim());
            Paint p = new Paint();
            p.setTextSize(fontSize);
            p.setColor(0xffeeeeee);
            p.setTypeface(Typeface.DEFAULT_BOLD);
            canvas.drawText(Integer.toString(n),
                    sx + layout.getSquareDim() - fontSize / 4,
                    sy + layout.getSquareDim() - fontSize / 2,
                    p);
        }
    }

    private boolean isHumanPlayer(Player p) {
        return mHumanPlayers.contains(p);
    }

    // Load bitmaps for pieces. Called whenever view is resized
    private final void initializePieceBitmaps(Context context) {
        final String prefix = mPrefs.getString("piece_style", ShogiPreferenceActivity.DEFAULT_PIECES);
        int darkening = Integer.parseInt(mPrefs.getString("darkening", "0"));
        final Resources r = getResources();
        ScreenLayout layout = getScreenLayout();
        int squareDim = layout.getSquareDim();
        if (mBitmaps != null) {
            for (int i=0; i<2; i++)
                for (int k=0; k<Piece.NUM_TYPES+1; k++) {
                    try {
                        mBitmaps[i][k].recycle();
                    }
                    catch(Exception e) {
                    }
                }
        }
        mBitmaps = new Bitmap[2][Piece.NUM_TYPES+1];
        String koma_names[] = {
                null,
                "fu", "kyo", "kei", "gin", "kin", "kaku", "hi", "ou",
                "to", "nari_kyo", "nari_kei", "nari_gin", null, "uma", "ryu"
        };

        final Matrix flip = new Matrix();
        flip.postRotate(180);

        for (int i = 1; i < Piece.NUM_TYPES+1; ++i) {
            int id = 0;
            String name = null;
            if (i == CHALLENGING_KING) {
                id = r.getIdentifier(String.format("@mobi.omegacentauri.shogi:drawable/%s_%s", prefix, "gyokusho"), null, null);
                if (id == 0)
                    name = "ou";
            }
            else {
                if (koma_names[i] == null) continue;
                name = koma_names[i];
            }
            if (id == 0)
                id = r.getIdentifier(String.format("@mobi.omegacentauri.shogi:drawable/%s_%s", prefix, name), null, null);
            Bitmap base = Bitmap.createScaledBitmap(BitmapFactory.decodeResource(r, id), squareDim, squareDim, true);
            Bitmap darkenedBase = null;
            if (darkening != 0) {
                darkenedBase = Bitmap.createBitmap(squareDim, squareDim, Bitmap.Config.ARGB_8888);
                Canvas c = new Canvas(darkenedBase);
                Paint p = new Paint();
                p.setColorFilter(getDarkeningFilter(darkening));
                c.drawBitmap(base, 0,0, p);
            }
            if (! mFlipped) {
                Bitmap flipped = Bitmap.createBitmap(base, 0, 0, squareDim, squareDim, flip, false);
                mBitmaps[1][i] = flipped;
                if (darkenedBase == null) {
                    mBitmaps[0][i] = base;
                }
                else {
                    mBitmaps[0][i] = darkenedBase;
                    base.recycle();
                }
            }
            else {
                mBitmaps[1][i] = base;
                Bitmap flipped = Bitmap.createBitmap(darkenedBase == null ? base : darkenedBase, 0, 0,
                        squareDim, squareDim, flip, false);
                mBitmaps[0][i] = flipped;
                if (darkenedBase != null)
                    darkenedBase.recycle();
            }
        }
    }

    private ColorFilter getDarkeningFilter(float darken) {
        int intensity = (int)(255*(1-darken/100f));
        return new LightingColorFilter(
                0xFF000000 | intensity | (intensity<<8) | (intensity << 16), 0);
    }

    private final ScreenLayout getScreenLayout() {
        return getScreenLayout(getWidth(), getHeight());
    }

    private final ScreenLayout getScreenLayout(int width, int height) {
        if (mCachedLayout != null &&
                mCachedLayout.getScreenWidth() == width &&
                mCachedLayout.getScreenHeight() == height &&
                mCachedLayout.getFlipped() == mFlipped) {
            // reuse the cached value
        } else {
            mCachedLayout = new ScreenLayout(width, height, mFlipped);
        }
        return mCachedLayout;
    }

    @Override protected void onMeasure (int widthMeasureSpec, int heightMeasureSpec) {
        int originalWidth = MeasureSpec.getSize(widthMeasureSpec);
        int originalHeight = MeasureSpec.getSize(heightMeasureSpec);
        ScreenLayout layout = getScreenLayout(originalWidth, originalHeight);
        super.onMeasure(
                MeasureSpec.makeMeasureSpec(layout.getScreenWidth(), MeasureSpec.EXACTLY),
                MeasureSpec.makeMeasureSpec(layout.getScreenHeight(), MeasureSpec.EXACTLY));
    }
}
