#include <jni.h>
#include <pthread.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>
#include <android/log.h>

#include "shogi.h"
#include "shogi_jni.h"

// CAUTION: These constants must match the values defined in BonanzaJNI.java
#define R_OK 0
#define R_ILLEGAL_MOVE -1
#define R_CHECKMATE -2
#define R_RESIGNED -3
#define R_DRAW -4
#define R_INSTANCE_DELETED -5
#define R_INITIALIZATION_ERROR -6

// Handicap settings. A positive (negative) value means that the black
// (resp. white) player will remove the specified pieces from the initial
// board configuration.
//
// CAUTION: these values must match R.array.handicap_type_values
#define H_NONE 0
#define H_KYO  1  // left kyo
#define H_KAKU 2
#define H_HI 3
#define H_HI_KYO 4  // hi + left kyo
#define H_HI_KAKU 5
#define H_HI_KAKU_KYO 6
#define H_HI_KAKU_KEI_KYO 7

static pthread_mutex_t g_lock = PTHREAD_MUTEX_INITIALIZER;
static int g_instance_id = 0;
static int g_initialized = 0;
static char* g_initialization_error = NULL;
char* g_storage_dir = NULL;
static char* Basename(const char* path, char* buf, int buf_size) {
  const char* r = strrchr(path, '/');
  if (r == NULL) r = path;
  else r++;
  snprintf(buf, buf_size, "%s", r);
  return buf;
}

static void PrintExtraMessage(const char* fmt, va_list ap,
                              char* buf, int buf_size) {
  vsnprintf(buf, buf_size, fmt, ap);
  if (str_error != NULL) {
    strncat(buf, ": ", buf_size - 1);
    strncat(buf, str_error, buf_size - 1);
  }
}

void CheckFailure(const char* file, int line,
                  const char* label, const char* fmt, ...) {
  char msg_buf[256];
  char file_buf[256];
  msg_buf[0] = '\0';
  if (fmt != NULL) {
    va_list ap;
    va_start(ap, fmt);
    PrintExtraMessage(fmt, ap, msg_buf, sizeof(msg_buf));
    va_end(ap);
  }
  __android_log_print(ANDROID_LOG_FATAL, DEBUG_TAG,
		      "Aborted: %s:%d: %s %s",
                      Basename(file, file_buf, sizeof(file_buf)),
                      line, label, msg_buf);
  abort();
}

void CheckIntCmpFailure(const char* file, int line,
                        int a, int b,
                        const char* label,
                        const char* fmt, ...) {
  char msg_buf[256];
  char file_buf[256];
  msg_buf[0] = '\0';
  if (fmt != NULL) {
    va_list ap;
    va_start(ap, fmt);
    PrintExtraMessage(fmt, ap, msg_buf, sizeof(msg_buf));
    va_end(ap);
  }
  __android_log_print(ANDROID_LOG_FATAL, DEBUG_TAG,
		      "Aborted: %s:%d: %s %d<>%d %s",
                      Basename(file, file_buf, sizeof(file_buf)),
                      line, label, a, b, msg_buf);
  abort();
}

static void LogTree(const char* label, tree_t* ptree) {
  char msg[1024];
#if 0  // full board display. this is too verbose, so turn off
  for (int i = 0; i < 9; ++i) {
    sprintf(msg, "%d: ", i);
    for (int j = 0; j < 9; ++j) {
      char* p = msg + strlen(msg);
      sprintf(p, "%02d ", BOARD[i * 9 + j]);
    }
    LOG_DEBUG("%s: Board: %s", label, msg);
  }
#endif
  LOG_DEBUG("%s: captured black %x white %x material %x",
            label,
            ptree->posi.hand_black,
            ptree->posi.hand_white,
            ptree->posi.material);
  msg[0] = '\0';
  if (game_status & flag_quit) {
    strcat(msg, "quit ");
  }
  if (game_status & flag_mated) {
    strcat(msg, "checkmate ");
  }
  if (game_status & flag_drawn) {
    strcat(msg, "draw ");
  }
  if (game_status & flag_resigned) {
    strcat(msg, "resigned ");
  }
  LOG_DEBUG("%s: game: %s", label, msg);
}

static void FillIntField(JNIEnv* env,
                         int value, jclass cls, jobject obj,
                         const char* field) {
  jfieldID fid = (*env)->GetFieldID(env, cls, field, "I");
  (*env)->SetIntField(env, obj, fid, value);
}

static int ExtractIntField(JNIEnv* env,
                        jclass cls, jobject obj,
                        const char* field) {
  jfieldID fid = (*env)->GetFieldID(env, cls, field, "I");
  return (*env)->GetIntField(env, obj, fid);
}

