#define BUF_SIZE 500000			// about 10 seconds of buffer (@ 48K samples/sec)
#define BUF_THRESHOLD 96		// 75% of 128 word buffer

#define ACHTERGROND_PRIORITY      3
#define SPELER_PRIORITY      7
#define GRID_PRIORITY      6
#define INVOER_PRIORITY      4

#include <stdio.h>
#include "includes.h"
#include "altera_up_avalon_character_lcd.h"
#include "altera_up_avalon_parallel_port.h"
#include "string.h"
#include <os/alt_sem.h>
#include <os/alt_flag.h>

void LCD_cursor_off( void );
void VGA_text (int, int, char *);
void VGA_box (int, int, int, int, short);

#define   TASK_STACKSIZE       2048
#define	  GRIDTASK_STACKSIZE	131072
OS_STK    achtergrond_stk[TASK_STACKSIZE];
OS_STK    speler_stk[TASK_STACKSIZE];
OS_STK    grid_stk[GRIDTASK_STACKSIZE];
OS_STK    invoer_stk[TASK_STACKSIZE];

ALT_SEM(resetWaarde)
ALT_SEM(pos)
ALT_SEM(af)
ALT_SEM(states)
ALT_SEM(bezig)

int hoogte;
int lengte;
int spelerAf;
int resetten = 0;
int statePlus;
int stateMin;

int coords [115][115];
int coords1;
int coords2;

void Achtergrond (void* pdata)
{
	//blauwe randjes
	VGA_box(0,0,3,239,0x47FF);
	VGA_box(0,0,319,3,0x47FF);
	VGA_box(80,0,83,239,0x47FF);
	VGA_box(0,236,319,239,0x47FF);
	VGA_box(316,0,319,239,0x47FF);

	//zwarte vlakken
	VGA_box(4,4,79,235,0);
	VGA_box(84,4,315,235,0);
	VGA_text(3,3,"                            \0");
	VGA_text(3,5,"                            \0");

	OSTaskDel(OS_PRIO_SELF);

}

void speler(void* pdata)
{

	int l= 95;
	int h = 15;
	int state = 1;
	int i;
	lengte = 5;
	hoogte = 5;

	while (1)
	{
		// border hit detection
		ALT_SEM_PEND(af, 0);

		if (spelerAf == 1)
		{
			VGA_text(5, 3, "GAME OVER \0");
			//OSTaskDel(OS_PRIO_SELF);
		}

		ALT_SEM_POST(af);

		ALT_SEM_PEND(states, 0);
	  	if (stateMin)
	  	{
			stateMin = 0;
			state--;
	  	}
	  	else if (statePlus)
	  	{
			statePlus = 0;
			state++;
	  	}
		ALT_SEM_POST(states);

		// state reset
	  	if(state >=5)
		{
			state = 1;
	  	} else if(state<=0)
		{
			state = 4;
	  	}

		//Veranderen locatie

		if(state == 1)		//omlaag
		{
			for(i = 0; i < 2; i++)
			{
				h++;
				//tekenen locatie
				VGA_box(l,h,l,h,0x0F00);
				OSTimeDlyHMSM(0, 0, 0, 500);
			}
			//opslaan in grid
			ALT_SEM_PEND(pos, 0);

			hoogte++;

			ALT_SEM_POST(pos);
		} else if(state == 2)		//rechts
		{
			for(i = 0; i < 2; i++)
			{
				l++;
				//tekenen locatie
				VGA_box(l,h,l,h,0x0F00);
				OSTimeDlyHMSM(0, 0, 0, 500);
			}
			//opslaan in grid
			ALT_SEM_PEND(pos, 0);

			lengte++;

			ALT_SEM_POST(pos);
		} else if(state == 3)		// omhoog
		{
			for(i = 0; i < 2; i++)
			{
				h--;
				//tekenen locatie
				VGA_box(l,h,l,h,0x0F00);
				OSTimeDlyHMSM(0, 0, 0, 500);
			}
			//opslaan in grid
			ALT_SEM_PEND(pos, 0);

			hoogte--;

			ALT_SEM_POST(pos);
		} else if(state == 4)		//links
		{
			for(i = 0; i < 2; i++)
			{
				l--;
				//tekenen locatie
				VGA_box(l,h,l,h,0x0F00);
				OSTimeDlyHMSM(0, 0, 0, 500);
			}
			//opslaan in grid
			ALT_SEM_PEND(pos, 0);

			lengte--;

			ALT_SEM_POST(pos);
		}

		//printf("%d & %d \n", hoogte, lengte);

		//vertraging om andere tasks tijd te geven
		OSTimeDlyHMSM(0, 0, 0, 500);
	}
}

