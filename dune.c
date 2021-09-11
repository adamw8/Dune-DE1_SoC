/* This files provides address values that exist in the system */

#define SDRAM_BASE            0xC0000000
#define FPGA_ONCHIP_BASE      0xC8000000
#define FPGA_CHAR_BASE        0xC9000000

/* Cyclone V FPGA devices */
#define LEDR_BASE             0xFF200000
#define HEX3_HEX0_BASE        0xFF200020
#define HEX5_HEX4_BASE        0xFF200030
#define SW_BASE               0xFF200040
#define KEY_BASE              0xFF200050
#define TIMER_BASE            0xFF202000
#define PIXEL_BUF_CTRL_BASE   0xFF203020
#define CHAR_BUF_CTRL_BASE    0xFF203030
#define PS2_BASE              0xFF200100


/* VGA colors */
#define WHITE 0xFFFF
#define BLACK 0x0000
#define YELLOW 0xFFE0
#define RED 0xF800
#define GREEN 0x07E0
#define BLUE 0x001F
#define CYAN 0x07FF
#define MAGENTA 0xF81F
#define GREY 0xC618
#define PINK 0xFC18
#define ORANGE 0xFC00
#define BACKGROUND 0x0570
#define GRADIENT 15
#define DUNE_COLOR 0xFFB5
#define NUM_BALL_COLORS 10
	
#define ABS(x) (((x) > 0) ? (x) : -(x))


/* Screen size. */
#define RESOLUTION_X 320
#define RESOLUTION_Y 240

// Ball location parameters
#define BALL_X 20
#define BALL_Y 10
#define BALL_R 3
#define MAX_SPEED 25
#define MAX_ACCELERATION 3
#define MIN_ACCELERATION 1

//Dune parameters
#define NUM_DUNES 15 //more than 3 plz
#define MIDDLE_DUNE 180 //Highest point is 2/3 of Y, lowest point is 1/10 of Y, mid point is 23/60 of Y
#define MAX_AMPLITUDE_DUNE 50 // from analysis, must be bigger than 10
#define DUNE_FREQUENCY 3.1415926535897/80  // two dunes on the screen at all times
#define DUNE_PERIOD 160 //half of the screen
#define PI 3.1415926535897


// arrow parameters
#define ARROW_SCALE 0.3	// height per pixel out of bounds
#define MIN_ARROW_HEIGHT 20
#define MAX_ARROW_HEIGHT 60
	
// other PARAMETERS
#define SCORE_LINE_Y 60
#define X_2500 BALL_X + MAX_ARROW_HEIGHT/2
#define Y_2500 SCORE_LINE_Y-15
#define FRAMES_2500 12// must be greater than 2

#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <math.h>
#include <time.h>

// useful structs
typedef struct Ball {
	int x, y;
	double dx, dy;
	int color;
	int radius;
	// variables to help clear buffer
	int x1, y1; // position 1 frame ago
	int x2, y2; // position 2 frames ago
} Ball;

typedef struct Dune{
	int dunePoints[RESOLUTION_X];
	double duneAngles[RESOLUTION_X];
	int dunePoints1[RESOLUTION_X];
	int dunePoints2[RESOLUTION_X];
} Dune;

// note that this arrow will always be pointing upwards
typedef struct Arrow {
	int x, y; // location of arrow tip
	int h; //current height
	int h1; // 1 framge ago
	int h2; // 2 frames ago
	short int color;
} Arrow;


//functions we need to implement
/*
drawPixel();  // draws pixel at given location and color
drawBackground(); // if possible
drawDune(); // draws the dune on the screen
drawBall();
drawArrow();
drawStartingScreen();
drawScore(); //start the score on the screen
drawGameOverScreen();
gameOverCheck(); // check if the ball crashed or not
inBounds function();
updateGameInfo(); updates the position of the ball and the dunes before the next draw()
clearScreen();
wait_for_vsync();
*/

// helper functions
bool in_bounds(int x, int y);
bool in_y_bounds(int y);
bool in_x_bounds(int x);
void draw_line(int x0, int y0, int x1, int y1, short int line_color);
bool isBallTouchingDune(Ball* ball, Dune* dune); 
void display_score(int score);
int max(int a, int b);
int min(int a, int b);

// drawing/buffer function
void draw(Ball* ball, Dune* dune, Arrow* arrow, int score);
void draw_ball(Ball* ball);
void draw_dune(Dune* Dune);
void draw_dune_slice(Dune* dune, int x1, int x2);
void draw_arrow(Arrow* arrow);
void update_arrow(Arrow* arrow, int y);
void draw_starting_screen();
void draw_game_over_screen(Ball* ball, Dune* dune, int points, int frames);
void plot_pixel(int x, int y, short int line_color);
void black_screen();
void draw_background();
void draw_score(int points);
void draw_2500(int x, int y, short int color);

// clearing functions
void clear_pixel(int x, int y);
void clear_line(int x0, int y0, int x1, int y1);
void clear_rectangle(int x0, int y0, int x1, int y1);
void clear_screen(Ball* ball, Dune* dune, Arrow* arrow);
void clear_ball(Ball* ball);
void clear_arrow(Arrow* arrow);
void clear_running_dune(Dune* dune);
void clear_all_dune(Dune* dune);

void wait_for_vsync();

// mouse functions
void space_key_clicked(bool* flag);
void receive_bytes(int n);
// change color
int read_SW();
short int set_ball_color();

volatile int pixel_buffer_start; // global variable
unsigned char seven_seg_decode_table[] = {0x3F, 0x06, 0x5B, 0x4F, 0x66, 0x6D, 0x7D, 0x07, 0x7F, 0x67};
short int ball_colors[] = {WHITE, RED, ORANGE, YELLOW, GREEN, CYAN, BLUE, MAGENTA, PINK, BLACK};
// variables to clear +2500
int toClear = 0;
int toDraw = 0;

