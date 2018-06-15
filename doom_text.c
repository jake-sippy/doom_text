/* This code defines a terminal-based,
 * wolfenstein-inspired raycasting renderer
 * of 2-dimensional maps. This program allows the
 * player to move using the WASD keys for cardinal
 * movement, the LEFT and RIGHT arrows to rotate
 * and the + and - keys to increase and decrease
 * the player's field of vision respectively.
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

// generated map dimensions
#define MIN_WIDTH 20
#define MAX_WIDTH 30
#define MIN_HEIGHT 20
#define MAX_HEIGHT 30

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
float playerA = M_PI / 2.0f;

// players field of view
float playerFOV = M_PI / 4.0f;

// current map data
int mapWidth;
int mapHeight;
char *map;

// helper methods
bool handleUserInput(void);
void generateNewMap(int width, int height);
void carveMaze(int x, int y, int width, int height);

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
  srand(time(NULL));          // set random seed


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
    free(map);
    return 1;
  }

  /* game loop */
  mapWidth  = 23;
  mapHeight = mapWidth;
  generateNewMap(mapWidth, mapHeight);


  for (int row = 0; row < mapHeight; row++) {
    fprintf(stderr, "row%2d: %.*s\n", row, mapWidth, map);
  }

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
        /* if (testX < 0 || testX >= mapWidth || */
        /*     testY < 0 || testY > mapHeight) { */
        /*   // the ray extends past the map boundaries */
        /*   hit = true; */
        /*   distanceToWall = MAX_DEPTH; */
        /* } else  */
        if (map[testY * mapWidth + testX] == '#') {
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
    int mapstartX = w - 1 - (2 * mapWidth);
    for (int row = 0; row < mapHeight; row++) {
      for (int col = 0; col < mapWidth; col++) {
        move(row, mapstartX + 2 * col);
        if (map[col + mapWidth * row] == '#')
          printw("[]");
        else
          printw("  ");
      }
    }

    move((int) playerY, mapstartX + 2 * (int) playerX);
    printw("><");

    if (DEBUG) {
      // print debug info
      move(h - 1, 0);
      clrtoeol();
      printw("Angle: %.3f X: %f Y: %f FOV: %f Fps: %d Cols: %d, Rows: %d",
             playerA, playerX, playerY, playerFOV, fps, w, h);
      attroff(COLOR_PAIR(TEXT));
    }

    refresh();

    clock_gettime(CLOCK_REALTIME, &end);
    fps = 1000000000.0f / (end.tv_nsec - start.tv_nsec);
  } // end of game loop

  // cleanup
  endwin();
  free(map);
  return 0;
}

// updates globals based on user input, returns
// whether the user hit the quit button
bool handleUserInput(void) {
  int ch = getch();
  float newX = playerX;
  float newY = playerY;
  switch (ch) {
    case 'w':
      newX = playerX + 0.5 * cos(playerA);
      newY = playerY + 0.5 * sin(playerA);
      break;
    case 'a':
      newX = playerX + 0.5 * sin(playerA);
      newY = playerY - 0.5 * cos(playerA);
      break;
    case 's':
      newX = playerX - 0.5 * cos(playerA);
      newY = playerY - 0.5 * sin(playerA);
      break;
    case 'd':
      newX = playerX - 0.5 * sin(playerA);
      newY = playerY + 0.5 * cos(playerA);
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

// used to generate a new map
void generateNewMap(int width, int height) {
  // free the old map, and allocate a new map with
  // the right size
  map = malloc(width * height * sizeof(char));
  if (map == NULL) {
    fprintf(stderr, "[ERROR] Out of memory");
  }

  int x, y;

  /* Initialize the new map to be all walls */
  for (x = 0; x < width * height; x++) {
    map[x] = '#';
  }

  /* Manually start the first tunnel */
  map[1 + width * 1] = ' ';
  map[1 + width * 2] = ' ';

  /* Carve the maze */
  for (y = 1; y < height; y += 2) {
    for (x = 1; x < width; x += 2) {
      carveMaze(x, y, width, height);
    }
  }

  /* Set up entrance and exit */
  map[1 + width * 0] = ' ';
  map[(width - 2) + width * (height - 1)] = ' ';

  // set player at the start
  playerX = 1.5f;
  playerY = 0.5f;
}

void carveMaze(int x, int y, int width, int height) {
  int x1, y1;
  int x2, y2;
  int dx, dy;
  int dir, count;

  dir = rand() % 4;
  count = 0;
  while (count < 4) {
    dx = 0; dy = 0;
    switch (dir) {
      case 0:  dx =  1; break;
      case 1:  dy =  1; break;
      case 2:  dx = -1; break;
      default: dy = -1; break;
    }

    x1 = x + dx;
    y1 = y + dy;
    x2 = x1 + dx;
    y2 = y1 + dy;

    if (x2 > 0 && x2 < width && y2 > 0 && y2 < height &&
        map[x1 + width * y1] == '#' && map[x2 + width * y2] == '#') {

      map[x1 + width * y1] = ' ';
      map[x2 + width * y2] = ' ';
      x = x2; y = y2;
      dir = rand() % 4;
      count = 0;
    } else {
      dir = (dir + 1) % 4;
      count += 1;
    }
  }
}
