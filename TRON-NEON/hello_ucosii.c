/* globals */
#define BUF_SIZE 500000			// about 10 seconds of buffer (@ 48K samples/sec)
#define BUF_THRESHOLD 96		// 75% of 128 word buffer

/* Definition of Task Priorities */
#define _PRIORITY      1
#define TASK2_PRIORITY      2
#define TASK3_PRIORITY      3
#define TASK4_PRIORITY      15

#define FLAG_1 0x01

#include <stdio.h>
#include "includes.h"
#include "altera_up_avalon_character_lcd.h"
#include "altera_up_avalon_parallel_port.h"
#include "string.h"
#include <os/alt_sem.h>
#include <os/alt_flag.h>


/* function prototypes */
void LCD_cursor( int, int );
void LCD_text( char * );
void LCD_cursor_off( void );
void VGA_text (int, int, char *);
void VGA_box (int, int, int, int, short);
void HEX_PS2(char, char, char);
void check_KEYs( int *, int *, int * );

/* Definition of Task stacksize */
#define   TASK_STACKSIZE       2048
OS_STK    _stk[TASK_STACKSIZE];
OS_STK    task2_stk[TASK_STACKSIZE];
OS_STK    task3_stk[TASK_STACKSIZE];
OS_STK    task4_stk[TASK_STACKSIZE];


/*intitalizing semaphores */
ALT_SEM(reset) //for reset
ALT_SEM(resetWaarde)
ALT_SEM(pos)
ALT_SEM(af)
ALT_SEM(stapje)

int h;
int l;
int spelerAf;
int resetten;

/* Prints "Hello World" and sleeps for three seconds */
void Achtergrond (void* pdata)
{
  VGA_box(0,0,319,239,0x00F0); //clear screen

  while (1)
  {
	  ALT_SEM_PEND(reset, 0);// whenever the semaphore reset is posted, this will clear the screen and set default background.

	 //VGA_box(0,0,319,239,0x47FF);
	 VGA_box(0,0,319,239,0xFD22);
	 VGA_box(4,4,79,234,0);
	 VGA_box(84,4,314,234,0x0000);
	 VGA_text(3,3,"                            \0");



  }
}

void task2(void* pdata)
{
	volatile int * KEY_ptr = (int *) 0x10000050;
	int KEY_value;
	ALT_SEM_PEND(pos, 0);
	l= 100;
	h = 14;
	ALT_SEM_POST(pos);
	int state = 1;
  while (1)
  {
	  	KEY_value = *(KEY_ptr + 3);			// read the pushbutton interrupt register
	  	*(KEY_ptr + 3) = 0; 						// Clear the interrupt

	  	if (KEY_value == 0x4)					// check KEY2
	  	{
			state--;
	  	}
	  	else if (KEY_value == 0x8)				// check KEY3
	  	{
			state++;
	  	}

	  	// state reset
	  	 if(state >=5){
	  				state = 1;
	  	} else if(state<=0){
	  				state = 4;
	  	}

	  	ALT_SEM_PEND(pos, 0);

	  	if(state == 1){ //omlaag
	  		//for(i=0;i<=1;i++){
	  			h++;
				VGA_box(l,h,l,h,0x0F00);
	  		//}
		} else if(state == 2){ //rechts
			//for(i=0;i<=1;i++){
				l++;
				VGA_box(l,h,l,h,0x0F00);
			//}
		} else if(state == 3){ // omhoog
			//for(i=0;i<=1;i++){
				h--;
				VGA_box(l,h,l,h,0x0F00);
			//}
		} else if(state == 4){ //links
			//for(i=0;i<=1;i++){
				l--;
				VGA_box(l,h,l,h,0x0F00);
			//}
		}

	  	ALT_SEM_POST(pos);

	  	// border hit detection
	  	ALT_SEM_PEND(af,0);

	  	if(spelerAf==1){
	  		VGA_text(5 ,3,"GAME OVER \0");
	  		OSTaskDel(OS_PRIO_SELF);
	  	}
	  	ALT_SEM_POST(af);


    OSTimeDlyHMSM(0, 0, 0, 500);
  }
}