int main(void){
	//initialize location and game physics
	Ball ball = {.x = BALL_X, .y = BALL_Y, .dx = 0, .dy = 0, .color = RED, .radius = BALL_R, 
				 .x1 = BALL_X, .y1 = BALL_Y, .x2 = BALL_X, .y2 = BALL_Y};
	Arrow arrow = {.x = BALL_X, .y = 5, .h = MIN_ARROW_HEIGHT, .h1 = MIN_ARROW_HEIGHT, .h2 = MIN_ARROW_HEIGHT, .color = WHITE};
    double acceleration = 1;
	
	//parameters for the dunes
	Dune dune;
	// Dune lastDune; // commented to suppress error
	double duneHeight[NUM_DUNES];
	for(int i = 0; i < NUM_DUNES; i++){
		// seeding rand for actual randomness
		//srand(time(NULL));
		duneHeight[i] = rand()%(MAX_AMPLITUDE_DUNE-10)+10; //between 10 and MAX_AMPLITUDE_DUNE
	}
	int allDunePoints[NUM_DUNES*DUNE_PERIOD];
	double allDuneAngles[NUM_DUNES*DUNE_PERIOD];
    int amplitude;
	for(int x = 0; x < NUM_DUNES*DUNE_PERIOD; x++){
		amplitude = (int) x/DUNE_PERIOD;
		allDunePoints[x] = (int) duneHeight[amplitude]*sin(DUNE_FREQUENCY*(x%DUNE_PERIOD)) + MIDDLE_DUNE;
		allDuneAngles[x] = atan(duneHeight[amplitude]*DUNE_FREQUENCY*cos(DUNE_FREQUENCY*(x%DUNE_PERIOD)));
	}
	double currentX = 0;

	for(int x = 0; x < RESOLUTION_X; x++){
		dune.dunePoints[x] = allDunePoints[x];
		dune.duneAngles[x] = allDuneAngles[x];
		dune.dunePoints1[x] = 239;
		dune.dunePoints2[x] = 239;
	}
	
	bool spacebar = false; // variable to store if spacebar is pressed	
	
	//game screen game
	int startScreen = 1;
	int gameOver = 0;
	int gameOverFramesDrawn = 0;

	// set up ps2 port
	volatile int * PS2_ptr = (int *)PS2_BASE;
	*(PS2_ptr) = 0xFF; // reset keyboard
	receive_bytes(2); // receive acknowledge bits
	
	// variable for tracking time
	//clock_t start;
		
	// set up buffers
    volatile int * pixel_ctrl_ptr = (int *)0xFF203020;
	
	/* set front pixel buffer to start of FPGA On-chip memory */
    *(pixel_ctrl_ptr + 1) = 0xC8000000; // first store the address in the back buffer
    /* now, swap the front/back buffers, to set the front buffer location */
    wait_for_vsync();
    /* initialize a pointer to the pixel buffer, used by drawing functions */
    pixel_buffer_start = *pixel_ctrl_ptr;
	// clear the buffer at 0xc8000000 (back buffer)
	draw_background();
	
    /* set back pixel buffer to start of SDRAM memory */
    *(pixel_ctrl_ptr + 1) = 0xC0000000;
	pixel_buffer_start = *(pixel_ctrl_ptr + 1); // we draw on the back buffer
	// clear the back buffer
	draw_background();
	wait_for_vsync();
	
	// clear the score	
	int score = 0;
	while(1){		
		/* Erase any game things that were drawn in the last iteration */
		// set the buffer ID correctly and clear the screen
		// clock_t start = clock();
		if(!startScreen && !gameOver){
			clear_screen(&ball, &dune, &arrow);
			clear_running_dune(&dune);
		}
		
		// printf("Clear screen: %.3fs\n", 1.0*(clock() - start) / CLOCKS_PER_SEC);
		
		// code for drawing the current game iteration
		if(startScreen) draw_starting_screen();
		// check for game over
		else if(gameOver) {
			ball.color = set_ball_color();
			draw_game_over_screen(&ball, &dune, score, gameOverFramesDrawn);
			gameOverFramesDrawn++;
		}
		else{
			// check for ball color
			ball.color = set_ball_color();
			// check for +2500 score bonus
			if (toDraw == FRAMES_2500)
				score += 2500;
			score = min(999999, score + ball.dx);
			draw(&ball, &dune, &arrow, score);
		}
		
		// printf("Draw: %.3fs\n", 1.0*(clock() - start) / CLOCKS_PER_SEC);
		
		display_score(score);
		
		//printf("Score %.3fs\n", 1.0*(clock() - start) / CLOCKS_PER_SEC);
		space_key_clicked(&spacebar);
		//printf("Spacebar: %.3fs\n", 1.0*(clock() - start) / CLOCKS_PER_SEC);
		//update drawing
		int prevY = ball.y;
		double isGameOver = 0;
		if(!startScreen && !gameOver){
			if(spacebar) acceleration = MAX_ACCELERATION;
			else acceleration = MIN_ACCELERATION;

			
			//update dune
			currentX+= ball.dx;
			if(currentX >= NUM_DUNES*DUNE_PERIOD) currentX -= NUM_DUNES*DUNE_PERIOD;
        	for(int x = 0; x < RESOLUTION_X; x++){
				int xINC = ((int)(x + currentX))%(NUM_DUNES*DUNE_PERIOD);
				dune.dunePoints2[x] = dune.dunePoints1[x];
				dune.dunePoints1[x] = dune.dunePoints[x];
				dune.dunePoints[x] = allDunePoints[xINC];
				dune.duneAngles[x] = allDuneAngles[xINC];
			}
			
			//update ball position
			ball.y += ball.dy + 0.5*acceleration;
			
			//update ball speeds
			if(isBallTouchingDune(&ball,&dune)){
				
				for(int i = 0; i < ball.radius; i++){
					if(ball.y+ball.radius > dune.dunePoints[ball.x+i]) ball.y = dune.dunePoints[ball.x+i]-ball.radius;
					if(ball.y+ball.radius > dune.dunePoints[ball.x-i]) ball.y = dune.dunePoints[ball.x-i]-ball.radius;
				}
				double duneAngle = dune.duneAngles[ball.x];
				double ballAngle = atan(ball.dy/ball.dx);
				double speed = sqrt(ball.dx*ball.dx + ball.dy*ball.dy);
				
				if(ball.dx < 0 && duneAngle < 0 && acceleration == MIN_ACCELERATION) ;
				else if(duneAngle > 0 && ballAngle >= 0){
					double dx1 = speed*cos(ballAngle - duneAngle) + acceleration;

					ball.dx = dx1*cos(duneAngle);
					ball.dy = dx1*sin(duneAngle);
				}
				else if(duneAngle < 0 && ballAngle < 0){
					duneAngle = - duneAngle;
					ballAngle = - ballAngle;
					
					double dx1 = speed*cos(duneAngle - ballAngle) + acceleration;
					
					ball.dx = dx1*cos(duneAngle);
					ball.dy = -dx1*sin(duneAngle);
				}
				else if(duneAngle < 0 && ballAngle >= 0){		
					duneAngle = - duneAngle;
					double dx1 = speed*cos(duneAngle + ballAngle) + acceleration;
					if(ballAngle >= PI/2 - duneAngle && speed > MAX_SPEED) isGameOver = 1;
					
					ball.dx = dx1*cos(duneAngle);
					ball.dy = -dx1*sin(duneAngle);
				}
				else if(duneAngle == 0){
					ball.dy = (int) -(ball.dy + acceleration);
					ball.dy += acceleration;
					if(ball.dx < 1) ball.dx = acceleration;
				}
				else{
       				ball.dy += acceleration;
				}
				
				
				//update ball speed to maximum
				speed = min(MAX_SPEED, sqrt(ball.dx*ball.dx + ball.dy*ball.dy));
				ballAngle = atan(ball.dy/ball.dx);
				
				ball.dy = speed*sin(ballAngle);
				ball.dx = speed*cos(ballAngle);
				
				if(((int) ball.dx == 0) && duneAngle < 0) ball.dx = -1;
			}
    		else{
       			ball.dy += 0.5*acceleration;
    		}
		}

		// draw +2500
		if (prevY > SCORE_LINE_Y && ball.y < SCORE_LINE_Y)
			toDraw =  FRAMES_2500;
		
		//start the game after pressing space of the start screen
		if(startScreen){
			if(spacebar){
				startScreen = 0;
				draw_background();				
				wait_for_vsync();
				pixel_buffer_start = *(pixel_ctrl_ptr + 1);
				draw_background();
			}
		}
		else if(gameOver){ //reset values
			if(spacebar && gameOverFramesDrawn >= 3){
				gameOver = 0;
				gameOverFramesDrawn = 0;
				toDraw = 0; // set variables to clear +2500
				toClear = 2; 
				currentX = 0;
				draw_background();
				for(int x = 0; x < RESOLUTION_X; x++){
					dune.dunePoints[x] = allDunePoints[x];
					dune.duneAngles[x] = allDuneAngles[x];
					dune.dunePoints1[x] = 239;
					dune.dunePoints2[x] = 239;
				}
				wait_for_vsync();
				pixel_buffer_start = *(pixel_ctrl_ptr + 1);
				draw_background();
				Ball newBall = {.x = BALL_X, .y = BALL_Y, .dx = 0, .dy = 0, .color = ball.color, .radius = BALL_R, 
				 .x1 = BALL_X, .y1 = BALL_Y, .x2 = BALL_X, .y2 = BALL_Y};
				ball = newBall;
				for(int x = 0; x < RESOLUTION_X; x++){
					dune.dunePoints[x] = allDunePoints[x];
					dune.duneAngles[x] = allDuneAngles[x];
					dune.dunePoints1[x] = 239;
					dune.dunePoints2[x] = 239;
				}
				score = 0;
			}
		}
		// printf("Update drawing: %.3fs\n", 1.0*(clock() - start) / CLOCKS_PER_SEC);
		
		if (score >= 999999)
			gameOver = 1;
		if(isGameOver) gameOver = 1;
		
		wait_for_vsync(); // swap front and back buffers on VGA vertical sync
		pixel_buffer_start = *(pixel_ctrl_ptr + 1); // new back buffer
		//printf("Total time: %.3fs\n", 1.0*(clock() - start) / CLOCKS_PER_SEC);
		//printf("----------------------\n");
	}
}

