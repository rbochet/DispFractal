/*
   Generate a random image from a 256 bit number, e.g., for visual confirmation of 
   D-H exchange for authenticating users without bumping or other mechanisms.

 */

#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#ifndef STANDALONE
#include <jni.h>
#include <android/log.h>
#endif


int debug=0;

/* 128x128 pixels should be enough, and we will only calculate the mandelbrot to
   255 iterations. */
unsigned char pixels[128][128];
unsigned char pixelset[128][128];
double mx1,mx2,my1,my2;
double xstep,ystep;

unsigned int complexity[128][128];
unsigned int cumulative_complexity=0;

unsigned char bits[256/8];
int bits_left;
int generation_number;

unsigned char colours[256][3];

unsigned int getbits(int count)
{
	unsigned int b=0;

	while(count--) { 
		b=b<<1;
		bits_left--;
		if (bits[bits_left>>3]&(1<<(bits_left&7))) b|=1;
	}

	return b;
}

int makePalette()
{
	unsigned char c0[3],c1[3];
	int d[3];
	int i,j,e;

	/* use up all 256 bits coming up with a palette
	   (and then we use it all up again coming up with the zoom region) */

	/* 8 bits used up right away */
	c0[0]=0+getbits(3)<<4;
	c0[1]=0x80+getbits(3)<<4;
	c0[2]=0x80+getbits(2)<<5;

	// (256/9)<(248/8)
	for(i=0;i<=255;i+=9)
	{
		/* 8 bits of entropy to pick end colours.
		   And a trick to make sure that the end colours are quite different from the beginning colours */
		c1[0]=c0[0]+0x40+getbits(3)<<4;
		c1[1]=c0[1]+0x40+getbits(3)<<4;
		c1[2]=c0[2]+0x40+getbits(2)<<5;

		/* Work out stepping */
		for(e=0;e<3;e++) { 
			d[e]=abs(c1[e]-c0[e])/9;
			if (c1[e]<c0[e]) d[e]=-d[e];
		}

		for(j=0;j<9&&(i+j)<256;j++)
			for(e=0;e<3;e++)
				colours[i+j][e]=c0[e]+d[e]*j;

		for(e=0;e<3;e++) c0[e]=c1[e];
	}

	for(e=0;e<3;e++) colours[255][e]=0;
	return 0;
}

int writePixel(FILE *f,int p)
{
	int r,g,b;

	if (p<0xff) 
		fprintf(f,"%c%c%c%c",
				colours[p][0],
				colours[p][1],
				colours[p][2],
				0xff);
	else
		fprintf(f,"%c%c%c%c",0,0,0,0xff);

	return 0;
}

int writeBitmap(char *filename)
{
	FILE *f=fopen(filename,"w");
	if (!f) return -1;
	int x,y;

	fprintf(f,"%c%c%c%c%c%c%c%c"
			"%c%c%c%c%c%c%c%c"
			"%c%c%c%c%c%c%c%c"
			"%c%c%c%c%c%c%c%c"
			"%c%c%c%c%c%c%c%c"
			"%c%c%c%c%c%c%c%c"
			"%c%c%c%c%c%c%c%c"
			"%c%c%c%c%c%c%c%c"
			"%c%c%c%c%c%c",
			0x42,0x4d,0x46,00,01,00,00,00,00,00,0x46,00,00,00,0x38,00,
			00,00,0x80,00,00,00,0x80,0xff,0xff,0xff,01,00,0x20,00,00,00,
			00,00,00,00,00,00,00,00,00,00,00,00,00,00,00,00,
			00,00,00,00,00,00,00,00,0xff,00,00,0xff,00,00,0xff,00,
			00,00,00,00,00,00);
	for(y=127;y>=0;y--)
		for(x=0;x<128;x++)
		{
			writePixel(f,pixels[x][y]);
		}

	fclose(f);
	return 0;
}

int mandelSetPoint(int x,int y, int c) {
	pixels[x][y]=c;
	pixelset[x][y]=1;
	return 0;
}

