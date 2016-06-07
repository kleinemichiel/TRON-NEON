#define BUF_SIZE 500000			// about 10 seconds of buffer (@ 48K samples/sec)
#define BUF_THRESHOLD 96		// 75% of 128 word buffer

#define INVOER_PRIORITY     1
#define SPELER_PRIORITY		4
#define MENU_PRIORITY		3

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

alt_up_parallel_port_dev *gpio_dev;

void LCD_cursor_off( void );
void VGA_text (int, int, char *);
void VGA_box (int, int, int, int, short);
void Game_Reset( void );

#define   TASK_STACKSIZE       2048
OS_STK    menu_stk[TASK_STACKSIZE];
OS_STK    speler1_stk[TASK_STACKSIZE];
OS_STK    speler2_stk[TASK_STACKSIZE];
OS_STK    speler3_stk[TASK_STACKSIZE];
OS_STK    speler4_stk[TASK_STACKSIZE];
OS_STK    invoer_stk[TASK_STACKSIZE];


ALT_SEM(pos)
ALT_SEM(af)
ALT_SEM(states)
ALT_SEM(spelerScore)


//global values
int hoogte[4]; // for every player pos Y
int lengte[4]; // for every player pos X
int spelerAf[4]; // for every player a seperate value if the player is game over
int score[4]; // for every player a seperate score
int statePlus[4];// for every player a seperate value for the state + or state -
int stateMin[4]; // for every player a seperate value for the state + or state -

int coords [115][115]; // the coordinates of the playeble area