void wait_for_vsync(){
    volatile int * pixel_ctrl_ptr = (int *)0xFF203020; //pointer to VGA controller
    int status;
    
    *pixel_ctrl_ptr = 1; //write 1 in front buffer to launch the swap process
    status = *(pixel_ctrl_ptr + 3); //poll the status bit of the status register, the other bit is A
    while((status & 0x01)!=0){
        status = *(pixel_ctrl_ptr + 3);
    }
    //after the swap, status bit will be 0
}

void black_screen(){
	for(int x = 0; x < RESOLUTION_X; x++){
		for(int y = 0; y < RESOLUTION_Y; y++){
			plot_pixel(x,y,BLACK);
		}
	}
}

void draw_background(){
	for(int x = 0; x < RESOLUTION_X; x++){
		for(int y = 0; y < RESOLUTION_Y; y++){
			clear_pixel(x, y);
		}
	}
}

void clear_screen(Ball* ball, Dune* dune, Arrow* arrow){
	clear_rectangle(RESOLUTION_X-92, 5, RESOLUTION_X-4, 15);
	
	// clear ball and dunes
	clear_ball(ball);
	clear_arrow(arrow);
	
	// clear +2500
	if (toClear > 0){
		clear_rectangle(X_2500, Y_2500, X_2500+40, Y_2500+10);
		toClear--;
	}
}

void clear_rectangle(int x0, int y0, int x1, int y1){
	for (int x = x0; x <= x1; x++)
		for(int y = y0; y <= y1; y++)
			clear_pixel(x, y);
}

void plot_pixel(int x, int y, short int line_color){
    *(short int *)(pixel_buffer_start + (y << 10) + (x << 1)) = line_color;
}

void clear_pixel(int x, int y){
    *(short int *)(pixel_buffer_start + (y << 10) + (x << 1)) = BACKGROUND + GRADIENT*y / RESOLUTION_Y;
}

