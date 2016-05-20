#define BUF_SIZE 500000			// about 10 seconds of buffer (@ 48K samples/sec)
#define BUF_THRESHOLD 96		// 75% of 128 word buffer

#define GRID_PRIORITY      2
#define INVOER_PRIORITY     1
#define SPELER_PRIORITY		4

//player number
#define SPELER1 		0
#define SPELER2 		1
#define SPELER3 		2
#define SPELER4 		3

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
void Game_Reset( void );

#define   TASK_STACKSIZE       2048
#define	  GRIDTASK_STACKSIZE	131072
OS_STK    speler1_stk[TASK_STACKSIZE];
OS_STK    speler2_stk[TASK_STACKSIZE];
OS_STK    speler3_stk[TASK_STACKSIZE];
OS_STK    speler4_stk[TASK_STACKSIZE];
OS_STK    invoer_stk[TASK_STACKSIZE];

ALT_SEM(resetWaarde)
ALT_SEM(pos)
ALT_SEM(af)
ALT_SEM(states)
ALT_SEM(bezig)
ALT_SEM(spelerScore)

int hoogte[4];
int lengte[4];
int spelerAf[4];
int score[4];
int resetten = 0;
int statePlus;
int stateMin;

int coords [115][115];
int coords1;
int coords2;

void speler(void* pdata)
{
	int l;
	int h;
	int state;
	int i;
	int spelerID = (int)(pdata);
	unsigned int kleur;
	char punten[40];
	int af;
	spelerAf[spelerID]=0;

	if(spelerID==0){
		l= 95;
		h = 15;
		state = 1;
		lengte[spelerID] = 5;
		hoogte[spelerID] = 5;
		kleur = 0x0F00;
		VGA_text(3, ((spelerID+1) * 6), "Player 1 \0");
	}else if(spelerID==1){
		l= 295;
		h = 15;
		state = 1;
		lengte[spelerID] = 105;
		hoogte[spelerID] = 5;
		kleur = 0xF000;
		VGA_text(3, ((spelerID+1) * 6), "Player 2 \0");
	}else if(spelerID==2){
		l= 95;
		h = 215;
		state = 3;
		lengte[spelerID] = 5;
		hoogte[spelerID] = 105;
		kleur = 0x00FF;
		VGA_text(3, ((spelerID+1) * 6), "Player 3 \0");
	}else if(spelerID==3){
		l= 295;
		h = 215;
		state = 3;
		lengte[spelerID] = 105;
		hoogte[spelerID] = 105;
		kleur = 0xFF00;
		VGA_text(3, ((spelerID+1) * 6), "Player 4 \0");
	}


	while (1)
	{
		ALT_SEM_PEND(pos, 0);

		if(hoogte[spelerID] >=115 || hoogte[spelerID] <=0)
		{
			af = 1;
		} else if (lengte[spelerID] >=115 || lengte[spelerID] <=0)
		{
			af = 1;
		} else if (coords[hoogte[spelerID]][lengte[spelerID]]==1)
		{
			af = 1;
		} else
		{
			coords[hoogte[spelerID]][lengte[spelerID]]=1;
		}

		ALT_SEM_POST(pos);
		ALT_SEM_PEND(spelerScore,0);
		if(af == 1){
			ALT_SEM_PEND(af, 0);
			if(spelerAf[0] == 0){
				spelerAf[0] = spelerID;
			}else if(spelerAf[1] == 0){
				spelerAf[1] = spelerID;
				score[spelerID] = score[spelerID] +1;
			}else if(spelerAf[2] == 0){
				spelerAf[2] = spelerID;
				score[spelerID] = score[spelerID] +2;
			}else{
				spelerAf[3] = spelerID;
				score[spelerID] = score[spelerID] +3;
			}

			sprintf(punten,"%d \0", score[spelerID]);

			VGA_text(3, (((spelerID+1) * 6)+2), "GAME OVER \0");
			VGA_text(18, ((spelerID+1) * 6), punten);

			ALT_SEM_POST(spelerScore);
			ALT_SEM_POST(af);

			OSTaskDel(OS_PRIO_SELF);
		}
		ALT_SEM_POST(spelerScore);

//			ALT_SEM_PEND(resetWaarde,0);
//			if(resetten>=1){
//			resetten=0;
//			}
//			ALT_SEM_POST(resetWaarde);

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
				VGA_box(l,h,l,h,kleur);
				OSTimeDlyHMSM(0, 0, 0, 500);
			}
			//opslaan in grid
			ALT_SEM_PEND(pos, 0);

			hoogte[spelerID]++;

			ALT_SEM_POST(pos);
		} else if(state == 2)		//rechts
		{
			for(i = 0; i < 2; i++)
			{
				l++;
				//tekenen locatie
				VGA_box(l,h,l,h,kleur);
				OSTimeDlyHMSM(0, 0, 0, 500);
			}
			//opslaan in grid
			ALT_SEM_PEND(pos, 0);

			lengte[spelerID]++;

			ALT_SEM_POST(pos);
		} else if(state == 3)		// omhoog
		{
			for(i = 0; i < 2; i++)
			{
				h--;
				//tekenen locatie
				VGA_box(l,h,l,h,kleur);
				OSTimeDlyHMSM(0, 0, 0, 500);
			}
			//opslaan in grid
			ALT_SEM_PEND(pos, 0);

			hoogte[spelerID]--;

			ALT_SEM_POST(pos);
		} else if(state == 4)		//links
		{
			for(i = 0; i < 2; i++)
			{
				l--;
				//tekenen locatie
				VGA_box(l,h,l,h,kleur);
				OSTimeDlyHMSM(0, 0, 0, 500);
			}
			//opslaan in grid
			ALT_SEM_PEND(pos, 0);

			lengte[spelerID]--;

			ALT_SEM_POST(pos);
		}

		//vertraging om andere tasks tijd te geven
		OSTimeDlyHMSM(0, 0, 0, 500);
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
	ALT_SEM_CREATE(&spelerScore, 1);


	Game_Reset();


	OSTaskCreateExt(speler,SPELER1,(void *)&speler1_stk[TASK_STACKSIZE-1],SPELER_PRIORITY,SPELER_PRIORITY,speler1_stk,TASK_STACKSIZE,NULL,0);

	OSTaskCreateExt(speler,SPELER2,(void *)&speler2_stk[TASK_STACKSIZE-1],SPELER_PRIORITY+1,SPELER_PRIORITY+1,speler2_stk,TASK_STACKSIZE,NULL,0);

	OSTaskCreateExt(speler,SPELER3,(void *)&speler3_stk[TASK_STACKSIZE-1],SPELER_PRIORITY+2,SPELER_PRIORITY+2,speler3_stk,TASK_STACKSIZE,NULL,0);

	OSTaskCreateExt(speler,SPELER4,(void *)&speler4_stk[TASK_STACKSIZE-1],SPELER_PRIORITY+3,SPELER_PRIORITY+3,speler4_stk,TASK_STACKSIZE,NULL,0);

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




