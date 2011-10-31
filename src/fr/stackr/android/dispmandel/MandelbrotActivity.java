package fr.stackr.android.dispmandel;

import java.io.BufferedInputStream;
import java.io.File;
import java.io.FileInputStream;
import java.io.IOException;

import android.app.Activity;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.os.Bundle;
import android.util.Log;
import android.widget.ImageView;

public class MandelbrotActivity extends Activity {
	private String path = "/data/data/fr.stackr.android.dispmandel/cache/out.bmp";

	static {
		System.loadLibrary("random_image");
	}

	private native void generateImage();

	/** Called when the activity is first created. */
	@Override
	public void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		setContentView(R.layout.main);
		Log.v("RB", getCacheDir().getAbsolutePath());
		new File(path).delete();
		generateImage();
		disp();
	}


	private void disp() {
		ImageView image = (ImageView) findViewById(R.id.fractal);
		try {
			BufferedInputStream bis = new BufferedInputStream(
					new FileInputStream(path));
			Bitmap bm = BitmapFactory.decodeStream(bis);
			bis.close();
			image.setImageBitmap(bm);
		} catch (IOException e) {
			e.printStackTrace();
		}

	}

}