void draw(Ball* ball, Dune* dune, Arrow* arrow, int score){
	// check if the ball is even on the screen
	draw_line(0, SCORE_LINE_Y, RESOLUTION_X-1, SCORE_LINE_Y, WHITE);
	draw_score(score);
	if (in_y_bounds(ball->y + ball->radius)){
		draw_ball(ball);
	}
	else{
		// update previous ball position anyways
		ball->x2 = ball->x1;
		ball->y2 = ball->y1;
		ball->x1 = ball->x;
		ball->y1 = ball->y;
		// update and draw the arrow
		update_arrow(arrow, ball->y);
		draw_arrow(arrow);
	}
	if (toDraw > 0){
		if (toDraw > FRAMES_2500-2)
			draw_2500(X_2500, Y_2500, WHITE);
		toDraw--;
		if (toDraw == 0)
			toClear = 2;
	}
	draw_dune_slice(dune, BALL_X-BALL_R, BALL_X+BALL_R);
}

double distance(int x1, int y1, int x2, int y2);

void draw_ball(Ball* ball){	
	// update ball position
	ball->x2 = ball->x1;
	ball->y2 = ball->y1;
	ball->x1 = ball->x;
	ball->y1 = ball->y;
	
	int r = ball->radius;
	int xc = ball->x; // x center
	int yc = ball->y; // y center
	int color = ball->color;
    int x = r, y = 0;
      
	// plot horizontal and vertical diameters of the circle
	if (in_y_bounds(yc)){
		for (int i = -x; i <= x; i++){
			plot_pixel(xc+i, yc, color);
		}
	}
      
    // Initialising the value of P
    int P = 1 - r;
    while (x > y){ 
        y++;  
        if (P <= 0){ // midpoint is inside or on the permiter
            P = P + 2*y + 1;
		}
        else{ // midpoint is outside the perimeter
            x--;
            P = P + 2*y - 2*x + 1;
        }
          
        // All the perimeter points have already been drawn
        if (x < y)
            break;
          
        // draw lines
		if (in_y_bounds(yc+y)){
			draw_line(xc-x, yc+y, xc+x, yc+y, color);
			//for (int i = -x; i <= x; i++){
			//	plot_pixel(xc+i, yc+y, color);
			//}	
		}
		if (in_y_bounds(yc-y)){
			for (int i = -x; i <= x; i++){
				plot_pixel(xc+i, yc-y, color);
			}	
		}
		
		// draw second pair of lines if we aren't on the edge of the octant
        if (x != y){
			if (in_y_bounds(yc+x)){
				for (int i = -y; i <= y; i++){
					plot_pixel(xc+i, yc+x, color);
				}	
			}
			if (in_y_bounds(yc-x)){
				for (int i = -y; i <= y; i++){
					plot_pixel(xc+i, yc-x, color);
				}	
			}
        }
    } 
}

void clear_ball(Ball* ball){	
	int r = ball->radius;
	int xc = ball->x2; // x center 2 frames ago
	int yc = ball->y2; // y center 2 frames ago
    int x = r, y = 0;
      
	// plot horizontal and vertical diameters of the circle
	if (in_y_bounds(yc)){
		for (int i = -x; i <= x; i++){
			clear_pixel(xc+i, yc);
		}
	}
      
    // Initialising the value of P
    int P = 1 - r;
    while (x > y){ 
        y++;  
        if (P <= 0){ // midpoint is inside or on the permiter
            P = P + 2*y + 1;
		}
        else{ // midpoint is outside the perimeter
            x--;
            P = P + 2*y - 2*x + 1;
        }
          
        // All the perimeter points have already been drawn
        if (x < y)
            break;
          
        // draw lines
		if (in_y_bounds(yc+y)){
			for (int i = -x; i <= x; i++){
				clear_pixel(xc+i, yc+y);
			}	
		}
		if (in_y_bounds(yc-y)){
			for (int i = -x; i <= x; i++){
				clear_pixel(xc+i, yc-y);
			}	
		}
		
		// draw second pair of lines if we aren't on the edge of the octant
        if (x != y){
			if (in_y_bounds(yc+x)){
				for (int i = -y; i <= y; i++){
					clear_pixel(xc+i, yc+x);
				}	
			}
			if (in_y_bounds(yc-x)){
				for (int i = -y; i <= y; i++){
					clear_pixel(xc+i, yc-x);
				}	
			}
        }
    } 
}

void draw_dune(Dune* dune){
	for(int i = 0; i < RESOLUTION_X; i++){
		for(int j = 0; j < RESOLUTION_Y - dune->dunePoints[i]; j++){
			plot_pixel(i, dune->dunePoints[i]+j, DUNE_COLOR);
		}
	}
}

void draw_dune_slice(Dune* dune, int x1, int x2){
	for(int i = x1; i <=x2; i++){
		int y_max = RESOLUTION_Y - dune->dunePoints[i];
		for(int j = 0; j < y_max; j++){
			plot_pixel(i, dune->dunePoints[i]+j, DUNE_COLOR);
		}
	}
}

void clear_running_dune(Dune* dune){
	for(int x = 0; x < RESOLUTION_X; x++){
		if(dune->dunePoints[x] > dune->dunePoints2[x]){
			clear_line(x, dune->dunePoints2[x],x, dune->dunePoints[x]);
		}
		else{
			draw_line(x, dune->dunePoints2[x], x, dune->dunePoints[x], DUNE_COLOR);
		}
	}
}

void clear_all_dune(Dune* dune){
	for(int x = 0; x < RESOLUTION_X; x++){
		draw_line(x, dune->dunePoints2[x], x, RESOLUTION_Y, BLACK);
	}
}

	
void draw_isosceles_triangle(int h, int w, int x, int y, short int color);
void clear_isosceles_triangle(int h, int w, int x, int y, short int color);
void draw_arrow(Arrow* arrow){
	// update previous frames
	arrow->h2 = arrow->h1;
	arrow->h1 = arrow->h;
	
	// draw the triangle
	int half = arrow->h/2;
	draw_isosceles_triangle(half, half, arrow->x, arrow->y, arrow->color);
	// draw rest of arrow
	for (int i = 0; i < half; i++){
		for (int j = -half/5; j <= half/5; j++){
			plot_pixel(arrow->x+j, arrow->y+half+i, arrow->color);	
		}
	}
}

