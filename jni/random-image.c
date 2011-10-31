/*
  Generate a random image from a 256 bit number, e.g., for visual confirmation of 
  D-H exchange for authenticating users without bumping or other mechanisms.

*/

#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <jni.h>


int debug=0;

/* 128x128 pixels should be enough, and we will only calculate the mandelbrot to
   255 iterations. */
unsigned char pixels[128][128];
double mx1,mx2,my1,my2;
double xstep,ystep;

unsigned int complexity[128][128];
unsigned int cumulative_complexity=0;

unsigned char bits[256/8];
int bits_left=256;

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
  int v[3];
  int d[3];
  int i;
  int rotor=0;
  d[0]=0; d[1]=1; d[2]=0;
  v[0]=0x80; v[1]=0x20; v[2]=0x00;
  
  /* use up 128 bits coming up with a palette */
  for(i=0;i<256;i++)
    {
      unsigned int b=getbits(2);
      if (b<3) {
	int c=(b+rotor)%3;
	d[c]++;
	if (d[c]>1) d[c]==-1;
	rotor++;
      }
      for(;(i&3)!=3;i++) {
	int j;
	for(j=0;j<3;j++) {
	  v[j]+=0x30*d[j];
	  colours[i][j]=v[j];
	}
	if (debug) printf("%d: %d,%d,%d\n",
			  i,colours[i][0],colours[i][1],colours[i][2]);
      }
      if (debug) printf("i=%d\n",i);
    }
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

int generation_number=1;
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

  for(y=0; y<128; y++)
    {
      double c_im = my1+ystep*y;
      for(x=0; x<128; x++)
	{
	  double c_re = mx1+xstep*x;
	  
	  double Z_re = c_re, Z_im = c_im;
	  int isInside=1;
	  int n;
	  int MaxIterations=32;
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
	  i=n*8;
	  if (i>255) i=255;
	  pixels[x][y]=i;

	  if (debug) {
	    if (!x) printf("\n");
	    printf("%c",32+(pixels[x][y]>>2));
	  }

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
	    xl=x-2; if (xl<0) xl=x;
	    yl=y-2; if (yl<0) yl=y;
	    xh=x+2; yh=y+2;
	    if (xh>127) xh=127;
	    if (yh>127) yh=127;
	    int vlist[25];
	    int vcount=0;
	    for(xn=xl;xn<=xh;xn++)
	      for(yn=yl;yn<=yh;yn++)
		{
		  for(v=0;v<vcount;v++)
		    if (pixels[xn][yn]==vlist[v]) break;
		  if (v>=vcount)
		    vlist[vcount++]=pixels[x][y];
		}

	    if (vcount<12)
	      this_deviation=0;

	    int this_complexity=this_deviation;
	    
	    complexity[x][y]=cumulative_complexity;
	    cumulative_complexity+=this_complexity;
	  } 
	}
    }
  if (debug) printf("Cumulative complexity = %d\n",cumulative_complexity);

  if (cumulative_complexity==0) {
    printf("complexity is zero\n");
    exit(-1);
  }

  int bits_required=0;
  while ((1<<bits_required)<cumulative_complexity) bits_required++;
  int value = getbits(bits_required);
  value^=(1<<((bits_required+generation_number)%bits_required));

  if (debug) printf("value=%d, bits_required=%d, cumulative_complexity=%d\n",
		    value,bits_required,cumulative_complexity);
  value=value*1.0*cumulative_complexity/(1<<bits_required);
  if (debug) printf("normalised value=%d\n",value);

  /* work out the cell that we should focus on */
  for(y=0;y<128;y++) 
    {
      if (complexity[127][y]<value) continue;
      for(x=0;x<128;x++) 
	if (complexity[x][y]>value) break; 
      if (x<128) break;
    }
  if (y==128) {
    printf("y==64 error\n");
    exit(-1);
  }

  if (debug) {
    printf("zooming in on (%d,%d) (complexity=%d) pixels=%d,%d,%d,%d\n",
	   x,y,complexity[x][y],
	   pixels[x][y],pixels[x][y+1],
	   pixels[x+1][y],pixels[x+1][y+1]);
    printf("  pixel chars=%c%c\n              %c%c\n",
	   32+pixels[x][y]/4,32+pixels[x+1][y]/4,
	   32+pixels[x][y+1]/4,32+pixels[x+1][y+1]/4);
  }
  
  if (x<32) x=0; else x-=32;
  if (y<32) y=0; else y-=32;

  mx1=mx1+(x*xstep);
  mx2=mx1+xstep*64;
  my1=my1+(y*ystep);
  my2=my1+ystep*64;

  x--; if (x<0) { x=127;y--; }
  if (debug) printf("previous point (%d,%d) (complexity=%d)\n",
		    x,y,complexity[x][y]);

  generation_number++;
  return 0;
}

JNIEXPORT void JNICALL Java_fr_stackr_android_dispmandel_MandelbrotActivity_generateImage(JNIEnv * env, jobject  obj)
{
    
    
  int i;
  
  mx1=-2.5; mx2=1;
  my1=-2; my2=2;

  /* Get 32 random bytes */
  srandom(time(0));
  for(i=0;i<32;i++) bits[i]=random()&0xff;

  /* XXX The random data should be hashed before use for safety */

  /* XXX This function is far from ideal, but it is probably okay for now */
  
  makePalette();

  while(!generation()) {
    continue;
  }

  writeBitmap("/data/data/fr.stackr.android.dispmandel/cache/out.bmp"); 

}
