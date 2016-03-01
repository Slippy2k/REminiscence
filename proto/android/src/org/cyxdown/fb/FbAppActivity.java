package org.cyxdown.fb;

import java.io.File;
import java.io.IOException;

import android.app.Activity;
import android.app.AlertDialog;
import android.content.DialogInterface;
import android.content.Context;

import android.opengl.GLSurfaceView;
import android.os.Bundle;
import android.view.KeyEvent;
import android.view.MotionEvent;

public class FbAppActivity extends Activity {

	class FbGLSurfaceView extends GLSurfaceView {
		FbGLSurfaceView(Context context) {
			super(context);
			setRenderer(new FbRenderer());
			setFocusable(true);
			setFocusableInTouchMode(true);
		}

		@Override
		public boolean onKeyDown(int keyCode, KeyEvent event) {
			return queueKeyEvent(keyCode, 1);
		}

		@Override
		public boolean onKeyUp(int keyCode, KeyEvent event) {
			return queueKeyEvent(keyCode, 0);
		}

		public boolean onTouchEvent(MotionEvent event) {
			int i, id;
			final int action = event.getAction();
			switch (action & MotionEvent.ACTION_MASK) {
			case MotionEvent.ACTION_DOWN:
				queueTouchEvent(0, (int)event.getX(), (int)event.getY(), 1);
				break;
			case MotionEvent.ACTION_UP:
				queueTouchEvent(0, (int)event.getX(), (int)event.getY(), 0);
				break;
			case MotionEvent.ACTION_MOVE:
				for (i = 0; i < event.getPointerCount(); ++i) {
					id = event.getPointerId(i);
					queueTouchEvent(id, (int)event.getX(i), (int)event.getY(i), 1);
				}
				break;
			case MotionEvent.ACTION_CANCEL:
				queueTouchEvent(0, (int)event.getX(), (int)event.getY(), 0);
				break;
			case MotionEvent.ACTION_POINTER_DOWN:
				i = (action & MotionEvent.ACTION_POINTER_INDEX_MASK) >> MotionEvent.ACTION_POINTER_INDEX_SHIFT;
				id = event.getPointerId(i);
				queueTouchEvent(id, (int)event.getX(i), (int)event.getY(i), 1);
				break;
			case MotionEvent.ACTION_POINTER_UP:
				i = (action & MotionEvent.ACTION_POINTER_INDEX_MASK) >> MotionEvent.ACTION_POINTER_INDEX_SHIFT;
				id = event.getPointerId(i);
				queueTouchEvent(id, (int)event.getX(i), (int)event.getY(i), 0);
				break;
			}
			return true;
		}
        
		public boolean onTrackballEvent(MotionEvent event) {
			return false;
		}
        
		private boolean queueKeyEvent(final int code, final int pressed) {
			switch (code) {
			case KeyEvent.KEYCODE_DPAD_LEFT:
			case KeyEvent.KEYCODE_DPAD_RIGHT:
			case KeyEvent.KEYCODE_DPAD_UP:
			case KeyEvent.KEYCODE_DPAD_DOWN:
			case KeyEvent.KEYCODE_DPAD_CENTER:
			case KeyEvent.KEYCODE_SHIFT_LEFT:
			case KeyEvent.KEYCODE_SHIFT_RIGHT:
			case KeyEvent.KEYCODE_SPACE:
			case KeyEvent.KEYCODE_DEL:
				queueEvent(new Runnable() {
					public void run() {
						FbJni.queueKeyEvent(code, pressed);
					}
				});
				return true;
			}
			return false;
		}

		private boolean queueTouchEvent(final int num, final int x, final int y, final int pressed) {
			queueEvent(new Runnable() {
				public void run() {
					FbJni.queueTouchEvent(num, x, y, pressed);
				}
			});
			return true;
		}
	}

	private boolean checkGameData() {
		return new File(FbJni.DATA_PATH).exists();
	}

	private void showAlert(String s) {
		AlertDialog.Builder builder = new AlertDialog.Builder(this);
		builder.setMessage(s);
		builder.setPositiveButton("OK", new DialogInterface.OnClickListener() {
			public void onClick(DialogInterface dialog, int id) {
			}
		});
		AlertDialog dialog = builder.create();
		dialog.show();
	}
	
	private GLSurfaceView m_surfaceView;

	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		if (!checkGameData()) {
			setContentView(R.layout.main);
			showAlert("Unable to open " + FbJni.DATA_PATH);
			return;
		}
		String cacheDir = "/data/data/org.cyxdown.fb/cache";
		try {
			cacheDir = getCacheDir().getCanonicalPath();
		} catch (IOException e) {
		}
		FbJni.setSaveDirectory(cacheDir);
		FbJni.setLibraryDirectory(cacheDir.replace("cache", "lib"));
		m_surfaceView = new FbGLSurfaceView(this);
		setContentView(m_surfaceView);
	}
	
	protected void onDestroy() {
		FbJni.quitGame();
		super.onDestroy();
	}

	protected void onPause() {
		FbJni.saveGame();
		super.onPause();
	}

	protected void onResume() {
		super.onResume();
		if (m_surfaceView != null) {
			m_surfaceView.onResume();
		}
	}
}