void speler(void* pdata)
{

	//defining the values used in speler
	int l;
	int h;
	int state;
	int i;
	int spelerID = (int)(pdata); // spelerID is definded by pdata
	unsigned int kleur;
	char punten[40];
	int Af=0;
	spelerAf[spelerID]=0;


	// for every player there is a different pdata
	if(spelerID==0){
		l= 95;
		h = 15;
		state = 1;
		lengte[spelerID] = 5;
		hoogte[spelerID] = 5;
		kleur = 0x0F00;
		VGA_text(3, ((spelerID+1) * 6), "Player 1 \0");
		VGA_box(10,22,45,37,0x0F00);
	}else if(spelerID==1){
		l= 295;
		h = 15;
		state = 1;
		lengte[spelerID] = 105;
		hoogte[spelerID] = 5;
		kleur = 0xF000;
		VGA_text(3, ((spelerID+1) * 6), "Player 2 \0");
		VGA_box(10,46,45,61,0xF000);
	}else if(spelerID==2){
		l= 95;
		h = 215;
		state = 3;
		lengte[spelerID] = 5;
		hoogte[spelerID] = 105;
		kleur = 0x00FF;
		VGA_text(3, ((spelerID+1) * 6), "Player 3 \0");
		VGA_box(10,70,45,85,0x00FF);
	}else if(spelerID==3){
		l= 295;
		h = 215;
		state = 3;
		lengte[spelerID] = 105;
		hoogte[spelerID] = 105;
		kleur = 0xEE00;
		VGA_text(3, ((spelerID+1) * 6), "Player 4 \0");
		VGA_box(10,94,45,109,0xEE00);
	}

	while (1)
	{
		ALT_SEM_PEND(pos, 0);
		//hit detection for the player
		if(hoogte[spelerID] >=115 || hoogte[spelerID] <=0)
		{
			Af = 1;
		} else if (lengte[spelerID] >=115 || lengte[spelerID] <=0)
		{
			Af = 1;
		} else if (coords[hoogte[spelerID]][lengte[spelerID]]==1)
		{
			Af = 1;
		} else
		{
			coords[hoogte[spelerID]][lengte[spelerID]]=1;
		}
		ALT_SEM_POST(pos);
		ALT_SEM_PEND(spelerScore,0);

		// when the player dies his leds go off and his place and score get set
		if(Af == 1){
			ALT_SEM_PEND(af, 0);
			if(spelerAf[0] == 0){
				spelerAf[0] = spelerID+1;
				VGA_text(3, (((spelerID+1) * 6)+2), "Last \0");
				if(spelerID==0){

					alt_up_parallel_port_write_data(gpio_dev, 0x00019000);//alle leds behalve groen

				}else if(spelerID==1){
					alt_up_parallel_port_write_data(gpio_dev, 0x0001A000);//alle leds behalve rood
				}else if(spelerID==2){
					alt_up_parallel_port_write_data(gpio_dev, 0x0000F000);//alle leds behalve blauw
				}else{
					alt_up_parallel_port_write_data(gpio_dev, 0x00013000);//alle leds behalve geel
				}
			}else if(spelerAf[1] == 0){
				spelerAf[1] = spelerID+1;
				score[spelerID] = score[spelerID] +1;
				VGA_text(3, (((spelerID+1) * 6)+2), "Third \0");
					if(spelerAf[0]==1){
						if(spelerID==1){
							alt_up_parallel_port_write_data(gpio_dev, 0x00018000);//alle leds behalve groen en rood
						}
						else if(spelerID==2){
							alt_up_parallel_port_write_data(gpio_dev, 0x00009000);//alle leds behalve groen en blauw
						}
						else if(spelerID==3){
							alt_up_parallel_port_write_data(gpio_dev, 0x00011000);//alle leds behalve groen en geel
						}
					}else if(spelerAf[0]==2){

						if(spelerID==0){
							alt_up_parallel_port_write_data(gpio_dev, 0x00018000);//alle leds behalve groen en rood
						}
						else if(spelerID==2){
							alt_up_parallel_port_write_data(gpio_dev, 0x0000A000);//alle leds behalve rood en blauw
						}
						else if(spelerID==3){
							alt_up_parallel_port_write_data(gpio_dev, 0x00012000);//alle leds behalve rood en geel
						}
					}else if(spelerAf[0]==3){

						if(spelerID==0){
							alt_up_parallel_port_write_data(gpio_dev, 0x00009000);//alle leds behalve groen en blauw
						}
						if(spelerID==1){
							alt_up_parallel_port_write_data(gpio_dev, 0x0000A000);//alle leds behalve rood en blauw
						}
						if(spelerID==3){
							alt_up_parallel_port_write_data(gpio_dev, 0x00003000);//alle leds behalve blauw en geel
						}
					}else if(spelerAf[0]==4){

						if(spelerID==0){
							alt_up_parallel_port_write_data(gpio_dev, 0x00011000);//alle leds behalve groen en geel
						}
						if(spelerID==1){
							alt_up_parallel_port_write_data(gpio_dev, 0x00012000);//alle leds behalve rood en geel
						}
						if(spelerID==2){
							alt_up_parallel_port_write_data(gpio_dev, 0x00003000);//alle leds behalve blauw en geel
						}
					}
			}else if(spelerAf[2] == 0){
				spelerAf[2] = spelerID+1;
				score[spelerID] = score[spelerID] +2;
				VGA_text(3, (((spelerID+1) * 6)+2), "Second \0");

				if((spelerAf[0]==1&&spelerAf[1]==2)||(spelerAf[0]==2&&spelerAf[1]==1)){
					if(spelerID==2){

						alt_up_parallel_port_write_data(gpio_dev, 0x00008000);//blauw af
					}
					else if(spelerID==3){

						alt_up_parallel_port_write_data(gpio_dev, 0x00010000);//geel af
				}
				}
				else if((spelerAf[0]==2&&spelerAf[1]==3)||(spelerAf[0]==3&&spelerAf[1]==2)){
					if(spelerID==0){

						alt_up_parallel_port_write_data(gpio_dev, 0x00008000);//groen af
					}
					else if(spelerID==3){

						alt_up_parallel_port_write_data(gpio_dev, 0x00002000);//geel af
					}

				}
				else if((spelerAf[0]==1&&spelerAf[1]==3)||(spelerAf[0]==3&&spelerAf[1]==1)){
					if(spelerID==1){

						alt_up_parallel_port_write_data(gpio_dev, 0x00008000);//rood af
					}
					else if(spelerID==3){

						alt_up_parallel_port_write_data(gpio_dev, 0x00001000);//geel af
					}

				}
				else if((spelerAf[0]==2&&spelerAf[1]==4)||(spelerAf[0]==4&&spelerAf[1]==2)){
					if(spelerID==0){

						alt_up_parallel_port_write_data(gpio_dev, 0x00010000);//groen af
					}
					else if(spelerID==2){

						alt_up_parallel_port_write_data(gpio_dev, 0x00002000);//blauw af
					}

				}
				else if((spelerAf[0]==3&&spelerAf[1]==4)||(spelerAf[0]==4&&spelerAf[1]==3)){
					if(spelerID==0){

						alt_up_parallel_port_write_data(gpio_dev, 0x00001000);//groen af
					}
					else if(spelerID==1){

						alt_up_parallel_port_write_data(gpio_dev, 0x00002000);//rood af
					}

				}
				else if((spelerAf[0]==1&&spelerAf[1]==4)||(spelerAf[0]==4&&spelerAf[1]==1)){
					if(spelerID==1){

						alt_up_parallel_port_write_data(gpio_dev, 0x00010000);//rood af
					}
					else if(spelerID==2){

						alt_up_parallel_port_write_data(gpio_dev, 0x00001000);//blauw af
					}

				}
			}else{
				spelerAf[3] = spelerID+1;
				score[spelerID] = score[spelerID] +3;
				VGA_text(3, (((spelerID+1) * 6)+2), "First \0");
				alt_up_parallel_port_write_data(gpio_dev, 0x00000000);//alle leds
			}

			sprintf(punten,"%d", score[spelerID]);

			VGA_text(18, ((spelerID+1) * 6), punten);

			ALT_SEM_POST(spelerScore);
			ALT_SEM_POST(af);

			OSTaskDel(OS_PRIO_SELF);
		}
		ALT_SEM_POST(spelerScore);


		ALT_SEM_PEND(states, 0);
	  	if (stateMin[spelerID])
	  	{
			stateMin[spelerID] = 0;
			state--;
	  	}
	  	else if (statePlus[spelerID])
	  	{
			statePlus[spelerID] = 0;
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
void menu(void* pdata){
	int state=1;
	int selected=0;
	int Switch=1;
	int i;
	int credits = 1;

while(1){
	ALT_SEM_PEND(states, 0);
	if(statePlus[0]==1){
		statePlus[0]=0;
		state++;
		Switch=1;
	}
	if(stateMin[0]==1){
		stateMin[0]=0;
		selected=1;
	}
	ALT_SEM_POST(states);

	if(state==1){//play game
	if(selected==1){
		selected=0;
		VGA_box(160,56,240,66,0xC0FC);
		VGA_box(161,57,239,65,0xC0FC);
		OSTimeDlyHMSM(0, 0, 1, 0);
		VGA_box(160,56,240,66,0xC0FC);
		VGA_box(161,57,239,65,0x80CC);

		VGA_text(42,15 , "GAME STARTS IN 3               \0");
		alt_up_parallel_port_write_data(gpio_dev, 0x00000000);//alle leds
		OSTimeDlyHMSM(0, 0, 12, 0);
		VGA_text(42,15 , "GAME STARTS IN 2               \0");
		alt_up_parallel_port_write_data(gpio_dev, 0x0001F000);//alle leds
		OSTimeDlyHMSM(0, 0, 12, 0);
		VGA_text(42,15 , "GAME STARTS IN 1               \0");
		alt_up_parallel_port_write_data(gpio_dev, 0x00000000);//alle leds
		OSTimeDlyHMSM(0, 0, 12, 0);
		VGA_text(42,15 , "                               \0");
		alt_up_parallel_port_write_data(gpio_dev, 0x0001F000);//alle leds

		Game_Reset();
		VGA_text(45,20 , "                             \0");

		//clears score for a new game
		for(i=0;i<=4;i++){
			score[i]=0;
		}
		OSTimeDlyHMSM(0, 0, 12, 0);
		OSTaskCreateExt(speler,SPELER1,(void *)&speler1_stk[TASK_STACKSIZE-1],SPELER_PRIORITY,SPELER_PRIORITY,speler1_stk,TASK_STACKSIZE,NULL,0);
		OSTaskCreateExt(speler,SPELER2,(void *)&speler2_stk[TASK_STACKSIZE-1],SPELER_PRIORITY+1,SPELER_PRIORITY+1,speler2_stk,TASK_STACKSIZE,NULL,0);
		OSTaskCreateExt(speler,SPELER3,(void *)&speler3_stk[TASK_STACKSIZE-1],SPELER_PRIORITY+2,SPELER_PRIORITY+2,speler3_stk,TASK_STACKSIZE,NULL,0);
		OSTaskCreateExt(speler,SPELER4,(void *)&speler4_stk[TASK_STACKSIZE-1],SPELER_PRIORITY+3,SPELER_PRIORITY+3,speler4_stk,TASK_STACKSIZE,NULL,0);

		OSTaskDel(OS_PRIO_SELF);
	}
	if(Switch==1){
	Switch=0;

	VGA_box(160,56,240,66,0xC0FC);
	VGA_box(161,57,239,65,0x80CC);
	VGA_text(45,15 , " PLAY GAME               \0");

	VGA_box(160,76,240,86,0xC0FC);
	VGA_box(161,77,239,85,0x0000);
	VGA_text(45,20 , "  CREDITS                    \0");
	}
	}
	else if(state==2){//credits
	if (selected==1){
		selected=0;
		VGA_box(160,76,240,86,0xC0FC);
		VGA_box(161,77,239,85,0xC0FC);
		OSTimeDlyHMSM(0, 0, 1, 0);
		VGA_box(160,76,240,86,0xC0FC);
		VGA_box(161,77,239,85,0x80CC);

		Game_Reset();
		VGA_text(42,15 , "                               \0");
		VGA_text(45,20 , "                             \0");

		while(credits == 1){
			VGA_box(90,55,310,76, 0x0F00);
			OSTimeDlyHMSM(0, 0, 1, 0);
			alt_up_parallel_port_write_data(gpio_dev, 0x00000000);//geen leds
			VGA_box(91,56,309,75, 0x0000);
			VGA_text(24,15 , " Made by the Great and Powerful Michiel van Dalfsen           \0");
			VGA_text(24,17 , "    and by the Beautiful and Wise Daniek Borst :)              \0");

			ALT_SEM_PEND(states, 0);
			if(stateMin[0]==1)
			{
				stateMin[0]=0;
				credits = 0;
			}
			ALT_SEM_POST(states);

			OSTimeDlyHMSM(0, 0, 1, 0);
			alt_up_parallel_port_write_data(gpio_dev, 0x0001F000);//alle leds
		}
		credits = 1;

		VGA_text(24,15 , "                                                                    \0");
		VGA_text(24,17 , "                                                                    \0");

		VGA_box(90,55,310,76, 0x0000);
// button for play game
		VGA_box(160,56,240,66,0xC0FC);
		VGA_box(161,57,239,65,0x0000);
		VGA_text(45,15 , " PLAY GAME               \0");
// button for credits
		VGA_box(160,76,240,86,0xC0FC);
		VGA_box(161,77,239,85,0x80CC);
		VGA_text(45,20 , "  CREDITS                    \0");
	}

	if(Switch==1){
		Switch=0;
		VGA_box(160,56,240,66,0xC0FC);
		VGA_box(161,57,239,65,0x0000);
		VGA_text(45,15 , " PLAY GAME               \0");

		VGA_box(160,76,240,86,0xC0FC);
		VGA_box(161,77,239,85,0x80CC);
		VGA_text(45,20 , "  CREDITS                    \0");
	}

	}
	else if(state>2){
		state=1;
	}
	OSTimeDlyHMSM(0, 0, 1, 0);
}
}

void invoer(void* pdata)
{

	volatile int * KEY_ptr = (int *)0x10000050;
	int KEY_value;
	int gpio_values;
	int veranderd[8];

	while (1) {
		KEY_value = *(KEY_ptr + 3);			// read the pushbutton interrupt register
		*(KEY_ptr + 3) = 0; 						// Clear the interrupt


		if (KEY_value == 0x2)					// check KEY1 to reset game so you can play again, players keep their score
		{
			if(spelerAf[0]!=0&&spelerAf[1]!=0&&spelerAf[2]!=0&&spelerAf[3]!=0){
				Game_Reset();

						VGA_text(42,15 , "GAME STARTS IN 3               \0");
						alt_up_parallel_port_write_data(gpio_dev, 0x00000000);//alle leds
						OSTimeDlyHMSM(0, 0, 12, 0);
						VGA_text(42,15 , "GAME STARTS IN 2               \0");
						alt_up_parallel_port_write_data(gpio_dev, 0x0001F000);//alle leds
						OSTimeDlyHMSM(0, 0, 12, 0);
						VGA_text(42,15 , "GAME STARTS IN 1               \0");
						alt_up_parallel_port_write_data(gpio_dev, 0x00000000);//alle leds
						OSTimeDlyHMSM(0, 0, 12, 0);
						VGA_text(42,15 , "                               \0");
						alt_up_parallel_port_write_data(gpio_dev, 0x0001F000);//alle leds

				OSTaskCreateExt(speler,SPELER1,(void *)&speler1_stk[TASK_STACKSIZE-1],SPELER_PRIORITY,SPELER_PRIORITY,speler1_stk,TASK_STACKSIZE,NULL,0);
				OSTaskCreateExt(speler,SPELER2,(void *)&speler2_stk[TASK_STACKSIZE-1],SPELER_PRIORITY+1,SPELER_PRIORITY+1,speler2_stk,TASK_STACKSIZE,NULL,0);
				OSTaskCreateExt(speler,SPELER3,(void *)&speler3_stk[TASK_STACKSIZE-1],SPELER_PRIORITY+2,SPELER_PRIORITY+2,speler3_stk,TASK_STACKSIZE,NULL,0);
				OSTaskCreateExt(speler,SPELER4,(void *)&speler4_stk[TASK_STACKSIZE-1],SPELER_PRIORITY+3,SPELER_PRIORITY+3,speler4_stk,TASK_STACKSIZE,NULL,0);
			}

		}
		if (KEY_value == 0x4)					// check KEY2 to go back to Menu
		{
			if(spelerAf[0]!=0&&spelerAf[1]!=0&&spelerAf[2]!=0&&spelerAf[3]!=0){
				Game_Reset();
				//clears screen of text
					 VGA_text(3,3,   "                            \0");
					 VGA_text(3, 6,  "                            \0");
					 VGA_text(3, 8,  "                            \0");
					 VGA_text(3, 12, "                            \0");
					 VGA_text(3, 14, "                            \0");
					 VGA_text(3, 18, "                            \0");
					 VGA_text(3, 20, "                            \0");
					 VGA_text(3, 24, "                            \0");
					 VGA_text(3, 26, "                            \0");
				//clears player area
				VGA_box(5,5,79,149,0);

				OSTaskCreateExt(menu,NULL,(void *)&menu_stk[TASK_STACKSIZE-1],MENU_PRIORITY,MENU_PRIORITY,menu_stk,TASK_STACKSIZE,NULL,0);
			}
		}


		ALT_SEM_PEND(states, 0);
							//state naar rechts

		gpio_values = alt_up_parallel_port_read_data(gpio_dev);

		gpio_values &= 0x00000001;		//	negates all the other ports exept port 1

		if (gpio_values == 0){
			if (veranderd[0] == 0){
				statePlus[0]= 1;
				veranderd[0] = 1;

				}
			}
			else{
				veranderd[0] = 0;
			}

		gpio_values = alt_up_parallel_port_read_data(gpio_dev);

		gpio_values &= 0x00000002;		//	negates all the other ports exept port 2

		if (gpio_values == 0){
			if (veranderd[1] == 0){
			stateMin[0]= 1;
			veranderd[1] = 1;

			}
		}
		else
			veranderd[1] = 0;

		gpio_values = alt_up_parallel_port_read_data(gpio_dev);

		gpio_values &= 0x00000004;		//		negates all the other ports exept port 3

		if (gpio_values == 0){
			if (veranderd[2] == 0){
				statePlus[1]= 1;
				veranderd[2] = 1;
			}
		}
		else
			veranderd[2] = 0;

		gpio_values = alt_up_parallel_port_read_data(gpio_dev);

		gpio_values &= 0x00000008;		//	negates all the other ports exept port 4

		if (gpio_values == 0){
			if (veranderd[3] == 0){
				stateMin[1]=1;
				veranderd[3] = 1;
			}
		}
		else
			veranderd[3] = 0;

		gpio_values = alt_up_parallel_port_read_data(gpio_dev);

			gpio_values &= 0x00000010;		//	negates all the other ports exept port 5

			if (gpio_values == 0){
				if (veranderd[4] == 0){
					statePlus[2]=1;
					veranderd[4] = 1;
				}
			}
			else
				veranderd[4] = 0;

		gpio_values = alt_up_parallel_port_read_data(gpio_dev);

		gpio_values &= 0x00000020;		//		negates all the other ports exept port 6

		if (gpio_values == 0){
			if (veranderd[5] == 0){
				stateMin[2]=1;
				veranderd[5] = 1;
			}
		}
		else
			veranderd[5] = 0;

		gpio_values = alt_up_parallel_port_read_data(gpio_dev);

		gpio_values &= 0x00000080;		//		negates all the other ports exept port 8, we got no results from port 7 so we skipped it

		if (gpio_values == 0){
			if (veranderd[6] == 0){
				statePlus[3]=1;
				veranderd[6] = 1;
			}
		}
		else
			veranderd[6] = 0;

		gpio_values = alt_up_parallel_port_read_data(gpio_dev);

		gpio_values &= 0x00000100;		//		negates all the other ports exept port 9

		if (gpio_values == 0){
			if (veranderd[7] == 0){
				stateMin[3]=1;
				veranderd[7] = 1;
			}
		}
		else
			veranderd[7] = 0;

		ALT_SEM_POST(states);

		OSTimeDlyHMSM(0, 0, 1, 0);
	}
}

int main(void)
{
 	gpio_dev = alt_up_parallel_port_open_dev("/dev/Expansion_JP5");		//	DE2-115 gpio
	alt_up_parallel_port_set_port_direction(gpio_dev, 0x0001F000);		// zet de poorten waaraan de leds gekoppelt zijn op output

	alt_up_parallel_port_write_data(gpio_dev, 0x0001F000);//alle leds aan

	LCD_cursor_off();

	ALT_SEM_CREATE(&pos, 1);
	ALT_SEM_CREATE(&af, 1);
	ALT_SEM_CREATE(&states, 1);
	ALT_SEM_CREATE(&spelerScore, 1);

	Game_Reset();

	OSTaskCreateExt(menu,NULL,(void *)&menu_stk[TASK_STACKSIZE-1],MENU_PRIORITY,MENU_PRIORITY,menu_stk,TASK_STACKSIZE,NULL,0);

	OSTaskCreateExt(invoer, NULL, (void *)&invoer_stk[TASK_STACKSIZE - 1], INVOER_PRIORITY, INVOER_PRIORITY, invoer_stk, TASK_STACKSIZE, NULL, 0);

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

/****************************************************************************************
 * Reset the playfield
 ****************************************************************************************/
void Game_Reset(void){
	int coord1;
	int coord2;
	int i;
	// shows game is resetting
	VGA_text(5, 3, "RESETTING \0");


	//clears array
	for(coord1=0;coord1<115;coord1++)
	{
		for(coord2=0;coord2<115;coord2++)
		{
			coords[coord1][coord2] = 0;
		}
	}
	for(i=0;i<3;i++){
		spelerAf[i]=0;
		statePlus[i]=0;
		stateMin[i]=0;
	}

	//clears screen of text
	 VGA_text(3,3,   "                            \0");
	 //VGA_text(3, 6,  "                            \0");
	 VGA_text(3, 8,  "                            \0");
	 //VGA_text(3, 12, "                            \0");
	 VGA_text(3, 14, "                            \0");
	// VGA_text(3, 18, "                            \0");
	 VGA_text(3, 20, "                            \0");
	// VGA_text(3, 24, "                            \0");
	 VGA_text(3, 26, "                            \0");

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

	//TRON NEON

	//LETTER T
	VGA_box(15,151,15,170,0x00FF);
	VGA_box(9,150,21,150,0x00FF);
	VGA_box(9,151,9,153,0x00FF);
	VGA_box(21,151,21,153,0x00FF);

	//LETTER R
	VGA_box(24,150,24,170,0xF000);
	VGA_box(24,150,34,150,0xF000);
	VGA_box(34,151,35,151,0xF000);
	VGA_box(35,152,36,152,0xF000);
	VGA_box(36,153,36,158,0xF000);
	VGA_box(34,160,35,160,0xF000);
	VGA_box(35,159,36,159,0xF000);
	VGA_box(24,161,34,161,0xF000);
	VGA_box(28,162,28,162,0xF000);
	VGA_box(29,162,29,163,0xF000);
	VGA_box(30,163,30,164,0xF000);
	VGA_box(31,164,31,165,0xF000);
	VGA_box(32,165,32,166,0xF000);
	VGA_box(33,166,33,167,0xF000);
	VGA_box(34,167,34,168,0xF000);
	VGA_box(35,168,35,169,0xF000);
	VGA_box(36,169,36,170,0xF000);

	//LETTER O
	VGA_box(39,151,39,169,0x0F00);
	VGA_box(40,150,50,150,0x0F00);
	VGA_box(40,170,50,170,0x0F00);
	VGA_box(51,151,51,169,0x0F00);

	//LETTER N
	VGA_box(54,150,54,170,0xFF00);
	VGA_box(66,150,66,170,0xFF00);
	VGA_box(55,151,55,153,0xFF00);
	VGA_box(56,153,56,154,0xFF00);
	VGA_box(57,154,57,156,0xFF00);
	VGA_box(58,156,58,157,0xFF00);
	VGA_box(59,157,59,159,0xFF00);
	VGA_box(60,159,60,160,0xFF00);
	VGA_box(61,160,61,162,0xFF00);
	VGA_box(62,162,62,163,0xFF00);
	VGA_box(63,163,63,165,0xFF00);
	VGA_box(64,165,64,166,0xFF00);
	VGA_box(65,166,65,168,0xFF00);

	// DUBBLE PUNT
	VGA_box(70,155,71,156,0x47FF);
	VGA_box(70,166,71,167,0x47FF);

	//NEON

	//LETTER N
	VGA_box(9,175,9,195,0xFF00);
	VGA_box(21,175,21,195,0xFF00);
	VGA_box(10,176,10,178,0xFF00);
	VGA_box(11,178,11,179,0xFF00);
	VGA_box(12,179,12,181,0xFF00);
	VGA_box(13,181,13,182,0xFF00);
	VGA_box(14,182,14,184,0xFF00);
	VGA_box(15,184,15,185,0xFF00);
	VGA_box(16,185,16,187,0xFF00);
	VGA_box(17,187,17,188,0xFF00);
	VGA_box(18,188,18,190,0xFF00);
	VGA_box(19,190,19,191,0xFF00);
	VGA_box(20,191,20,193,0xFF00);

	//LETTER E
	VGA_box(24,175,24,195,0x0F00);
	VGA_box(24,175,36,175,0x0F00);
	VGA_box(24,185,34,185,0x0F00);
	VGA_box(24,195,36,195,0x0F00);

	//LETTER O
	VGA_box(39,176,39,194,0x00FF);
	VGA_box(40,175,50,175,0x00FF);
	VGA_box(40,195,50,195,0x00FF);
	VGA_box(51,176,51,194,0x00FF);

	//LETTER N
	VGA_box(54,175,54,195,0xF000);
	VGA_box(66,175,66,195,0xF000);
	VGA_box(55,176,55,178,0xF000);
	VGA_box(56,178,56,179,0xF000);
	VGA_box(57,179,57,181,0xF000);
	VGA_box(58,181,58,182,0xF000);
	VGA_box(59,182,59,184,0xF000);
	VGA_box(60,184,60,185,0xF000);
	VGA_box(61,185,61,187,0xF000);
	VGA_box(62,187,62,188,0xF000);
	VGA_box(63,188,63,190,0xF000);
	VGA_box(64,190,64,191,0xF000);
	VGA_box(65,191,65,193,0xF000);

	//clear game over

	VGA_text(3,3,"                            \0");
}
