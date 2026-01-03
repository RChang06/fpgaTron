#include <unistd.h>
#include <stdio.h>
#include <stdbool.h>

// if using CPUlator, you should copy+paste contents of the file below instead of using #include
#include "address_map_niosv.h"




typedef uint16_t pixel_t;

volatile pixel_t *pVGA = (pixel_t *)FPGA_PIXEL_BUF_BASE;

#define MTIME_LO    (*(volatile uint32_t *)(MTIMER_BASE + 0x0))
#define MTIME_HI    (*(volatile uint32_t *)(MTIMER_BASE + 0x4))
#define MTIMECMP_LO (*(volatile uint32_t *)(MTIMER_BASE + 0x8))
#define MTIMECMP_HI (*(volatile uint32_t *)(MTIMER_BASE + 0xC))
#define LEDR   		(*(volatile uint32_t *)(LEDR_BASE))
#define HEX3  		(*(volatile uint32_t *)(HEX3_HEX0_BASE))
#define HEX5 		(*(volatile uint32_t *)(HEX5_HEX4_BASE))
#define SW (*(volatile uint32_t *)SW_BASE)




static uint32_t disable_interrupts(void) {
    uint32_t mstatus;
    __asm__ volatile("csrr %0, mstatus" : "=r"(mstatus)); // read mstatus
    uint32_t old_mie = mstatus & (1u << 3);               // check MIE bit
    mstatus &= ~(1u << 3);                                // clear MIE bit
    __asm__ volatile("csrw mstatus, %0" :: "r"(mstatus)); // write back
    return old_mie;
}

static void enable_interrupts(void) {
    uint32_t mstatus;
    __asm__ volatile("csrr %0, mstatus" : "=r"(mstatus)); // read mstatus
    mstatus |= (1u << 3);                                 // set MIE bit
    __asm__ volatile("csrw mstatus, %0" :: "r"(mstatus)); // write back
}

void setup_cpu_irqs(uint32_t mask) {
    __asm__ volatile("csrw mie, %0" :: "r"(mask));
}
uint64_t getGameSpeed(void) {
    uint32_t sw = SW & 0x3FF; // mask only SW[9:0]
    
    for (int i = 9; i >= 0; i--) {
        if (sw & (1 << i)) {
            return 400000000ULL >> (9 - i);
        }
    }
    
    return 400000000ULL >> 10; 
}


static uint64_t mtime_read(void) {
    uint32_t hi1, lo, hi2;
    do {
        hi1 = MTIME_HI;
        lo  = MTIME_LO;
        hi2 = MTIME_HI;
    } while (hi1 != hi2);
    return ((uint64_t)hi2 << 32) | lo;
}

/* Writes MTIMECMP 64-bit value atomically (HIGH then LOW) */
static void mtimecmp_write(uint64_t t) {
    uint32_t old_mie = disable_interrupts();
    MTIMECMP_HI = (uint32_t)(t >> 32);
    MTIMECMP_LO = (uint32_t)(t & 0xFFFFFFFFu);
    if (old_mie) {
        enable_interrupts();
    }
}




const pixel_t blk = 0x0000;
const pixel_t wht = 0xffff;
const pixel_t red = 0xf800;
const pixel_t grn = 0x07e0;
const pixel_t blu = 0x001f;

int posX1 = MAX_X/3;
int posY1 = MAX_Y/2;
int posX2 = MAX_X*2/3;
int posY2 = MAX_Y/2; 
volatile int win1 = 0;
volatile int win2 = 0; 
volatile bool gameOver = false;
volatile uint32_t last_key = 0;
int dirs1[] = {1, 0};
int dirs2[] = {-1, 0};
const uint8_t HEX_DIGITS[10] = {
    0x3F, // 0
    0x06, // 1
    0x5B, // 2
    0x4F, // 3
    0x66, // 4
    0x6D, // 5
    0x7D, // 6
    0x07, // 7
    0x7F, // 8
    0x6F  // 9
};
const uint8_t HEX_BLANK = 0x00;

