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
ALT_SEM(state)

int h;
int l;
int spelerAf;
int resetten;
int statePlus;
int stateMin;

/* Prints "Hello World" and sleeps for three seconds */
void Achtergrond (void* pdata)
{
  VGA_box(0,0,319,239,0x00F0); //clear screen

  while (1)
  {
	  ALT_SEM_PEND(reset, 0);// whenever the semaphore reset is posted, this will clear the screen and set default background.

	 VGA_box(0,0,319,239,0x47FF);
	 //VGA_box(0,0,319,239,0xFD22);
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

		ALT_SEM_PEND(state, 0);
	  	if (stateMin)					// check KEY2
	  	{
			stateMin = 0;
			state--;
	  	}
	  	else if (statePlus)				// check KEY3
	  	{
			statePlus = 0;
			state++;
	  	}
		ALT_SEM_POST(state);
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
	  		//OSTaskDel(OS_PRIO_SELF);
	  	}
	  	ALT_SEM_POST(af);


    OSTimeDlyHMSM(0, 0, 0, 500);
  }
}

void task3(void* pdata)
{
	int coords [230][230];
	int (*p)[230]= coords;

	int coord1;
	int coord2;

	while (1)
	{
		ALT_SEM_PEND(resetWaarde,0);
		if(resetten == 1){
			for(coord1 = 0; coord1<230;coord1++ ){
				for (coord2 = 0; coord2<230;coord2++){
					p[coord1][coord2] = 0;
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
	volatile int * KEY_ptr = (int *)0x10000050;
	int KEY_value;

	while (1) {
		KEY_value = *(KEY_ptr + 3);			// read the pushbutton interrupt register
		*(KEY_ptr + 3) = 0; 						// Clear the interrupt


		if (KEY_value == 0x2)					// check KEY1
		{
			ALT_SEM_PEND(resetWaarde, 0);
			resetten = 1;
			ALT_SEM_POST(resetWaarde);
			ALT_SEM_POST(reset);
		}
		if (KEY_value == 0x4)					// check KEY2
		{
			ALT_SEM_PEND(state, 0);
			stateMin = 1;
			ALT_SEM_POST(state);
		}
		else if (KEY_value == 0x8)				// check KEY3
		{
			ALT_SEM_PEND(state, 0);
			statePlus=1;
			ALT_SEM_POST(state);
		}


		OSTimeDlyHMSM(0, 0, 0, 70);
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
	ALT_SEM_CREATE(&resetWaarde, 1);
	ALT_SEM_CREATE(&state, 1);

	OSTaskCreateExt(Achtergrond,NULL,(void *)&_stk[TASK_STACKSIZE-1],_PRIORITY, _PRIORITY,_stk,TASK_STACKSIZE,NULL,0);

	OSTaskCreateExt(task2,NULL,(void *)&task2_stk[TASK_STACKSIZE-1],TASK2_PRIORITY,TASK2_PRIORITY,task2_stk,TASK_STACKSIZE,NULL,0);
	OSTaskCreateExt(task3,NULL,(void *)&task3_stk[TASK_STACKSIZE-1],TASK3_PRIORITY,TASK3_PRIORITY,task3_stk,TASK_STACKSIZE,NULL,0);
	OSTaskCreateExt(task4, NULL, (void *)&task4_stk[TASK_STACKSIZE - 1], TASK4_PRIORITY, TASK4_PRIORITY, task4_stk, TASK_STACKSIZE, NULL, 0);

	OSStart();
	return 0;
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