void clear_arrow(Arrow* arrow){
	// draw the triangle
	int half = arrow->h2/2;
	clear_isosceles_triangle(half, half, arrow->x, arrow->y, BLACK);
	// draw rest of arrow
	for (int i = 0; i < half; i++){
		for (int j = -half/5; j <= half/5; j++){
			clear_pixel(arrow->x+j, arrow->y+half+i);	
		}
	}
}

void draw_isosceles_triangle(int h, int w, int x, int y, short int color){
	// x, y is tip of triangle
	double slope = 2*h / w;
	
	// draw triangle from top down
	for (int i = 0; i <= w; i++){
		int dx = (int) round(i / slope);
		for (int j = -dx; j <= dx; j++){
			plot_pixel(x+j, y+i, color);
		}
	}
}

void clear_isosceles_triangle(int h, int w, int x, int y, short int color){
	// x, y is tip of triangle
	double slope = 2*h / w;
	
	// draw triangle from top down
	for (int i = 0; i <= w; i++){
		int dx = (int) round(i / slope);
		for (int j = -dx; j <= dx; j++){
			clear_pixel(x+j, y+i);
		}
	}
}

void update_arrow(Arrow* arrow, int y){
	y = -(y+BALL_R); // distance from bottom of ball to top of screen
	int height = (int) round(y*ARROW_SCALE + MIN_ARROW_HEIGHT);
	arrow->h = min(MAX_ARROW_HEIGHT, height);
	
	// update arrow color
	int r = 0x1F; // 31
	int b = 0x1F - y/2;
	int g = 0x3F;
	if (b < 0){
		b = 0;
		g = max(0, g-(y-62)/2);
	}
	arrow->color = (r << 11) | (g << 5) | b;
}

int min(int a, int b){
	if (a < b) return a;
	else return b;
}

int max(int a, int b){
	if (a < b) return b;
	else return a;
}

double distance(int x1, int y1, int x2, int y2){
	return sqrt(pow((x1-x2), 2) + pow((y1-y2), 2));
}

// helper functions
bool in_bounds(int x, int y){
	return in_y_bounds(y) && in_x_bounds(x);	
}

bool in_y_bounds(int y){
	return (y >= 0) && (y < RESOLUTION_Y);	
}
bool in_x_bounds(int x){
	return (x >= 0) && (x < RESOLUTION_X);
}

//return true if ball is touching dune
bool isBallTouchingDune(Ball* ball, Dune* dune){
	if(ball->y > RESOLUTION_Y) return true;
	for(int i = 0; i < ball->radius; i++){
		if((ball->y + ball->radius) > dune->dunePoints[ball->x+i]) return true;
		if((ball->y + ball->radius) > dune->dunePoints[ball->x-i]) return true;
	}
	return false;
		
}

void draw_DUNE(short int color);
void draw_press_space_to_start(int x, int y, short int color);
void draw_starting_screen(){
    //moving ball
	static int calculation = 1;
	double accel = 1;
    static Ball ball = {.x = BALL_X, .y = BALL_Y, .dx = 0, .dy = 0, .color = WHITE, .radius = BALL_R,
						.x1 = BALL_X, .y1 = BALL_Y, .x2 = BALL_X, .y2 = BALL_Y};
	ball.color = set_ball_color();
    static Dune dune;
	int x = 0;
	int amplitude;
    if(calculation){
		while(x < RESOLUTION_X){
     		if(x <= DUNE_PERIOD) amplitude = 20;
			else amplitude = 10;
    		dune.dunePoints[x] = (int) amplitude*sin(DUNE_FREQUENCY*(x)) + 200;
  			dune.duneAngles[x] = atan(amplitude*DUNE_FREQUENCY*cos(DUNE_FREQUENCY*(x)));
        	x++;
		}
		calculation = 0;
		draw_press_space_to_start(83, 120,WHITE);
	}
    draw_DUNE(WHITE);
	
	clear_ball(&ball);
    draw_ball(&ball);
    draw_dune(&dune);
        
        
   	if(isBallTouchingDune(&ball,&dune)){
		for(int i = 0; i < ball.radius; i++){
			if(ball.y+ball.radius > dune.dunePoints[ball.x+i]) ball.y = dune.dunePoints[ball.x+i]-ball.radius;
			if(ball.y+ball.radius > dune.dunePoints[ball.x-i]) ball.y = dune.dunePoints[ball.x-i]-ball.radius;
		}
		double angle = dune.duneAngles[ball.x];
        double speed = sqrt(ball.dx*ball.dx + ball.dy*ball.dy)*cos(angle);
		ball.dx = speed*cos(angle);
		ball.dy = speed*sin(angle);
		ball.y += ball.dy + 0.5*accel;
		ball.x += ball.dx;
    }
    else{
      	ball.y += (int) ball.dy + 0.5*accel;
       	ball.dy += accel;
		ball.x += ball.dx;
    }

	if(!in_bounds(ball.x,ball.y)){
		ball.x = BALL_X;
		ball.y = 0;
		ball.dy = sqrt(ball.dx*ball.dx + ball.dy*ball.dy);
		ball.dx = 0;
	}
}