int mandelPoint(int x,int y) {
	if (pixelset[x][y]) return 0;
	int i;
	double c_im = my1+ystep*y;
	double c_re = mx1+xstep*x;

	double Z_re = c_re, Z_im = c_im;
	int isInside=1;
	int n;
	int MaxIterations=255;
	if (MaxIterations>255) MaxIterations=255;
	int sf=256/MaxIterations;
	for(n=0; n<MaxIterations; ++n)
	{
		double Z_re2 = Z_re*Z_re, Z_im2 = Z_im*Z_im;
		if(Z_re2 + Z_im2 > 4)
		{
			isInside = 0;
			break;
		}
		Z_im = 2*Z_re*Z_im + c_im;
		Z_re = Z_re2 - Z_im2 + c_re;
	}
	if (n>=MaxIterations) i=255; 
	else {
		i=n*sf;
		if (i>255) i=255;
	}
	pixels[x][y]=i;
	pixelset[x][y]=1;

	if (debug) {
		if (!x) printf("\n");
		printf("%c",32+(pixels[x][y]>>2));
	}
}

int mandelSetBox(int xl,int yl,int xh,int yh,int c)
{
	int x,y;
	for(x=xl;x<=xh;x++)
		for(y=yl;y<=yh;y++)
		{
			pixels[x][y]=c;
			pixelset[x][y]=1;
		}
	return 0;
}

int mandelBox(int xl,int yl,int xh,int yh)
{
	int x,y;
	int foo=0;

	/* Draw along one edge, and then the other to find
	   the extent of the same colour space, */
	y=yl;

	int xm=(xl+xh)/2;
	int ym=(yl+yh)/2;

	y=yl;
	for(x=xl;x<=xm;x++) { mandelPoint(x,y); if (pixels[x][y]!=pixels[xl][yl]) foo|=1; }       // XL,YL -> XM,YL
	for(x=xm+1;x<=xh;x++) { mandelPoint(x,y); if (pixels[x][y]!=pixels[xl][yl]) foo|=2; }     // XM,YL -> XH,YL
	y=ym;
	for(x=xl;x<=xm;x++) { mandelPoint(x,y); if (pixels[x][y]!=pixels[xl][yl]) foo|=4; }       // XL,YM -> XM,YM
	for(x=xm+1;x<=xh;x++) { mandelPoint(x,y); if (pixels[x][y]!=pixels[xl][yl]) foo|=8; }     // XM,YM -> XH,YM
	y=yh;
	for(x=xl;x<=xm;x++) { mandelPoint(x,y); if (pixels[x][y]!=pixels[xl][yl]) foo|=16; }      // XL,YH -> XM,YH
	for(x=xm+1;x<=xh;x++) { mandelPoint(x,y); if (pixels[x][y]!=pixels[xl][yl]) foo|=32; }    // XM,YH -> XH,YH
	x=xl;
	for(y=yl;y<=ym;y++) { mandelPoint(x,y); if (pixels[x][y]!=pixels[xl][yl]) foo|=64; }      // XL,YL -> XL,YM
	for(y=ym+1;y<=yh;y++) { mandelPoint(x,y); if (pixels[x][y]!=pixels[xl][yl]) foo|=128; }   // XL,YM -> XL,YH
	x=xm;
	for(y=yl;y<=ym;y++) { mandelPoint(x,y); if (pixels[x][y]!=pixels[xl][yl]) foo|=256; }     // XM,YL -> XM,YM
	for(y=ym+1;y<=yh;y++) { mandelPoint(x,y); if (pixels[x][y]!=pixels[xl][yl]) foo|=512; }   // XM,YM -> XM,YH
	x=xh;
	for(y=yl;y<=ym;y++) { mandelPoint(x,y); if (pixels[x][y]!=pixels[xl][yl]) foo|=1024; }    // XH,YL -> XH,YM
	for(y=ym+1;y<=yh;y++) { mandelPoint(x,y); if (pixels[x][y]!=pixels[xl][yl]) foo|=2048; }  // XH,YM -> XH,YH

	if (foo&(1|256|4|64)) mandelBox(xl+1,yl+1,xm-1,ym-1); else mandelSetBox(xl+1,yl+1,xm-1,ym-1,pixels[xl][yl]);       // XL,YL -> XM,YM
	if (foo&(4|512|16|128)) mandelBox(xl+1,ym+1,xm-1,yh-1); else mandelSetBox(xl+1,ym+1,xm-1,yh-1,pixels[xl][ym]);     // XL,YM -> XM,YH 
	if (foo&(2|1024|8|256)) mandelBox(xm+1,yl+1,xh-1,ym-1); else mandelSetBox(xm+1,yl+1,xh-1,ym-1,pixels[xm][yl]);    // XM,YL -> XH,YM
	if (foo&(8|2048|32|512)) mandelBox(xm+1,ym+1,xh-1,yh-1); else mandelSetBox(xm+1,ym+1,xh-1,yh-1,pixels[xm][ym]);        // XM,YM -> XH,YH

}

