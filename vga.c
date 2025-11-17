#include <unistd.h>
#include <stdio.h>

// if using CPUlator, you should copy+paste contents of the file below instead of using #include
#include "address_map_niosv.h"


typedef uint16_t pixel_t;

volatile pixel_t *pVGA = (pixel_t *)FPGA_PIXEL_BUF_BASE;

const pixel_t blk = 0x0000;
const pixel_t wht = 0xffff;
const pixel_t red = 0xf800;
const pixel_t grn = 0x07e0;
const pixel_t blu = 0x001f;

void delay( int N )
{
	for( int i=0; i<N; i++ ) 
		*pVGA; // read volatile memory location to waste time
}





/* STARTER CODE BELOW. FEEL FREE TO DELETE IT AND START OVER */



void drawPixel( int y, int x, pixel_t colour )
{
	*(pVGA + (y<<YSHIFT) + x ) = colour;
}

pixel_t makePixel( uint8_t r8, uint8_t g8, uint8_t b8 )
{
	// inputs: 8b of each: red, green, blue
	const uint16_t r5 = (r8 & 0xf8)>>3; // keep 5b red
	const uint16_t g6 = (g8 & 0xfc)>>2; // keep 6b green
	const uint16_t b5 = (b8 & 0xf8)>>3; // keep 5b blue
	return (pixel_t)( (r5<<11) | (g6<<5) | b5 );
}

void rect( int y1, int y2, int x1, int x2, pixel_t c )
{
	for( int y=y1; y<y2; y++ )
		for( int x=x1; x<x2; x++ )
			drawPixel( y, x, c );
}

int main()
{
	pixel_t colour;
	const int half_y = MAX_Y/2;
	const int half_x = MAX_X/2;
	printf( "start\n" );
	rect( 0, MAX_Y, 0, MAX_X, blk );
	for( int y=0; y<half_y; y++ ) {
		for( int x=0; x<half_x; x++ ) {
			// red, green, blue and white colour bars
			const uint32_t scale = 256 * x / half_x; // scale = 0..255
			colour = makePixel(     0,     0, scale ); drawPixel( y       , x       , colour );
			colour = makePixel(     0, scale,     0 ); drawPixel( y       , x+half_x, colour );
			colour = makePixel( scale,     0,     0 ); drawPixel( y+half_y, x       , colour );
			colour = makePixel( scale, scale, scale ); drawPixel( y+half_y, x+half_x, colour );
			delay(10000);
		}
	}
	printf( "done\n" );
}