// Copy the board config (piece locations and captured pieces for each player)
// from "ptree" to "board".
static void FillBoard(const char* label,
                      JNIEnv* env,
		      tree_t* ptree, /* source */
		      jobject board /* dest */) {
  LogTree(label, ptree);

  jclass boardClass = (*env)->GetObjectClass(env, board);
  jfieldID fid = (*env)->GetFieldID(env, boardClass, "mSquares", "[I");
  jintArray jarray = (jintArray)((*env)->GetObjectField(env, board, fid));
  jint tmp[nsquare];
  for (int i = 0; i < nsquare; ++i) {
    tmp[i] = BOARD[i];
  }
  (*env)->SetIntArrayRegion(env,  jarray, 0, nsquare, tmp);

  fid = (*env)->GetFieldID(env, boardClass, "mCapturedBlack", "I");
  (*env)->SetIntField(env, board, fid, ptree->posi.hand_black);

  fid = (*env)->GetFieldID(env, boardClass, "mCapturedWhite", "I");
  (*env)->SetIntField(env, board, fid, ptree->posi.hand_white);
}

static void FillMoveResult(JNIEnv* env,
                           jobject move_result,
                           int move,
                           const char* move_str) {
  jclass move_class = (*env)->GetObjectClass(env, move_result);
  jfieldID fid = (*env)->GetFieldID(env, move_class,
                                    "move", "Ljava/lang/String;");
  (*env)->SetObjectField(env, move_result, fid,
                         (*env)->NewStringUTF(env, move_str));

  FillIntField(env, move, move_class, move_result, "cookie");
}

static void FillResult(const char* label,
                       JNIEnv* env,
                       int iret,
                       const char* error,
                       const char* move_str,
                       int move_cookie,
                       tree_t* ptree,
                       jobject result) {
  jclass result_class = (*env)->GetObjectClass(env, result);

  FillIntField(env, iret, result_class, result, "status");
  if (error != NULL) {
    jfieldID fid = (*env)->GetFieldID(env, result_class,
                                      "error", "Ljava/lang/String;");
    (*env)->SetObjectField(env, result, fid, (*env)->NewStringUTF(env, error));
  }

  // Fill move_str and move_cookie
  if (move_str != NULL) {
    jfieldID fid = (*env)->GetFieldID(env, result_class,
                                      "move", "Ljava/lang/String;");
    (*env)->SetObjectField(env, result, fid,
                           (*env)->NewStringUTF(env, move_str));
  }
  FillIntField(env, move_cookie, result_class, result, "moveCookie");

  // Fill the board
  if (ptree != NULL) {
    jfieldID fid = (*env)->GetFieldID(env, result_class,
                                      "board", "Lcom/ysaito/shogi/Board;");
    jobject board = (*env)->GetObjectField(env, result, fid);
    jclass board_class = (*env)->GetObjectClass(env, board);
    fid = (*env)->GetFieldID(env, board_class, "mSquares", "[I");
    jintArray jarray = (jintArray)((*env)->GetObjectField(env, board, fid));
    jint tmp[nsquare];
    for (int i = 0; i < nsquare; ++i) {
      tmp[i] = BOARD[i];
    }
    (*env)->SetIntArrayRegion(env,  jarray, 0, nsquare, tmp);
    fid = (*env)->GetFieldID(env, board_class, "mCapturedBlack", "I");
    (*env)->SetIntField(env, board, fid, ptree->posi.hand_black);
    fid = (*env)->GetFieldID(env, board_class, "mCapturedWhite", "I");
    (*env)->SetIntField(env, board, fid, ptree->posi.hand_white);
  }
}

static void RunCommand(const char* command) {
  LOG_DEBUG("Run %s", command);
  strcpy(str_cmdline, command);
  CHECK2_GE(procedure(&tree), 0, "error: %s", str_error);
}

typedef char MoveBuf[12];
static int ParseCsaMove(JNIEnv* env,
                        jstring jmove_str,
                        unsigned int* move,
                        MoveBuf move_str) {
  const char* tmp = (*env)->GetStringUTFChars(env, jmove_str, NULL);
  CHECK(tmp != NULL);
  CHECK2(strlen(tmp) < sizeof(MoveBuf) - 1, "move: %s", tmp);
  strcpy(move_str, tmp);

  int r = interpret_CSA_move(&tree, move, move_str);
  if (r < 0) {
    LOG_DEBUG("Failed to parse move: %s: %s", move_str, str_error);
  }
  (*env)->ReleaseStringUTFChars(env, jmove_str, tmp);
  return r;
}

