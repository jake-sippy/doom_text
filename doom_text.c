/* This code defines a terminal-based,
 * wolfenstein-inspired raycasting renderer
 * of 2-dimensional maps. Given a hardcoded
 * string representing the map data, it visually
 * renders the walls (represented as '#') and
 * allows the player to move using the WASD keys
 * It is implemented using ncurses and tested on
 * GNU/Linux.
 *
 * Created by Jake Sippy 2018
 */

#include <ncurses.h>
#include <math.h>
#include <time.h>
#include <stdlib.h>

// show debug info?
#define DEBUG true

// how far the player can see
#define MAX_DEPTH 25

// colors
#define BLACK 0
#define WHITE 1

// color pairs
#define TEXT 0
#define BLACK_ON_BLACK 1

// num of different shades
#define SHADES 20

// starting number for wall colors
#define WALL_SHADE_START 10

#define FLOOR_SHADE_START (WALL_SHADE_START + SHADES)

// characters to draw with
#define WALL_CHAR ' '
#define FLOOR_CHAR ' '

// players position and angle
float playerX = 8;
float playerY = 8;
float playerA = 0.0f;

// players field of view
float playerFOV = M_PI / 4.0f;

// hardcoded map
const int mapWidth = 20;
const int mapHeight = 20;
const char *map = "####################"
                  "#..................#"
                  "#..................#"
                  "#..................#"
                  "###############....#"
                  "#..................#"
                  "#..................#"
                  "#..................#"
                  "#..................#"
                  "#..................#"
                  "#..................#"
                  "#..................#"
                  "#..................#"
                  "#..................#"
                  "#..........#########"
                  "#..........#.......#"
                  "#..................#"
                  "#..........#.......#"
                  "#..........#.......#"
                  "####################";

// helper method
bool handleUserInput();

