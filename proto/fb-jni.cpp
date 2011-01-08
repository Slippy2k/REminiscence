
#define LOG_TAG "FbJni"

#include <stdio.h>
#include <string.h>
#include <sys/param.h>
#include <jni.h>
#include <android/log.h>
#include <dlfcn.h>
#include "stub.h"

static const char *gFbSoName = "libfb.so";
static const char *gFbSoSym = "g_stub";

static void *g_dlFbSo;
static Stub *g_stub;
static char *g_libDir;
static char *g_saveDir;
static const char *g_dataPath = "/sdcard/Flashback.bin";

extern "C" {

static int loadFbLib() {
	char path[MAXPATHLEN];
	if (g_libDir) {
		snprintf(path, sizeof(path), "%s/%s", g_libDir, gFbSoName);
		g_dlFbSo = dlopen(path, RTLD_NOW);
	} else {
		g_dlFbSo = dlopen(gFbSoName, RTLD_NOW);
	}
	if (!g_dlFbSo) {
		__android_log_print(ANDROID_LOG_ERROR, LOG_TAG, "Unable to open '%s'", gFbSoName);
		return 1;
	}
	g_stub = (struct Stub *)dlsym(g_dlFbSo, gFbSoSym);
	if (!g_stub) {
		__android_log_print(ANDROID_LOG_ERROR, LOG_TAG, "Unable to lookup symbol '%s'", gFbSoSym);
		dlclose(g_dlFbSo);
		return 1;
	}
	return 0;
}

JNIEXPORT void JNICALL Java_org_cyxdown_fb_FbJni_drawFrame(JNIEnv *env, jclass c, jint w, jint h) {
	if (g_stub) {
		g_stub->doTick();
		g_stub->drawGL(w, h);
	}
}

JNIEXPORT void JNICALL Java_org_cyxdown_fb_FbJni_initGame(JNIEnv *env, jclass c, jint w, jint h) {
	loadFbLib();
	if (g_stub) {
		g_stub->init(g_dataPath, g_saveDir, 0);
		g_stub->initGL(w, h);
	}
}

JNIEXPORT void JNICALL Java_org_cyxdown_fb_FbJni_quitGame(JNIEnv *env, jclass c) {
	if (g_stub) {
		g_stub->quit();
		g_stub = 0;
	}
}

JNIEXPORT void JNICALL Java_org_cyxdown_fb_FbJni_queueKeyEvent(JNIEnv *env, jclass c, jint keycode, jint pressed) {
	if (g_stub) {
		__android_log_print(ANDROID_LOG_INFO, LOG_TAG, "keyEvent %d pressed %d", keycode, pressed);
		g_stub->queueKeyInput(keycode, pressed);
	}
}

JNIEXPORT void JNICALL Java_org_cyxdown_fb_FbJni_queueTouchEvent(JNIEnv *env, jclass c, jint num, jint x, jint y, jint pressed) {
	if (g_stub) {
//		__android_log_print(ANDROID_LOG_INFO, LOG_TAG, "touchEvent %d,%d pressed %d index %d", x, y, pressed, num);
		g_stub->queueTouchInput(num, x, y, pressed);
	}
}

JNIEXPORT void JNICALL Java_org_cyxdown_fb_FbJni_setLibraryDirectory(JNIEnv *env, jclass c, jstring jpath) {
	const char *path = env->GetStringUTFChars(jpath, 0);
	g_libDir = strdup(path);
	env->ReleaseStringUTFChars(jpath, path);
	__android_log_print(ANDROID_LOG_INFO, LOG_TAG, "Using '%s' as directory for library", g_libDir);
}

JNIEXPORT void JNICALL Java_org_cyxdown_fb_FbJni_setSaveDirectory(JNIEnv *env, jclass c, jstring jpath) {
	const char *path = env->GetStringUTFChars(jpath, 0);
	g_saveDir = strdup(path);
	env->ReleaseStringUTFChars(jpath, path);
	__android_log_print(ANDROID_LOG_INFO, LOG_TAG, "Using '%s' as directory for savestates", g_saveDir);
}

}