int generation()
{
	if (bits_left<1)  return -1;

	cumulative_complexity=0;

	xstep=(mx2-mx1)/128;
	ystep=(my2-my1)/128;

	double xx,yy,x0,y0;
	int x,y,i;
	int debug=0;

	if (debug) printf("mx1=%f, my1=%f, xstep=%f, ystep=%f\n",mx1,my1,xstep,ystep);

	mandelBox(0,0,127,127);

	for(y=0; y<128; y++)
	{
		for(x=0; x<128; x++)
		{
			double this_mean=0;
			double this_deviation=0;
			int xd,yd;
			for(xd=0;xd<=1;xd++)
				for(yd=0;yd<=1;yd++)
					this_mean+=pixels[x-xd][y-yd];
			this_mean/=4;
			for(xd=0;xd<=1;xd++)
				for(yd=0;yd<=1;yd++)
					this_deviation+=(pixels[x-xd][y-yd]-this_mean)*(pixels[x-xd][y-yd]-this_mean);
			this_deviation/=4;
			this_deviation=sqrt(this_deviation);

			int xn,yn,xl,xh,yl,yh,v;
			xl=x-2; if (xl<0) xl=0;
			yl=y-2; if (yl<0) yl=0;
			xh=x+2; yh=y+2;
			if (xh>127) xh=127;
			if (yh>127) yh=127;
			int vlist[25];
			int vcount=0;
			int vdiff;
			int vdiffsum=0;
			for(xn=xl;xn<=xh;xn++)
				for(yn=yl;yn<=yh;yn++)
				{
					vdiff=pixels[xn][yn];
					for(v=0;v<vcount;v++)
					{
						int pdiff=abs(pixels[xn][yn]-vlist[v]);
						if (!pdiff) break; else if (pdiff<vdiff) vdiff=pdiff;
					}
					if (v>=vcount) { vlist[vcount++]=pixels[xn][yn]; vdiffsum+=vdiff; }
				}

			if (vcount<5)
				this_deviation=0;
			else this_deviation=vdiffsum*vcount;

			int this_complexity=this_deviation;

			complexity[x][y]=cumulative_complexity;
			cumulative_complexity+=this_complexity;
		} 
	}

	if (debug) printf("Cumulative complexity = %d\n",cumulative_complexity);

	if (cumulative_complexity<2) {
#ifdef STANDALONE
		printf("complexity is zero\n");
#else
		__android_log_write(ANDROID_LOG_ERROR,__FUNCTION__,"complexity is zero");
#endif
		return -20-100*generation_number;
	}

	int value;
	int bits_required=0;
	while ((1<<bits_required)<cumulative_complexity) bits_required++;
	value = getbits(bits_required);
	value^=(1<<((bits_required+generation_number/2)%bits_required));

	char msg[1024];
	snprintf(msg,1024,"value=%d, bits_required=%d, cumulative_complexity=%d",
			value,bits_required,cumulative_complexity);
#ifdef STANDALONE
	printf("%s\n",msg);
#else
	__android_log_write(ANDROID_LOG_INFO,__FUNCTION__, msg);
#endif
	value=value*1.0*cumulative_complexity/(1<<bits_required);
#ifdef STANDALONE
	printf("normalised value=%d\n",value);
#else
	snprintf(msg,1024,"normalised value=%d\n",value);
	__android_log_write(ANDROID_LOG_INFO,__FUNCTION__, msg);
#endif

	/* work out the cell that we should focus on */
	for(y=0;y<128;y++) 
	{
		if (complexity[127][y]<value) continue;
		for(x=0;x<128;x++) 
			if (complexity[x][y]>value) {
#ifdef STANDALONE
				printf("complex(%d,%d)>%d (=%d)\n",x,y,value,complexity[x][y]);
#endif
				break; 
			}
		if (x<128) break;
	}
	x--; if (x<0) { x=127;y--; }
#ifndef STANDALONE
	if (y<0||y>127) {
		char msg[1024];
		snprintf(msg,1024,"illegal y value when value hunting: y=%d",y);
		__android_log_write(ANDROID_LOG_ERROR,__FUNCTION__, msg);
	}
#endif
	if (y==128) return -30-100*generation_number;
	if (y<0) return -40-100*generation_number;

	int tx,ty;
#ifdef STANDALONE
	char filename[1024];
	snprintf(filename,1024,"g%02d-b-%d,%d.bmp",generation_number,x,y);

	printf("zooming in on (%d,%d) (complexity=%d) pixels=%d,%d,%d,%d\n",
			x,y,complexity[x][y],
			pixels[x][y],pixels[x][y+1],
			pixels[x+1][y],pixels[x+1][y+1]);
#endif

	if (x<32) x=32;
	if (y<32) y=32;
	if (x>95) x=95;
	if (y>95) y=95;

#ifdef STANDALONE
	printf("CLIPPED zooming in on (%d,%d) (complexity=%d) pixels=%d,%d,%d,%d\n",
			x,y,complexity[x][y],
			pixels[x][y],pixels[x][y+1],
			pixels[x+1][y],pixels[x+1][y+1]);

	/* Dump complexity map */
	int last_complex=0;
	for(ty=0;ty<128;ty++)
		for(tx=0;tx<128;tx++)
		{					       
			pixelset[tx][ty]=pixels[tx][ty];
			pixels[tx][ty]=complexity[tx][ty]-last_complex;
			if (last_complex>complexity[tx][ty]) printf("retrograde complexity count at %d,%d\n",tx,ty);
			last_complex=complexity[tx][ty];
		}
	for(ty=y-32;ty<y+32;ty++)
	{
		pixels[x-32][ty]=255;
		pixels[x+32][ty]=255;
	}
	for(tx=x-32;tx<x+32;tx++)
	{
		pixels[tx][y-32]=255;
		pixels[tx][y+32]=255;
	}

	writeBitmap(filename);
	for(tx=0;tx<128;tx++)
		for(ty=0;ty<128;ty++)
		{					       
			pixels[tx][ty]=pixelset[tx][ty];
		}

#endif

	if (x<32) x=0; else x-=32;
	if (y<32) y=0; else y-=32;

	mx1=mx1+(x*xstep);
	mx2=mx1+xstep*64;
	my1=my1+(y*ystep);
	my2=my1+ystep*64;

#ifdef STANDALONE
	snprintf(filename,1024,"g%02d-a.bmp",generation_number);
	writeBitmap(filename); 
#endif

	if (bits_left>0) {
		/* Copy portion of initial view we can keep for the zoom. */
		for(tx=0;tx<64;tx++)
			for(ty=0;ty<64;ty++)
				pixelset[64+tx][64+ty]=pixels[x+tx][y+ty];
		for(tx=0;tx<64;tx++)
			for(ty=0;ty<64;ty++)
			{
				if (pixelset[64+tx][64+ty]<255) {
					pixels[tx<<1][ty<<1]=pixelset[64+tx][64+ty];
					pixelset[0+(tx<<1)][0+(ty<<1)]=1;
				}
				else
				{
					pixels[0+(tx<<1)][0+(ty<<1)]=255;
					pixelset[0+(tx<<1)][0+(ty<<1)]=0;
				}
				pixels[1+(tx<<1)][0+(ty<<1)]=255;
				pixels[0+(tx<<1)][1+(ty<<1)]=255;
				pixels[1+(tx<<1)][1+(ty<<1)]=255;
				pixelset[0+(tx<<1)][1+(ty<<1)]=0;
				pixelset[1+(tx<<1)][0+(ty<<1)]=0;
				pixelset[1+(tx<<1)][1+(ty<<1)]=0;
			}
#ifdef STANDALONE
		snprintf(filename,1024,"g%02d-z.bmp",generation_number);
		writeBitmap(filename); 
#endif
	}

	if (debug) printf("previous point (%d,%d) (complexity=%d)\n",
			x,y,complexity[x][y]);

	generation_number++;
	return 0;
}

