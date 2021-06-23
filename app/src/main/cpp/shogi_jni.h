
#ifndef BONANZA_JNI_H
#define BONANZA_JNI_H

#ifdef ANDROID

#include <android/log.h>

#define DEBUG_TAG "NDK_Bonanza"

extern void CheckFailure(const char* file, int line,
                         const char* label,
                         const char* msg, ...);
extern void CheckIntCmpFailure(const char* file, int line,
                               int a, int b,
                               const char* label,
                               const char* msg, ...);

#define CHECK2(expr, ...) if (!(expr)) CheckFailure(__FILE__, __LINE__, #expr, __VA_ARGS__)

#define CHECK(expr) CHECK2(expr, NULL)

#define CHECK2_GE(expr_a, expr_b, ...) {                                \
  int a = (expr_a);                                                     \
  int b = (expr_b);                                                     \
  if (a < b) {                                                          \
    CheckIntCmpFailure(__FILE__, __LINE__, a, b, #expr_a ">" #expr_b,   \
                       __VA_ARGS__);                                    \
  }                                                                     \
 }

#define CHECK_GE(expr_a, expr_b) CHECK2_GE(expr_a, expr_b, NULL)

#define LOG_DEBUG(...) \
  __android_log_print(ANDROID_LOG_DEBUG, DEBUG_TAG, __VA_ARGS__)

#define LOG_FATAL(...) \
  __android_log_print(ANDROID_LOG_FATAL, DEBUG_TAG, __VA_ARGS__)

#else  // !ANDROID

#define LOG_DEBUG(...) printf(__VA_ARGS__)

#endif // !ANDROID

#endif  // BONANZA_JNI_H
