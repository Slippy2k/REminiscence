package org.cyxdown.fb;

public class FbJni {
	static {
		System.loadLibrary("fb-jni");
	}
	final static String DATA_PATH = "/sdcard/Flashback.bin";
	native static void initGame(String dataPath, int width, int height);
	native static void quitGame();
	native static void saveGame();
	native static void drawFrame(int width, int height);
	native static void setLibraryDirectory(String dir);
	native static void setSaveDirectory(String dir);
	native static void queueKeyEvent(int code, int pressed);
	native static void queueTouchEvent(int num, int x, int y, int pressed);
}