void draw_vertical(int x0, int y0, int x1, int y1, short int line_color);
void draw_horizontal(int x0, int y0, int x1, int y1, short int line_color);
void draw_line(int x0, int y0, int x1, int y1, short int line_color){
	if (x0 == x1){
		draw_vertical(x0, y0, x1, y1, line_color);
		return;
	}
	else if (y0 == y1) {
		draw_horizontal(x0, y0, x1, y1, line_color);
		return;	
	}
    int is_steep = ((ABS(y1-y0) > ABS(x1-x0))? 1 : 0); 
    if(is_steep){ //swap the variable around if the line is steep
        int temp = y0;
        y0 = x0;
        x0 = temp;
        temp = y1;
        y1 = x1;
        x1 = temp;
    }
    if(x0 > x1){  // we want to draw from smallest to biggest for simplicity
        int temp = x1;
        x1 = x0;
        x0 = temp;
        temp = y1;
        y1 = y0;
        y0 = temp;
    }
    
    int deltaX = x1 - x0;   
    int deltaY = ABS(y1 - y0);
    int error = -(deltaX/2); //error
    int x = x0;
    int y = y0;
    int y_step;  //add if y0 is below, but substract is y0 is above
    if(y0 < y1) y_step = 1;
    else y_step = -1;
        
    while(x<=x1){
        if(is_steep) plot_pixel(y,x,line_color); //draw the pixel
        else plot_pixel(x,y,line_color); 
        x++;
        error = error + deltaY; //determine if we need to increment y or not
        if(error >= 0){
             y = y + y_step;
             error = error - deltaX;
        }
    }
    
    //if(is_steep) plot_pixel(y,x,line_color); //draw the last pixel
    //else plot_pixel(x,y,line_color); 
}

void draw_vertical(int x0, int y0, int x1, int y1, short int line_color){
	if (y0 > y1){
		int temp = y0;
		y0 = y1;
		y1 = temp;
	}
	
	for (int y = y0; y <=y1; y++){
		plot_pixel(x0, y, line_color);
	}
}
void draw_horizontal(int x0, int y0, int x1, int y1, short int line_color){
	if (x0 > x1){
		int temp = x0;
		x0 = x1;
		x1 = temp;
	}
	
	for (int x = x0; x <=x1; x++){
		plot_pixel(x, y0, line_color);
	}	
}

void clear_vertical(int x0, int y0, int x1, int y1);
void clear_horizontal(int x0, int y0, int x1, int y1);
void clear_line(int x0, int y0, int x1, int y1){
	if (x0 == x1){
		clear_vertical(x0, y0, x1, y1);
		return;
	}
	else if (y0 == y1) {
		clear_horizontal(x0, y0, x1, y1);
		return;	
	}
	
    int is_steep = ((ABS(y1-y0) > ABS(x1-x0))? 1 : 0); 
    if(is_steep){ //swap the variable around if the line is steep
        int temp = y0;
        y0 = x0;
        x0 = temp;
        temp = y1;
        y1 = x1;
        x1 = temp;
    }
    if(x0 > x1){  // we want to draw from smallest to biggest for simplicity
        int temp = x1;
        x1 = x0;
        x0 = temp;
        temp = y1;
        y1 = y0;
        y0 = temp;
    }
    
    int deltaX = x1 - x0;   
    int deltaY = ABS(y1 - y0);
    int error = -(deltaX/2); //error
    int x = x0;
    int y = y0;
    int y_step;  //add if y0 is below, but substract is y0 is above
    if(y0 < y1) y_step = 1;
    else y_step = -1;
        
    while(x<=x1){
        if(is_steep) clear_pixel(y,x); //draw the pixel
        else clear_pixel(x,y); 
        x++;
        error = error + deltaY; //determine if we need to increment y or not
        if(error >= 0){
             y = y + y_step;
             error = error - deltaX;
        }
    }
    
    //if(is_steep) plot_pixel(y,x,line_color); //draw the last pixel
    //else plot_pixel(x,y,line_color); 
}

void clear_vertical(int x0, int y0, int x1, int y1){
	if (y0 > y1){
		int temp = y0;
		y0 = y1;
		y1 = temp;
	}
	
	for (int y = y0; y <=y1; y++){
		clear_pixel(x0, y);
	}
}
void clear_horizontal(int x0, int y0, int x1, int y1){
	if (x0 > x1){
		int temp = x0;
		x0 = x1;
		x1 = temp;
	}
	
	short int line_color = (short int) BACKGROUND + GRADIENT*y0 / RESOLUTION_Y;
	for (int x = x0; x <=x1; x++){
		plot_pixel(x, y0, line_color);
	}	
}

void display_score(int score){
	// extract the digits
	int digits[6];
	for (int i = 0; i < 6; i++){
		digits[i] = (score % 10);
		score /= 10;
	}
	
	// define hex base
	volatile int * HEX3_HEX0_ptr = (int *)HEX3_HEX0_BASE;
	volatile int * HEX5_HEX4_ptr = (int *)HEX5_HEX4_BASE;

	unsigned char hex_segs[] = {0, 0, 0, 0, 0, 0, 0, 0};
	unsigned int shift_buffer, nibble;
	shift_buffer = (digits[5] << 20) | (digits[4] << 16) | (digits[3] << 12) 
		| (digits[2] << 8) | (digits[1] << 4) | digits[0];
	
	for (int i = 0; i < 6; ++i) {
		nibble = shift_buffer & 0x0000000F; // character is in rightmost nibble
		hex_segs[i] = seven_seg_decode_table[nibble];
		shift_buffer = shift_buffer >> 4;
	}
	/* drive the hex displays */
	*(HEX3_HEX0_ptr) = *(int *)(hex_segs);
	*(HEX5_HEX4_ptr) = *(int *)(hex_segs + 4);

}

void receive_bytes(int n){
	volatile int * PS2_ptr = (int *)PS2_BASE;
	int received  = 0;
	while (received < n) {
		int PS2_data = *(PS2_ptr); 
		int RVALID = PS2_data & 0x8000; // extract the RVALID field
		if (RVALID) {
			char byte = PS2_data & 0xFF;
			printf("Received %X from keyboard reset\n", byte);
			received++;
		}
	}
}

void space_key_clicked(bool* flag){
	volatile int * PS2_ptr = (int *)PS2_BASE;
	int PS2_data, RVALID, RAVAIL;
	bool makeFlag = false;
	bool breakFlag = false;
	char byte;
	
	PS2_data = *(PS2_ptr); 
	RAVAIL = (PS2_data & 0xFFFF0000) >> 16;
	RVALID = (PS2_data & 0x8000);
	if (!RVALID){
		return;
	}
	
	byte = PS2_data & 0xFF;
	if (byte == 0x29){
		makeFlag = true;
	}
	else if (byte == 0xF0){
		breakFlag = true;
	}
	
	for (int i = 0; i < RAVAIL; i++){
		PS2_data = *(PS2_ptr);
		byte = PS2_data & 0xFF;
		if (byte == 0x29){
			makeFlag = true;
		}
		else if (byte == 0xF0){
			breakFlag = true;
		}
	}
	
	if (breakFlag){
		*flag = false;
	}
	else if (makeFlag){
		*flag = true;
	}
}
	