int main() {
  /* ncurses settings */
  initscr();                  // init main window
  cbreak();                   // dont buffer input
  noecho();                   // dont echo input
  nonl();                     // dont echo return -> newline
  nodelay(stdscr, TRUE);      // dont block on getch
  intrflush(stdscr, FALSE);   // dont flush output on interupt
  keypad(stdscr, TRUE);       // enable terminal keypad
  curs_set(0);                // hide cursor


  /* color definitions */
  start_color();

  // defining white on black pair for text
  init_color(BLACK, 0, 0, 0);     // default black
  init_color(WHITE, 0, 0, 0);     // default white
  init_pair(TEXT, WHITE, BLACK);  // pair for white on black text

  // defining the darkening shades for the walls
  for (int i = WALL_SHADE_START; i < SHADES + WALL_SHADE_START; i++) {
    short shade = ((short) (800.0f / SHADES)) * (i - WALL_SHADE_START);
    init_color(i, shade, shade, shade);
    init_pair(i, i, i);
  }

  // defining the darkening shades for the floor
  for (int i = FLOOR_SHADE_START; i < SHADES + FLOOR_SHADE_START; i++) {
    short shade = ((short) (600.0f / SHADES)) * (i - FLOOR_SHADE_START);
    init_color(i, 0, shade, 0);
    init_pair(i, i, i);
  }

  // define useful black on black color pair
  init_pair(BLACK_ON_BLACK, BLACK, BLACK);

  // check that we can actually use color
  if (!can_change_color()) {
    printf("This terminal does not support color");
    endwin();
    return 1;
  }

  /* game loop */
  struct timespec start, end;
  int fps = 0;
  while (1) {
    // getting fps
    clock_gettime(CLOCK_REALTIME, &start);

    // current height and width of the terminal screen
    int h, w;
    getmaxyx(stdscr, h, w);

    /* user input */
    if (handleUserInput()) {
      // user quit the program
      break;
    }

    /* Raycasting */
    erase();
    for (int col = 0; col < w; col++) {
      // get the current angle of the ray to cast
      float rayAngle = (playerA - playerFOV / 2) +
        ((float)col / w) * playerFOV;

      float unitX = cos(rayAngle);
      float unitY = sin(rayAngle);

      // calculate distance to a wall
      float distanceToWall = 0;
      bool hit = false;

      while (!hit && distanceToWall < MAX_DEPTH) {
        distanceToWall += 0.1f;

        int testX = (int) (playerX + unitX * distanceToWall);
        int testY = (int) (playerY + unitY * distanceToWall);
        if (testX < 0 || testX >= mapWidth ||
            testY < 0 || testY > mapHeight) {
          // the ray extends past the map boundaries
          hit = true;
          distanceToWall = MAX_DEPTH;
        } else if (map[testY * mapWidth + testX] == '#') {
          // the ray has just hit a block
          hit = true;
        }
      }

      // caclulate how high to draw the wall
      int ceiling = (h / 2.0) - (h / distanceToWall);
      if (ceiling < 0) ceiling = 0;
      int floor = h - ceiling;

      // determine which color pair to draw wall with
      short pair = BLACK_ON_BLACK;
      for (int i = WALL_SHADE_START + SHADES - 1; i >= WALL_SHADE_START; i--) {
        if (distanceToWall < ((float)MAX_DEPTH / (i - WALL_SHADE_START))) {
          pair = i;
          break;
        }
      }

      // draw the wall
      attron(COLOR_PAIR(pair));
      move(ceiling, col);
      vline(WALL_CHAR, floor - ceiling);
      attroff(COLOR_PAIR(pair));

      // draw the floor one character at a time
      attron(TEXT);
      for (int i = floor; i < h; i++) {
        float b = (i - h / 2.0f) / (h / 2.0f);
        pair = (b * (SHADES - 1)) + FLOOR_SHADE_START;
        attron(COLOR_PAIR(pair));
        move(i, col);
        addch(FLOOR_CHAR);
        attroff(COLOR_PAIR(pair));
      }
    }

    // print map and character
    attron(COLOR_PAIR(TEXT));
    for (int row = 0; row < mapHeight; row++) {
      move(row, w - mapWidth);
      printw("%.*s", mapWidth, map + (row * mapWidth));
    }

    move((int) playerY, (int) playerX + w - mapWidth);
    addch('@');

    if (DEBUG) {
      // print debug info
      move(h - 1, 0);
      clrtoeol();
      printw("Angle: %.3f X: %f Y: %f FOV: %f Fps: %d Cols: %d, Rows: %d",
             playerA, playerX, playerY, playerFOV, fps, h, w);
      attroff(COLOR_PAIR(TEXT));
    }

    refresh();

    clock_gettime(CLOCK_REALTIME, &end);
    fps = 1000000000.0f / (end.tv_nsec - start.tv_nsec);
  }

  // cleanup
  endwin();
  return 0;
}

// updates globals based on user input, returns
// whether the user hit the quit button
bool handleUserInput() {
  int ch = getch();
  float newX = playerX;
  float newY = playerY;
  switch (ch) {
    case 'w':
      newX = playerX + cos(playerA);
      newY = playerY + sin(playerA);
      break;
    case 'a':
      newX = playerX + sin(playerA);
      newY = playerY - cos(playerA);
      break;
    case 's':
      newX = playerX - cos(playerA);
      newY = playerY - sin(playerA);
      break;
    case 'd':
      newX = playerX - sin(playerA);
      newY = playerY + cos(playerA);
      break;
    case KEY_LEFT:
      playerA -= M_PI / 32.0;
      break;
    case KEY_RIGHT:
      playerA += M_PI / 32.0;
      break;
    case '+':
      playerFOV += M_PI / 32.0;
      break;
    case '-':
      playerFOV -= M_PI / 32.0;
      break;
    case 'q':
      return true;
  }

  /* collision detection */
  if (map[(int) newY * mapWidth + (int) newX] != '#') {
    playerX = newX;
    playerY = newY;
  }

  return false;
}
