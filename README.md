Requirements
============
* Android SDK > 8 (2.1)
* Android NDK 

How to test
===========
Clone it with Git.

	git clone git@github.com:rbochet/DispFractal.git
	

Get in the jni folder

	ndk-build clean && ndk-build
	
Get back in Eclipse, and clean the project

Warning
=======
As this relies on the NDK (or JNI), changing packages/class/method names can result in issues. Don't forget to change them accordingly in ``jni/random-image.c``.