// read from switches
int read_SW(){
	volatile int* SW_ptr = (int*) SW_BASE;
	return *SW_ptr;
}

short int set_ball_color(){
	int sel = read_SW();
	if (sel == 0){
		return RED;
	}
	
	int i = 0;
	while ((sel & 0x1) != 1 && (i < 10)){
		sel /= 2;
		i++;
	}
	if (i >= 10) return RED;
	else return ball_colors[i];
}

void draw_DUNE(short int color){
	//letters start at x=56 to 264
    //letters are 42 wide and 82 tall, with 5 blank each side, thickness of 7
    //letters are drawn from y = 24 to y = 106, middle is at 66
    
    //draw DUNE letters
    //letter D, goes from x =[61,103]
	for(int y = 0; y <= 42; y++){
         int x = (int) sqrt(1764 - y*y);
         draw_line(60, 65+y, 60+x , 65+y, color);
         draw_line(60, 65-y, 60+x , 65-y, color);
	}
    for(int y = 0; y <= 28; y++){
		 int x = (int) sqrt(784 - y*y);
         clear_line(67, 65+y, 67+x, 65+y);
         clear_line(67, 65-y, 67+x, 65-y);
	}
    //letter U, goes from x = [113,155]
    for(int x = 113; x <= 120; x++){
         draw_line(x, 24, x, 106, color);
    } 
    for(int x = 121; x < 148; x++){
         draw_line(x, 99, x, 106, color);
    }
	for(int x = 148; x <= 155; x++){
         draw_line(x, 24, x, 106, color);
    }
    //letter N x = [165, 207]
    for(int x = 165; x <= 172; x++){
         draw_line(x, 24, x, 106, color);
    }
    for(int i = 0; i<=7; i++){
	     draw_line(173,24+i, 200,99+i, color);
    }
    for(int x = 200; x <= 207; x++){
         draw_line(x, 24, x, 106, color);
    }
    //letter E x= [217, 259]
    for(int x = 217; x <= 224; x++){
         draw_line(x, 24, x, 106, color);
    }
    for(int y = 24; y<=31; y++){
     	draw_line(225, y, 259, y, color);
		draw_line(225, y+38, 259, y+38, color);
        draw_line(225, y+75, 259, y+75, color);
	}
}

void draw_P(int x, int y, short int color){
	draw_line(x, y, x, y+10, color);
	draw_line(x, y, x+5, y, color);
	draw_line(x, y+5, x+5, y+5, color);
	x+=5;
	draw_line(x, y, x, y+5, color);
}

void draw_R(int x, int y, short int color){
	draw_line(x, y, x, y+10, color);
	draw_line(x, y, x+5, y, color);
	draw_line(x, y+5, x+5, y+5, color);
	draw_line(x+5, y, x+5, y+5, color);
	draw_line(x, y+5, x+5, y+10, color);
}

void draw_E(int x, int y, short int color){
	draw_line(x, y, x, y+10, color);
	draw_line(x, y, x+5, y, color);
	draw_line(x, y+5, x+5, y+5, color);
	draw_line(x, y+10, x+5, y+10, color);
}

//same for S and 5
void draw_S(int x, int y, short int color){
	draw_line(x, y, x, y+5, color);
	draw_line(x, y, x+5, y, color);
	draw_line(x, y+5, x+5, y+5, color);
	draw_line(x, y+10, x+5, y+10, color);
	draw_line(x+5, y+5, x+5, y+10, color);
}

void draw_A(int x, int y, short int color){
	draw_line(x, y, x, y+10, color);
	draw_line(x+5, y, x+5, y+10, color);
	draw_line(x, y, x+5, y, color);
	draw_line(x, y+5, x+5, y+5, color);
}

void draw_C(int x, int y, short int color){
	draw_line(x, y, x, y+10, color);
	draw_line(x, y, x+5, y, color);
	draw_line(x, y+10, x+5, y+10, color);
}

void draw_T(int x, int y, short int color){
	draw_line(x, y, x+5, y, color);
	draw_line(x+2, y, x+2, y+10, color);
}

//draw 0 and O
void draw_O(int x, int y, short int color){
	draw_line(x, y, x, y+10, color);
	draw_line(x, y, x+5, y, color);
	draw_line(x, y+10, x+5, y+10, color);
	draw_line(x+5, y, x+5, y+10, color);
}

void draw_quote(int x, int y, short int color){
	draw_line(x+1, y, x+1, y+2, color);
	draw_line(x+3, y, x+3, y+2, color);
}

void draw_I(int x, int y, short int color){
	draw_line(x+2, y, x+2, y+10,color);
	draw_line(x, y, x+5, y, color);
	draw_line(x, y+10, x+5, y+10, color);
}

void draw_N(int x, int y, short int color){
	draw_line(x, y, x, y+10, color);
	draw_line(x+5, y, x+5, y+10, color);
	draw_line(x, y, x+5, y+10, color);
}

void draw_G(int x, int y, short int color){
	draw_line(x, y, x, y+10, color);
	draw_line(x, y, x+5, y, color);
	draw_line(x+3, y+5, x+5, y+5, color);
	draw_line(x, y+10, x+5, y+10, color);
	draw_line(x+5, y+5, x+5, y+10, color);
}

void draw_M(int x, int y, short int color){
	draw_line(x, y, x, y+10, color);
	draw_line(x+5, y, x+5, y+10, color);
	draw_line(x, y, x+2, y+5, color);
	draw_line(x+3, y+5, x+5, y, color);
}

void draw_V(int x, int y, short int color){
	draw_line(x, y, x+2, y+10, color);
	draw_line(x+3, y+10, x+5, y, color);
}

