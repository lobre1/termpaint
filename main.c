#include <ncurses.h>
#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>

#define COLORNUMBER 6

struct cursor{
	int y;//5
	int x;//5
	int isDraw;//0
	int colorCounter;//1
	int isPaint;//1
	int isErase;//0
};

void cursor_set_color_string(const char *color) {
    printf("\e]12;%s\a\e]12;%s\a", color, color);
    fflush(stdout);
}

void keyHandler(int c, int dimen[], struct cursor *cur){
	int oldPos[2]={cur->y,cur->x};
	switch (c) {
		case 'w':
			cur->y--;
			break;
		case 'a':
			cur->x--;
			break;
		case 's':
			cur->y++;
			break;
		case 'd':
			cur->x++;
			break;
		case ' ':
			cur->isDraw=1-cur->isDraw;
			cur->isErase=0;
			break;
		case 13:
			cur->colorCounter++;
			if(cur->colorCounter>=COLORNUMBER){ cur->colorCounter=1; }
			break;
		case 'x':
			cur->isErase = 1-cur->isErase;
			cur->isDraw=0;
			break;
		case 'q':
			cur->isPaint = 0;
			break;
	}
	if (cur->y>=dimen[0] || cur->y<=-1 || cur->x>=dimen[1] || cur->x<=-1) {
		cur->y=oldPos[0];
		cur->x=oldPos[1];
		return;
	}
}

void dumpScreen(FILE * fp, WINDOW * win, int dimen[2]){
	fprintf(fp, "P3\n %d %d 1\n", dimen[1], dimen[0]*2);

	fputc('\n',fp);

	for (int i = 0; i < dimen[0]*2; i++) {
		for (int j = 0; j < dimen[1]; j++) {
			char c = (char)mvwinch(win, floor((double)i/2), j);
			switch (c) {
				case ' ':
					fputs("1 1 1", fp);
					break;
				case '1':
					fputs("0 0 0", fp);
					break;
				case '2':
					fputs("1 0 0", fp);
					break;
				case '3':
					fputs("0 0 1", fp);
					break;
				case '4':
					fputs("0 1 0", fp);
					break;
				case '5':
					fputs("1 1 0", fp);
					break;
			}
			fputs("   ", fp);
		}
		fputc('\n',fp);
	}
}

void colorInit(){
	start_color();
	init_pair(1, COLOR_BLACK, COLOR_BLACK);
	init_pair(2, COLOR_RED, COLOR_RED);
	init_pair(3, COLOR_BLUE, COLOR_BLUE);
	init_pair(4, COLOR_GREEN, COLOR_GREEN);
	init_pair(5, COLOR_YELLOW, COLOR_YELLOW);
	init_pair(6, COLOR_WHITE, COLOR_WHITE);

	init_pair(7, COLOR_WHITE, COLOR_BLACK);
}

void genInit(int dimen[], struct cursor *cur){
	initscr();
	curs_set(0);
	colorInit();
	cbreak();
	keypad(stdscr, TRUE);
	noecho();
	cur->x=floor((double)dimen[1]/2);
	cur->y=floor((double)dimen[0]/2);
	cur->isDraw=0;
	cur->isPaint=1;
	cur->isErase=0;
	cur->colorCounter=1;
}

char * mode(struct cursor *cur){
	if (cur->isDraw == 0 && cur->isErase == 0) { return "Move "; }
	if (cur->isDraw == 1 && cur->isErase == 0) { return "Draw "; }
	if (cur->isDraw == 0 && cur->isErase == 1) { return "Erase"; }
	return "NULL";
}

void loop(WINDOW * win, WINDOW * headsUp, int dimen[], struct cursor *cur){
	while(cur->isPaint){
		int y,x;

		if ( cur->isErase == 1 ){
			wattron(headsUp, COLOR_PAIR(7));
			mvwaddch(win, cur->y, cur->x, ' ');
			mvwprintw(headsUp, 1, 1, "DEL");
		}else{
			wattron(headsUp, COLOR_PAIR(cur->colorCounter));
			mvwprintw(headsUp, 1, 1, "   ");
		}
		wattroff(win, COLOR_PAIR(cur->colorCounter));

		wattron(headsUp, COLOR_PAIR(7));
		mvwprintw(headsUp, 3, 1, "Mode:");
		mvwprintw(headsUp, 4, 1, "%s", mode(cur));
		wattroff(win, COLOR_PAIR(cur->colorCounter));

		wmove(win, cur->y, cur->x);

		wrefresh(headsUp);
		wrefresh(win);

		//cursor_set_color_string("black"); 

		if (cur->isDraw==1){
			wattron(win, COLOR_PAIR(cur->colorCounter));
			wprintw(win, "%d", cur->colorCounter);
		}

		wmove(win, cur->y, cur->x);

		keyHandler(getchar(), dimen, cur);
	}
	return;
}

int isNum( char num[] ){
	int length = strlen(num);
	for (int i = 0; i<length; i++) {
		if (!isdigit(num[i])) {
			return 1;
		}
	}
	return 0;
}

void argHandler( int argc, char **argv, int dimen[]){
	switch (argc) {
		case 4:
			for (int i = 1; i<3; i++) {
				if (isNum(argv[i])) {
					printf("Syntax: \n 2dmove int int filename\n");
					exit(1);
				}
			}
			dimen[0]=atoi(argv[1]);
			dimen[1]=atoi(argv[2]);
			break;
		default:
			printf("Syntax: \n 2dmove Height Width filename\n");
			exit(1);
			break;
	}
}

int main(int argc, char **argv){
	int dimen[2];

	struct cursor cur;

	argHandler(argc, argv, dimen);

	genInit(dimen, &cur);

	WINDOW *win = newwin(dimen[0],dimen[1],0,0);
	WINDOW *headsUp = newwin(dimen[0],7,0,dimen[1]);

	wattron(win, COLOR_PAIR(1));
	box(headsUp, '|', '-');
	wattroff(win, COLOR_PAIR(1));

	wbkgd(win, COLOR_PAIR(6));

	curs_set(0);

	wmove(win, cur.y, cur.x);

	loop(win, headsUp, dimen, &cur);

	FILE *fp = fopen(strcat(argv[3], ".ppm"),"w");

	if (fp == NULL) {
		endwin();
		printf("No file name specified");
		return 1;
	}

	cursor_set_color_string("white"); 
	dumpScreen(fp, win, dimen);

	endwin();
	return 0;
}