void task3(void* pdata)
{
	int coords [230][230];

	int coord1;
	int coord2;


	while (1)
	{
		ALT_SEM_PEND(resetWaarde,0);
		if(resetten == 1){
			for(coord1 = 0; coord1<230;coord1++ ){
				for(coord2 = 0; coord2<230;coord2++ ){
					coords[coord1][coord2]=0;
				}
			}
		}
		ALT_SEM_POST(resetWaarde);

		ALT_SEM_PEND(pos, 0);

		if(coords[h-84][l-4]==1){
			ALT_SEM_PEND(af, 0);
			spelerAf = 1;
			ALT_SEM_POST(af);
		} else if(h>=234 || h<=4 || l>=314 || l<=84){
			ALT_SEM_PEND(af, 0);
			spelerAf = 1;
			ALT_SEM_POST(af);
		} else{
			coords[h-84][l-4]=1;
		}

		ALT_SEM_POST(pos);

		OSTimeDlyHMSM(0, 0, 0, 500);
	}
}

void task4(void* pdata)
{

	volatile int * KEY_ptr = (int *) 0x10000050;
	int KEY_value;

	while (1)
	{
		KEY_value = *(KEY_ptr + 3);			// read the pushbutton interrupt register
		*(KEY_ptr + 3) = 0; 						// Clear the interrupt

		if (KEY_value == 0x2)					// check KEY2
		{
			ALT_SEM_PEND(resetWaarde,0);
			resetten = 1;
			ALT_SEM_POST(resetWaarde);
			ALT_SEM_POST(reset);

		}

		OSTimeDlyHMSM(0, 0, 0, 250);
	}
}

/* The main function creates two task and starts multi-tasking */
int main(void)
{

	LCD_cursor_off(); //clears lcd screen of cursor
	//extern volatile int buffer_index;
	ALT_SEM_CREATE(&reset, 1);
	ALT_SEM_CREATE(&pos, 1);
	ALT_SEM_CREATE(&af, 1);
	ALT_SEM_CREATE(&stapje, 1);
	ALT_SEM_CREATE(&resetWaarde, 1);


	OSTaskCreateExt(Achtergrond,NULL,(void *)&_stk[TASK_STACKSIZE-1],_PRIORITY, _PRIORITY,_stk,TASK_STACKSIZE,NULL,0);

	OSTaskCreateExt(task2,NULL,(void *)&task2_stk[TASK_STACKSIZE-1],TASK2_PRIORITY,TASK2_PRIORITY,task2_stk,TASK_STACKSIZE,NULL,0);
	OSTaskCreateExt(task3,NULL,(void *)&task3_stk[TASK_STACKSIZE-1],TASK3_PRIORITY,TASK3_PRIORITY,task3_stk,TASK_STACKSIZE,NULL,0);
	//OSTaskCreateExt(task4,NULL,(void *)&task4_stk[TASK_STACKSIZE-1],TASK4_PRIORITY,TASK4_PRIORITY,task4_stk,TASK_STACKSIZE,NULL,0);

	OSStart();
	return 0;
}

/****************************************************************************************
 * Subroutine to move the LCD cursor
****************************************************************************************/
void LCD_cursor(int x, int y)
{
  	volatile char * LCD_display_ptr = (char *) 0x10003050;	// 16x2 character display
	char instruction;

	instruction = x;
	if (y != 0) instruction |= 0x40;				// set bit 6 for bottom row
	instruction |= 0x80;								// need to set bit 7 to set the cursor location
	*(LCD_display_ptr) = instruction;			// write to the LCD instruction register
}

/****************************************************************************************
 * Subroutine to send a string of text to the LCD
****************************************************************************************/
void LCD_text(char * text_ptr)
{
  	volatile char * LCD_display_ptr = (char *) 0x10003050;	// 16x2 character display

	while ( *(text_ptr) )
	{
		*(LCD_display_ptr + 1) = *(text_ptr);	// write to the LCD data register
		++text_ptr;
	}
}