void draw_two_points(int x, int y, short int color){
	draw_line(x+1, y+2, x+1, y+4, color);
	draw_line(x+2, y+2, x+2, y+4, color);
	draw_line(x+1, y+6, x+1, y+8, color);
	draw_line(x+2, y+6, x+2, y+8, color);
}
	
void draw_1(int x, int y, short int color){
	draw_line(x+2, y, x+2, y+10,color);
	draw_line(x, y+2, x+2, y,color);
	draw_line(x, y+10, x+5, y+10, color);
}

void draw_2(int x, int y, short int color){
	draw_line(x+5, y, x+5, y+5, color);
	draw_line(x, y, x+5, y, color);
	draw_line(x, y+5, x+5, y+5, color);
	draw_line(x, y+10, x+5, y+10, color);
	draw_line(x, y+5, x, y+10, color);
}

void draw_3(int x, int y, short int color){
	draw_line(x+5, y, x+5, y+10, color);
	draw_line(x, y, x+5, y, color);
	draw_line(x, y+5, x+5, y+5, color);
	draw_line(x, y+10, x+5, y+10, color);
}

void draw_4(int x, int y, short int color){
	draw_line(x+5, y, x+5, y+10, color);
	draw_line(x, y, x, y+5, color);
	draw_line(x, y+5, x+5, y+5, color);
}

void draw_6(int x, int y, short int color){
	draw_line(x, y, x, y+10, color);
	draw_line(x, y, x+5, y, color);
	draw_line(x, y+5, x+5, y+5, color);
	draw_line(x, y+10, x+5, y+10, color);
	draw_line(x+5, y+5, x+5, y+10, color);
}

void draw_7(int x, int y, short int color){
	draw_line(x+5, y, x+5, y+10, color);
	draw_line(x, y, x, y+5, color);
	draw_line(x, y, x+5, y, color);
}

void draw_8(int x, int y, short int color){
	draw_line(x, y, x, y+10, color);
	draw_line(x+5, y, x+5, y+10, color);
	draw_line(x, y, x+5, y, color);
	draw_line(x, y+5, x+5, y+5, color);
	draw_line(x, y+10, x+5, y+10, color);
}

void draw_9(int x, int y, short int color){
	draw_line(x, y, x, y+5, color);
	draw_line(x+5, y, x+5, y+10, color);
	draw_line(x, y, x+5, y, color);
	draw_line(x, y+5, x+5, y+5, color);
	draw_line(x, y+10, x+5, y+10, color);
}

void draw_press_space_to_start(int x, int y, short int color){
	draw_P(x, y, color);
	x+=7;
	draw_R(x, y, color);
	x+=7;
	draw_E(x, y, color);
	x+=7;
	draw_S(x, y, color);
	x+=7;
	draw_S(x, y, color);
	x+=14;
	draw_quote(x, y, color);
	x+=7;
	draw_S(x, y, color);
	x+=7;
	draw_P(x, y, color);
	x+=7;
	draw_A(x, y, color);
	x+=7;
	draw_C(x, y, color);
	x+=7;
	draw_E(x, y, color);
	x+=7;
	draw_quote(x, y, color);
	x+=14;
	draw_T(x, y, color);
	x+=7;
	draw_O(x, y, color);
	x+=14;
	draw_S(x, y, color);
	x+=7;
	draw_T(x, y, color);
	x+=7;
	draw_A(x, y, color);
	x+=7;
	draw_R(x, y, color);
	x+=7;
	draw_T(x, y, color);
	x+=7;
}

void draw_game_over(int x, int y, short int color){
	draw_G(x,y,color);
	x+=7;
	draw_A(x,y,color);
	x+=7;
	draw_M(x,y,color);
	x+=7;
	draw_E(x,y,color);
	x+=14;
	draw_O(x,y,color);
	x+=7;
	draw_V(x,y,color);
	x+=7;
	draw_E(x,y,color);
	x+=7;
	draw_R(x,y,color);
	x+=7;
}

void draw_score(int points){
	int x = RESOLUTION_X - 11;
	int y = 5;
	if(points == 0){
		draw_O(x,y,WHITE);
		x-= 7;
		points = points/10;
	}
	while(points!=0){
		int number = points%10;
		if(number == 1) draw_1(x,y,WHITE);
		else if(number == 2) draw_2(x,y,WHITE);
		else if(number == 3) draw_3(x,y,WHITE);
		else if(number == 4) draw_4(x,y,WHITE);
		else if(number == 5) draw_S(x,y,WHITE);
		else if(number == 6) draw_6(x,y,WHITE);
		else if(number == 7) draw_7(x,y,WHITE);
		else if(number == 8) draw_8(x,y,WHITE);
		else if(number == 9) draw_9(x,y,WHITE);
		else draw_O(x,y,WHITE);
		x-= 7;
		points = points/10;
	}
	
	draw_two_points(x,y,WHITE);
	x-=7;
	draw_E(x,y,WHITE);
	x-=7;
	draw_R(x,y,WHITE);
	x-=7;
	draw_O(x,y,WHITE);
	x-=7;
	draw_C(x,y,WHITE);
	x-=7;
	draw_S(x,y,WHITE);
}

void draw_game_over_screen(Ball* ball, Dune* dune, int points, int frames){
	draw_background();
	draw_line(0, SCORE_LINE_Y, RESOLUTION_X-1, SCORE_LINE_Y, WHITE);
	draw_ball(ball);
	draw_dune(dune);	
	draw_game_over(130, 40,WHITE);
	if (frames % 2 == 1) draw_press_space_to_start(83, 80, WHITE);
	draw_score(points);
}

void draw_2500(int x, int y, short int color){
	// draw the plus sign
	draw_line(x, y+3, x+5, y+3, WHITE);
	draw_line(x+3, y+1, x+3, y+6, WHITE);
	x+=7;
	draw_2(x,y,color);
	x+=7;
	draw_S(x,y,color);
	x+=7;
	draw_O(x,y,color);
	x+=7;
	draw_O(x,y,color);
	x+=7;
}
	



	