int urandomfd=-1;
int urandombytes(unsigned char *x,unsigned long long xlen)
{
	int i;
	int t=0;

	if (urandomfd == -1) {
		for (i=0;i<4;i++) {
			urandomfd = open("/dev/urandom",O_RDONLY);
			if (urandomfd != -1) break;
			sleep(1);
		}
		if (i==4) return -1;
	}

	while (xlen > 0) {
		if (xlen < 1048576) i = xlen; else i = 1048576;

		i = read(urandomfd,x,i);
		if (i < 1) {
			sleep(1);
			t++;
			if (t>4) return -1;
			continue;
		} else t=0;

		x += i;
		xlen -= i;
	}

	return 0;
}


#ifndef STANDALONE
JNIEXPORT int JNICALL Java_fr_stackr_android_dispmandel_MandelbrotActivity_generateImage(JNIEnv * env, jobject  obj, jbyteArray thirtytwobytes)
{

	int i,x,y;

	mx1=-2.5; mx2=1;
	my1=-2; my2=2;
	bits_left=256;

	for(x=0;x<128;x++) for(y=0;y<128;y++) pixelset[x][y]=0;

	if ((*env)->GetArrayLength(env, thirtytwobytes)!=32) return -1;
	jbyte *r = (*env)->GetPrimitiveArrayCritical(env, thirtytwobytes, NULL);
	if (r) for(i=0;i<32;i++) bits[i]=r[i];
	if (r) (*env)->ReleasePrimitiveArrayCritical(env, thirtytwobytes, r, 0);
	if (!r) return -2;

	/* XXX The random data should be hashed before use for safety.
	   Easier to do from Java, I suspect. */


	/* XXX This function is far from ideal, but it is probably okay for now */

	makePalette();
	bits_left=256;

	int rv=1;
	generation_number=1;
	while(!(rv=generation())) {
		continue;
	}

	if (rv==-1) {
		__android_log_write(ANDROID_LOG_INFO,__FUNCTION__,"Writing image");    
		writeBitmap("/data/data/fr.stackr.android.dispmandel/cache/out.bmp"); 
	}
	else
		__android_log_write(ANDROID_LOG_ERROR,__FUNCTION__,"Got bad return value, not writing image");    
	return rv;

}

	JNIEXPORT jint JNICALL Java_fr_stackr_android_dispmandel_MandelbrotActivity_nativeRandomBytes
(JNIEnv *env, jobject obj, jbyteArray bytes)
{
	int l=(*env)->GetArrayLength(env, bytes);
	if (l<1) return -1;
	jbyte *b = (*env)->GetPrimitiveArrayCritical(env, bytes, NULL);

	urandombytes(b,l);

	if (b) (*env)->ReleasePrimitiveArrayCritical(env, bytes, b, 0);
	return 0;
}
#else
int main()
{
	int i,x,y;

	mx1=-2.5; mx2=1;
	my1=-2; my2=2;
	bits_left=256;

	urandombytes(bits,32);

	for(x=0;x<128;x++) for(y=0;y<128;y++) pixelset[x][y]=0;

	/* XXX The random data should be hashed before use for safety.
	   Easier to do from Java, I suspect. */


	/* XXX This function is far from ideal, but it is probably okay for now */

	makePalette();
	bits_left=256;

	int r=1;
	generation_number=1;
	while((r=generation())==0) {
		continue;
	}

	writeBitmap("out.bmp"); 
	return r;
}
#endif
