
package org.cyxdown.fb;

import javax.microedition.khronos.egl.EGLConfig;
import javax.microedition.khronos.opengles.GL10;

import android.opengl.GLSurfaceView;

public class FbRenderer implements GLSurfaceView.Renderer {
	
	private boolean m_init = false;
	private int m_width = 0;
	private int m_height = 0;
	
	public FbRenderer() {
		super();
	}

	public void onDrawFrame(GL10 gl) {
		FbJni.drawFrame(m_width, m_height);
	}
	
	public void onSurfaceChanged(GL10 gl, int width, int height) {
		if (!m_init) {
			m_width = width;
			m_height = height;
			FbJni.initGame(FbJni.DATA_PATH, width, height);
			m_init = true;
		}
	}

	public void onSurfaceCreated(GL10 gl, EGLConfig config) {
	}
}