static void SetDifficulty(int difficulty,
                          int total_think_time_secs,
                          int per_turn_think_time_secs) {
  // Disable background thinking. I don't know how to stop it in timely
  // fashion.
  RunCommand("ponder off");

  // Don't let the computer resign until it's really desperate.
  RunCommand("resign 999999");

  node_limit   = UINT64_MAX;
  sec_limit    = total_think_time_secs;
  sec_limit_up = per_turn_think_time_secs;

  if (difficulty == 0) {
    depth_limit = 1;
  } else if (difficulty == 1) {
    depth_limit = 2;
  } else if (difficulty == 2) {
    depth_limit = 4;
  } else if (difficulty == 3) {
    depth_limit = 6;
  } else {
    depth_limit  = PLY_MAX;
  }
  LOG_DEBUG("Set difficulty: #node=%llu #depth=%d total=%ds, per_turn=%ds",
            (unsigned long long)node_limit, depth_limit, sec_limit, sec_limit_up);
}

static void ClearBoard(int x, int y, min_posi_t* pos) {
  pos->asquare[x + y * nfile] = 0;
}

static void GenerateInitialBoardConfiguration(jintArray jarray,
					      min_posi_t* pos) {
}

static int GameStatusToReturnCode() {
  CHECK2((game_status & flag_quit) == 0, "status: %x", game_status);
  if (game_status & flag_mated) {
    return R_CHECKMATE;
  }
  if (game_status & flag_drawn) {
    return R_DRAW;
  }
  if (game_status & flag_resigned) {
    return R_RESIGNED;
  }
  return R_OK;
}

void Java_com_ysaito_shogi_BonanzaJNI_initialize(
    JNIEnv *env,
    jclass unused_bonanza_class,
    jstring storage_dir) {
  pthread_mutex_lock(&g_lock);
  if (!g_initialized) {
    const char* tmp = (*env)->GetStringUTFChars(env, storage_dir, NULL);
    g_storage_dir = strdup(tmp);
    (*env)->ReleaseStringUTFChars(env, storage_dir, tmp);
    LOG_DEBUG("Start initializing Bonanza, dir=%s", g_storage_dir);
    if (ini(&tree) < 0) {
      asprintf(&g_initialization_error,
               "Failed to initialize Bonanza: %s", str_error);
    }
    g_initialized = 1;
    LOG_DEBUG("Initialized Bonanza, dir=%s", g_storage_dir);
  }
  pthread_mutex_unlock(&g_lock);
}

jint Java_com_ysaito_shogi_BonanzaJNI_startGame(
    JNIEnv *env,
    jclass unused_bonanza_class,
    jint resume_instance_id,
    jobject initial_board,
    jint next_turn,  // 0: black 1: white
    jint difficulty,
    jint total_think_time_secs,
    jint per_turn_think_time_secs,
    jobject result) {
  int instance_id = -1;

  pthread_mutex_lock(&g_lock);
  if (!g_initialized) {
    FillResult("Init", env,
               R_INITIALIZATION_ERROR, "Bonanza not yet initialized",
               NULL, 0, NULL, result);
  } else if (g_initialization_error != NULL) {
    FillResult("Init", env,
               R_INITIALIZATION_ERROR, g_initialization_error,
               NULL, 0, NULL, result);
  } else if (resume_instance_id != 0 && resume_instance_id == g_instance_id) {
    LOG_DEBUG("Resuming game %d", g_instance_id);
    instance_id = resume_instance_id;
  } else {
    instance_id = ++g_instance_id;
    LOG_DEBUG("Starting game: d=%d t=%d p=%d",
              difficulty,
              total_think_time_secs, per_turn_think_time_secs);

    jclass board_class = (*env)->GetObjectClass(env, initial_board);
    jfieldID fid = (*env)->GetFieldID(env, board_class, "mSquares", "[I");
    jintArray jarray = (jintArray)((*env)->GetObjectField(
        env, initial_board, fid));

    min_posi_t initial_pos = min_posi_no_handicap;
    initial_pos.turn_to_move = next_turn;
    jint tmp_array[nsquare];
    (*env)->GetIntArrayRegion(env,  jarray, 0, nsquare, tmp_array);
    for (int i = 0; i < nsquare; ++i) {
      initial_pos.asquare[i] = tmp_array[i];
    }

    fid = (*env)->GetFieldID(env, board_class, "mCapturedBlack", "I");
    initial_pos.hand_black = (*env)->GetIntField(env, initial_board, fid);
    fid = (*env)->GetFieldID(env, board_class, "mCapturedWhite", "I");
    initial_pos.hand_white = (*env)->GetIntField(env, initial_board, fid);
    
    if (ini_game(&tree, &initial_pos, flag_history, NULL, NULL) < 0) {
      LOG_FATAL("Failed to initialize game: %s", str_error);
    }
    LOG_DEBUG("Initialized Bonanza successfully");

    // Disable background thinking; we can't send a interrupt Bonanza
    // in the middle of thinking otherwise.
    //
    // TODO(saito) I must be missing something. Enable background
    // thinking.
    SetDifficulty(difficulty,
                  total_think_time_secs,
                  per_turn_think_time_secs);
  }
  FillResult("Init", env, R_OK, NULL, NULL, 0, &tree, result);
  pthread_mutex_unlock(&g_lock);
  return instance_id;
}

