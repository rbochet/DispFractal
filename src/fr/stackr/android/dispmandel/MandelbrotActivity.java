package fr.stackr.android.dispmandel;

import java.io.BufferedInputStream;
import java.io.File;
import java.io.FileInputStream;
import java.io.IOException;

import android.app.Activity;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.os.Bundle;
import android.view.View;
import android.widget.ImageView;
import android.widget.TextView;

public class MandelbrotActivity extends Activity {
	private String path = "/data/data/fr.stackr.android.dispmandel/cache/out.bmp";

	static {
		System.loadLibrary("random_image");
	}

	private byte[] randomBytes = new byte[32];

	private native int generateImage(byte[] thirtytwobytes);

	private native int nativeRandomBytes(byte[] randombytes);

	/** Called when the activity is first created. */
	@Override
	public void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		setContentView(R.layout.main);
		new File(path).delete();

		imageClick(null);
	}

	public void imageClick(View view) {
		nativeRandomBytes(randomBytes);
		TextView t = (TextView) findViewById(R.id.message);
		String s = "Key: ";
		for (int i = 0; i < 32; i++) {
			int b = randomBytes[i] & 0xff;
			s = s + Integer.toHexString(b);
		}
		t.setText(s + " -- GeneratingÉ");
		int returnValue = generateImage(randomBytes);
		if (returnValue != -1)
			s = "generateImage(" + s + ") failed (returned " + returnValue
					+ ").";
		t.setText(s);
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