/****************************************************************************************
 * Reset the array
 ****************************************************************************************/
void Game_Reset(void){
	int coord1;
	int coord2;
	int i;
	int j;
	// shows game is resetting
	VGA_text(5, 3, "RESETTING \0");

	for(coord1=0;coord1<115;coord1++)
	{
		for(coord2=0;coord2<115;coord2++)
		{
			coords[coord1][coord2] = 0;
		}
	}
	for(i=0;i<3;i++){
		spelerAf[i]=0;
	}

	 VGA_text(3, 6, "                \0");
	 VGA_text(3, 8, "                \0");
	 VGA_text(3, 12, "                \0");
	 VGA_text(3, 14, "                \0");
	 VGA_text(3, 18, "                \0");
	 VGA_text(3, 20, "                \0");
	 VGA_text(3, 24, "                \0");
	 VGA_text(3, 26, "                \0");

	 ALT_SEM_PEND(spelerScore,0);
	 for(j = 0; j < 5; j++){
		 score[j-1] = 0;
	 }
	 ALT_SEM_POST(spelerScore);

	//Blue borders
	VGA_box(0,0,3,239,0x22F0);
	VGA_box(0,0,319,3,0x22F0);
	VGA_box(81,0,83,239,0x22F0);
	VGA_box(0,237,319,239,0x22F0);
	VGA_box(317,0,319,239,0x22F0);

	//lightblue borders
	VGA_box(4,4,4,236,0x47FF);
	VGA_box(4,4,80,4,0x47FF);
	VGA_box(84,4,316,4,0x47FF);
	VGA_box(80,4,80,236,0x47FF);
	VGA_box(84,4,84,236,0x47FF);
	VGA_box(4,236,80,236,0x47FF);
	VGA_box(84,236,316,236,0x47FF);
	VGA_box(316,4,316,236,0x47FF);


	//Score and Play area
	VGA_box(5,5,79,235,0);
	VGA_box(85,5,315,235,0x0000);

	//clear game over
	VGA_text(3,3,"                            \0");
}