static int AnotherInstanceStarted(JNIEnv* env,
                                  int instance_id,
                                  jobject result) {
  if (instance_id != g_instance_id) {
    FillResult("Human", env,
               R_INSTANCE_DELETED,
               "Another game already started",
               NULL, 0, NULL, result);
    return 1;
  }
  return 0;
}

void Java_com_ysaito_shogi_BonanzaJNI_humanMove(
    JNIEnv *env,
    jclass unused_bonanza_class,
    jint instance_id,
    jstring jmove_str,
    jobject result) {
  pthread_mutex_lock(&g_lock);
  if (AnotherInstanceStarted(env, instance_id, result)) {
    pthread_mutex_unlock(&g_lock);
    return;
  }

  MoveBuf move_str_buf;
  char* move_str = NULL;
  unsigned int move = 0;
  int status = R_OK;
  const char* error = NULL;
  int r = ParseCsaMove(env, jmove_str, &move, move_str_buf);
  if (r < 0) {
    LOG_DEBUG("Failed to parse move: %s: %s", move_str, str_error);
    status = R_ILLEGAL_MOVE;
    error = str_error;
  } else {
    r = make_move_root(&tree, move,
                       (flag_history | flag_time | flag_rep
                        | flag_detect_hang
                        | flag_rejections));
    if (r < 0) {
      LOG_DEBUG("Failed to make move: %s: %s", move_str_buf, str_error);
      move_str = NULL;
      move = 0;
      status = R_ILLEGAL_MOVE;
      error = str_error;
    } else {
      LOG_DEBUG("Human: %s", move_str_buf);
      move_str = move_str_buf;
      status = GameStatusToReturnCode();
      error = NULL;
    }
  }
  FillResult("Human", env, status, error, move_str, move, &tree, result);
  pthread_mutex_unlock(&g_lock);
}

void Java_com_ysaito_shogi_BonanzaJNI_undo(
    JNIEnv *env,
    jclass unused_bonanza_class,
    jint instance_id,
    jint undo_cookie1,
    jint undo_cookie2,
    jobject result) {
  pthread_mutex_lock(&g_lock);
  if (AnotherInstanceStarted(env, instance_id, result)) {
    pthread_mutex_unlock(&g_lock);
    return;
  }

  MoveBuf move_str;
  unsigned int move;
  CHECK2_GE(undo_cookie1, 1, "Cookie: %x", undo_cookie1);
  unmake_move_root(&tree, undo_cookie1);
  if (undo_cookie2 >= 0) {
    unmake_move_root(&tree, undo_cookie2);
  }
  LOG_DEBUG("Undo: %x %x", undo_cookie1, undo_cookie2);
  FillResult("Undo", env, R_OK, NULL, NULL, 0, &tree, result);
  pthread_mutex_unlock(&g_lock);
}

void Java_com_ysaito_shogi_BonanzaJNI_computerMove(
    JNIEnv *env,
    jclass unused_bonanza_class,
    jint instance_id,
    jobject result) {
  int status = R_OK;
  pthread_mutex_lock(&g_lock);
  if (AnotherInstanceStarted(env, instance_id, result)) {
    pthread_mutex_unlock(&g_lock);
    return;
  }

  CHECK2_GE(com_turn_start(&tree, 0), 0, "error: %s", str_error);

  unsigned int move = last_pv_save.a[1];
  const char* move_str = NULL;
  if (move != 0) {
    move_str = str_CSA_move( move );
    LOG_DEBUG("Comp: %x %s", move, move_str);
  } else {
    // Computer likely have resigned
  }
  status = GameStatusToReturnCode();
  FillResult("Computer", env, status, NULL, move_str, move, &tree, result);
  pthread_mutex_unlock(&g_lock);
}

void Java_com_ysaito_shogi_BonanzaJNI_abort(
    JNIEnv *env,
    jclass unused_bonanza_class) {
  LOG_DEBUG("Aborting the game");
  root_abort = 1;
}