//-1 = left, 0 = straight, 1 = right
volatile int turn1 = 0;
int turn2 = 0;
static volatile int prev = 0x3;

void pollKeysSimple() {
    int keys = *(volatile int*)KEY_BASE & 0x3;
    int falling = keys & ~prev;
    if (falling & 0x1) turn1 = -1;
    if (falling & 0x2) turn1 = 1;
    prev = keys;
}



void delay( int N )
{
	for( int i=0; i<N; i++ ) 
		*pVGA; // read volatile memory location to waste time
}
void updateScore(){
	HEX3 = HEX_DIGITS[win1];
	HEX5 = HEX_DIGITS[win2];
}


void update_LED(void) {
    uint32_t led = LEDR & ~0x3;  
    if (turn1 == -1) {
        led |= (1 << 1);  // set LEDR[1] (bit 1)
    } else if (turn1 == 1) {
        led |= (1 << 0);  // set LEDR[0] (bit 0)
    }
    LEDR = led;
}



void drawPixel( int y, int x, pixel_t colour )
{
	*(pVGA + (y<<YSHIFT) + x ) = colour;
}

pixel_t makePixel( uint8_t r8, uint8_t g8, uint8_t b8 )
{
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
void perimeter(){
	    for (int x = 0; x < MAX_X; x++) {
        drawPixel(0, x, wht);
    }

    for (int x = 0; x < MAX_X; x++) {
        drawPixel(MAX_Y - 1, x, wht);
    }

    for (int y = 1; y < MAX_Y - 1; y++) {
        drawPixel(y, 0, wht);
    }

    for (int y = 1; y < MAX_Y - 1; y++) {
        drawPixel(y, MAX_X - 1, wht);
    }
}
pixel_t getColour(int x, int y){
	return *(pVGA + (y<<YSHIFT)+x);
}
bool checkCollision(int x,int y){
	if (getColour(x, y) == wht || getColour (x , y) == blu || getColour(x , y) == red){
		return true; 
	}
	else{
		return false; 
	}
}
void replay(){
	rect( 0, MAX_Y, 0, MAX_X, blk );
	rect(0, 10, 0, 10, wht);
	perimeter();
	posX1 = MAX_X/3;
	posY1 = MAX_Y/2;
	posX2 = MAX_X*2/3;
	posY2 = MAX_Y/2; 
	dirs1[0] = 1; dirs1[1] = 0;   
    dirs2[0] = -1; dirs2[1] = 0;  
    turn1 = 0;
    turn2 = 0;
}
void updateColour( ){
	drawPixel(posY1, posX1, red);
	drawPixel(posY2, posX2, blu);
}
bool loseLogic(bool lose1, bool lose2){
	if (lose1 && lose2){

		replay();
		return true; 
	}
	else if (lose1){
		win2++;
		updateScore();
		replay();
		return true;
	}
	else if (lose2){
		win1++;
		updateScore();
		replay();
		return true; 
	}
	else{
		
		updateColour();
		return false; 
	}
}
bool winBreak(){
    if (win1 >= 9 || win2 >= 9){
        if (win1 >= 9){
            rect(0, MAX_Y, 0, MAX_X, red);
        }
        else{
            rect(0, MAX_Y, 0, MAX_X, blu);
        }
        gameOver = true;
        return true; 
    }
    return false; 
}
void applyTurn(int dirs[2], int turn) {
    if (turn == -1) {  // Turn left
        int temp = dirs[0];
        dirs[0] = -dirs[1];
        dirs[1] = temp;
    } else if (turn == 1) {  // Turn right
        int temp = dirs[0];
        dirs[0] = dirs[1];
        dirs[1] = -temp;
    }
}
void player2NextDir(){
	
	int tempPosX = posX2+dirs2[0];
	int tempPosY = posY2+dirs2[1];
	if (!checkCollision(tempPosX, tempPosY)){
		turn2 = 0;
		return; 
	}
	//check left
	 tempPosX = posX2 + -dirs2[1];
	 tempPosY = posY2 + dirs2[0];
	 if (!checkCollision(tempPosX, tempPosY)){
		turn2 = -1;
		return; 
	}
	//check right
	tempPosX = posX2 + dirs2[1];
	tempPosY = posY2 - dirs2[0];
	if (!checkCollision(tempPosX, tempPosY)){
		turn2 = 1;
		return; 
	}

}

void updatePos() {
    applyTurn(dirs1, turn1);
    posX1 += dirs1[0];
    posY1 += dirs1[1];

    applyTurn( dirs2, turn2);
    posX2 += dirs2[0];
    posY2 += dirs2[1];
    turn1 = 0;
    turn2 = 0;
	last_key = 0; 
}


void handler(void) __attribute__((interrupt("machine")));
void handler(void) {
    uint32_t cause;
    __asm__ volatile("csrr %0, mcause" : "=r"(cause));
    if (cause & 0x80000000) { 
        uint32_t code = cause & 0x7FFFFFFF;
        if (code == 7) {          // Machine timer IRQ
            isr_timer();
        } else if (code == 18) {   // External IRQ for KEY
            isr_key();
        }
    }
}

static void init_interrupts(void) {
    uintptr_t handler_addr = (uintptr_t)&handler;
    __asm__ volatile("csrw mtvec, %0" :: "r"(handler_addr));
    uint32_t irq_mask = (1u << 7) | (1u << 18); // Enable timer and KEY IRQs
    setup_cpu_irqs(irq_mask);
    enable_interrupts();
}

void isr_key(void) {
    uint32_t edges = *(volatile uint32_t*)(KEY_BASE + 0xC);
    *(volatile uint32_t*)(KEY_BASE + 0xC) = edges;   // clear edges

    // Convert edges to active-high
    edges = ~edges & 0xF;
    
    // Check for KEY0 press (left turn)
    if (edges & 0x1) {
        if (last_key == 0x1) {
            turn1 = 0;
        } else {
            turn1 = -1;
        }
        last_key = 0x1;
    }
    else if (edges & 0x2) {
        if (last_key == 0x2) {
            turn1 = 0;
        } else {
            turn1 = 1;
        }
        last_key = 0x2;
    }
    
    update_LED();
}


void isr_timer(void) {
	*(volatile uint32_t*)(TIMER_BASE + 0x0C) = 1;  // clear timeout bit
	if (gameOver) {
        uint32_t mie;
        __asm__ volatile("csrr %0, mie" : "=r"(mie));
        mie &= ~(1u << 7); 
        __asm__ volatile("csrw mie, %0" :: "r"(mie));
        return; 
    }


    uint64_t now = mtime_read();
    mtimecmp_write(now + getGameSpeed()); 

    
    player2NextDir();

    bool lose1 = checkCollision(posX1, posY1);
    bool lose2 = checkCollision(posX2, posY2);
    
    if (loseLogic(lose1, lose2)){
        return; 
    }
    
    updatePos(); 
	update_LED();
}



int main() {
    printf( "start\n" );
    replay();
	update_LED();


    uint64_t now = mtime_read();
    mtimecmp_write(now + 5000000ULL); 
	*(volatile uint32_t*)(KEY_BASE + 0x8) = 0x3;
	*(volatile uint32_t*)(KEY_BASE + 0xC) = 0x3;  // KEY_INTMASK = 0b11

    init_interrupts(); // Enable global interrupts

	
    while (true) {
		if (winBreak()) {
			break; // Exit loop if game is over
		}

    }

    printf( "done game\n" );
    while(1); // Halt after game over
}

// int main()
// {
// 	printf( "start\n" );
// 	replay();
// 	while (true){
// 		if (winBreak()){
// 			break; 
// 		}
// 		pollKeysSimple();
// 		player2NextDir();

// 		bool lose1 = checkCollision(posX1, posY1);
// 		bool lose2 = checkCollision(posX2, posY2);
// 		if (loseLogic(lose1, lose2)){
// 			continue; 
// 		}
		
// 		updatePos();
// 		delay(500000);
// 	}
	

// 	printf( "done game\n" );
// }