void grid(void* pdata)
{
	while (1)
	{
		//opslaan locaties in array
		ALT_SEM_PEND(pos, 0);

		if(hoogte >=115 || hoogte <=0)
		{
			ALT_SEM_PEND(af, 0);
			spelerAf = 1;
			ALT_SEM_POST(af);
		} else if (lengte  >=115 || lengte <=0)
		{
			ALT_SEM_PEND(af, 0);
			spelerAf = 1;
			ALT_SEM_POST(af);
		} else if (coords[hoogte][lengte]==1)
		{
			ALT_SEM_PEND(af, 0);
			spelerAf = 1;
			ALT_SEM_POST(af);
		} else
		{
			coords[hoogte][lengte]=1;
		}

		ALT_SEM_POST(pos);

		//vertraging om andere tasks tijd te geven
		OSTimeDlyHMSM(0, 0, 1, 0);
	}
}

void invoer(void* pdata)
{
	volatile int * KEY_ptr = (int *)0x10000050;
	int KEY_value;

	while (1) {
		KEY_value = *(KEY_ptr + 3);			// read the pushbutton interrupt register
		*(KEY_ptr + 3) = 0; 						// Clear the interrupt


		if (KEY_value == 0x2)					// check KEY1
		{
			ALT_SEM_PEND(resetWaarde, 0);
			resetten = 1;		//ga resetten
			ALT_SEM_POST(resetWaarde);

			//task om scherm te resetten
			OSTaskCreateExt(Achtergrond,NULL,(void *)&achtergrond_stk[TASK_STACKSIZE-1],ACHTERGROND_PRIORITY, ACHTERGROND_PRIORITY,achtergrond_stk,TASK_STACKSIZE,NULL,0);
		}
		if (KEY_value == 0x4)					// check KEY2
		{
			ALT_SEM_PEND(states, 0);
			stateMin = 1;		//state naar links
			ALT_SEM_POST(states);
		}
		else if (KEY_value == 0x8)				// check KEY3
		{
			ALT_SEM_PEND(states, 0);
			statePlus=1;		//state naar rechts
			ALT_SEM_POST(states);
		}

		OSTimeDlyHMSM(0, 0, 0, 70);
	}
}


int main(void)
{

	LCD_cursor_off();

	ALT_SEM_CREATE(&pos, 1);
	ALT_SEM_CREATE(&af, 1);
	ALT_SEM_CREATE(&states, 1);
	ALT_SEM_CREATE(&resetWaarde, 1);
	ALT_SEM_CREATE(&bezig, 1);

	for(coords1 =0; coords1 < 116; coords1++)
	{
		for(coords2 =0; coords2 < 116; coords2++)
		{
			coords[coords1][coords2] = 0;
			printf("%d & %d = %d \n",coords1, coords2, coords[coords1][coords2]);
		}
	}

	OSTaskCreateExt(Achtergrond,NULL,(void *)&achtergrond_stk[TASK_STACKSIZE-1],ACHTERGROND_PRIORITY, ACHTERGROND_PRIORITY,achtergrond_stk,TASK_STACKSIZE,NULL,0);
	OSTaskCreateExt(speler,NULL,(void *)&speler_stk[TASK_STACKSIZE-1],SPELER_PRIORITY,SPELER_PRIORITY,speler_stk,TASK_STACKSIZE,NULL,0);
	OSTaskCreateExt(grid,NULL,(void *)&grid_stk[GRIDTASK_STACKSIZE-1],GRID_PRIORITY,GRID_PRIORITY,grid_stk,GRIDTASK_STACKSIZE,NULL,0);
	OSTaskCreateExt(invoer, NULL, (void *)&invoer_stk[TASK_STACKSIZE - 1], INVOER_PRIORITY, INVOER_PRIORITY, invoer_stk, TASK_STACKSIZE, NULL, 0);

	OSStart();
	return 0;
}

//Vga controller hieronder
//		|
//		|
//	   \|/

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