/****************************************************************************************
 * Subroutine to turn off the LCD cursor
****************************************************************************************/
void LCD_cursor_off(void)
{
  	volatile char * LCD_display_ptr = (char *) 0x10003050;	// 16x2 character display
	*(LCD_display_ptr) = 0x0C;											// turn off the LCD cursor
}

/****************************************************************************************
 * Subroutine to send a string of text to the VGA monitor
****************************************************************************************/
void VGA_text(int x, int y, char * text_ptr)
{
	int offset;
  	volatile char * character_buffer = (char *) 0x09000000;	// VGA character buffer

	/* assume that the text string fits on one line */
	offset = (y << 7) + x;
	while ( *(text_ptr) )
	{
		*(character_buffer + offset) = *(text_ptr);	// write to the character buffer
		++text_ptr;
		++offset;
	}
}

/****************************************************************************************
 * Draw a filled rectangle on the VGA monitor
****************************************************************************************/
void VGA_box(int x1, int y1, int x2, int y2, short pixel_color)
{
	int offset, row, col;
  	volatile short * pixel_buffer = (short *) 0x08000000;	// VGA pixel buffer

	/* assume that the box coordinates are valid */
	for (row = y1; row <= y2; row++)
	{
		col = x1;
		while (col <= x2)
		{
			offset = (row << 9) + col;
			*(pixel_buffer + offset) = pixel_color;	// compute halfword address, set pixel
			++col;
		}
	}
}

/****************************************************************************************
 * Subroutine to show a string of HEX data on the HEX displays
****************************************************************************************/
void HEX_PS2(char b1, char b2, char b3)
{
	volatile int * HEX3_HEX0_ptr = (int *) 0x10000020;
	volatile int * HEX7_HEX4_ptr = (int *) 0x10000030;

	/* SEVEN_SEGMENT_DECODE_TABLE gives the on/off settings for all segments in
	 * a single 7-seg display in the DE2 Media Computer, for the hex digits 0 - F */
	unsigned char	seven_seg_decode_table[] = {	0x3F, 0x06, 0x5B, 0x4F, 0x66, 0x6D, 0x7D, 0x07,
		  										0x7F, 0x67, 0x77, 0x7C, 0x39, 0x5E, 0x79, 0x71 };
	unsigned char	hex_segs[] = { 0, 0, 0, 0, 0, 0, 0, 0 };
	unsigned int shift_buffer, nibble;
	unsigned char code;
	int i;

	shift_buffer = (b1 << 16) | (b2 << 8) | b3;
	for ( i = 0; i < 6; ++i )
	{
		nibble = shift_buffer & 0x0000000F;		// character is in rightmost nibble
		code = seven_seg_decode_table[nibble];
		hex_segs[i] = code;
		shift_buffer = shift_buffer >> 4;
	}
	/* drive the hex displays */
	*(HEX3_HEX0_ptr) = *(int *) (hex_segs);
	*(HEX7_HEX4_ptr) = *(int *) (hex_segs+4);
}

/****************************************************************************************
 * Subroutine to read KEYs
****************************************************************************************/
void check_KEYs(int * KEY1, int * KEY2, int * counter)
{
	volatile int * KEY_ptr = (int *) 0x10000050;		// pushbutton KEY address
	volatile int * audio_ptr = (int *) 0x10003040;	// audio port address
	int KEY_value;

	KEY_value = *(KEY_ptr); 				// read the pushbutton KEY values
	while (*KEY_ptr);							// wait for pushbutton KEY release

	if (KEY_value == 0x2)					// check KEY1
	{
		// reset counter to start recording
		*counter = 0;
		// clear audio-in FIFO
		*(audio_ptr) = 0x4;
		*(audio_ptr) = 0x0;

		*KEY1 = 1;
	}
	else if (KEY_value == 0x4)				// check KEY2
	{
		// reset counter to start playback
		*counter = 0;
		// clear audio-out FIFO
		*(audio_ptr) = 0x8;
		*(audio_ptr) = 0x0;

		*KEY2 = 1;
	}
}
