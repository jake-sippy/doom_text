#include <ncurses.h>
#include <unistd.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <stdlib.h>

#define MAX_DEPTH 20

float playerX = 8;
float playerY = 8;
float playerA = 0.0f;

float playerFOV = M_PI / 4.0f;

int mapWidth = 20;
int mapHeight = 20;
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


int main() {
  // initialize ncurses settings
  initscr();                  // init main window
  cbreak();                   // dont buffer input
  noecho();                   // dont echo input
  nonl();                     // dont echo return -> newline
  nodelay(stdscr, TRUE);      // dont block on getch
  intrflush(stdscr, FALSE);   // dont flush output on interupt
  keypad(stdscr, TRUE);       // enable terminal keypad
  curs_set(0);                // hide cursor


  // colors
  start_color();

  for (int i = 1; i <= 8; i++) {
    short shade = 400 + i * 50;
    init_color(i, shade, shade, shade);
    init_pair(i, i, i);
  }

  // default colors
  init_color(0, 0, 0, 0);            // black
  init_pair(0, 8, 0);

  if (!can_change_color()) {
    return 2;
  }

  // start the game loop
  while (1) {
    // current height and width vars
    int h, w;
    getmaxyx(stdscr, h, w);

    // handle user input
    int ch = getch();
    float newX = playerX;
    float newY = playerY;
    switch (ch) {
      case 'w':
        newX = playerX + cos(playerA);
        newY = playerY + sin(playerA);
        break;
      case 's':
        newX = playerX - cos(playerA);
        newY = playerY - sin(playerA);
        break;
      case 'a':
        playerA -= M_PI / 32.0;
        break;
      case 'd':
        playerA += M_PI / 32.0;
        break;
      case 'q':
        endwin();
        return 0;
    }

    // collision detect
    if (map[(int) newY * mapWidth + (int) newX] != '#') {
      playerX = newX;
      playerY = newY;
    }

    erase();
    // RAYCASTING
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
          hit = true;
          distanceToWall = MAX_DEPTH;
        } else if (map[testY * mapWidth + testX] == '#') {
          hit = true;
        }
      }

      int ceiling = (h / 2.0) - (h / distanceToWall);
      if (ceiling < 0) ceiling = 0;

      int floor = h - ceiling;

      // determite which color pair to draw wall with
      short pair;
      if      (distanceToWall <= MAX_DEPTH / 8.0) pair = 8;
      else if (distanceToWall <  MAX_DEPTH / 7.0) pair = 7;
      else if (distanceToWall <  MAX_DEPTH / 6.0) pair = 6;
      else if (distanceToWall <  MAX_DEPTH / 5.0) pair = 5;
      else if (distanceToWall <  MAX_DEPTH / 4.0) pair = 4;
      else if (distanceToWall <  MAX_DEPTH / 3.0) pair = 3;
      else if (distanceToWall <  MAX_DEPTH / 2.0) pair = 2;
      else if (distanceToWall <  MAX_DEPTH / 1.0) pair = 1;
      else                                        pair = 0;

      attron(COLOR_PAIR(pair));
      move(ceiling, col);
      vline(' ', floor - ceiling);
      attroff(COLOR_PAIR(pair));

      // draw the floor
      attron(COLOR_PAIR(0));
      for (int i = floor; i < h; i++) {
        float b = 1.0f - (i - h / 2.0f) / (h / 2.0f);
        move(i, col);
        if      (b < 0.25) addch('#');
        else if (b < 0.5)  addch('x');
        else if (b < 0.75) addch('!');
        else if (b < 0.9)  addch('-');
        else               addch(' ');
      }
      attroff(COLOR_PAIR(0));
    }

    attron(COLOR_PAIR(0));
    // print map
    for (int row = 0; row < mapHeight; row++) {
      move(1 + row, 0);
      printw("%.*s", mapWidth, map + (row * mapWidth));
    }

    // print character
    move(1 + (int) playerY, (int) playerX);
    addch('@');

    // print debug info
    move(0, 0);
    clrtoeol();
    printw("Angle: %.3f X: %f Y: %f Fps: %d Cols: %d, Rows: %d",
           playerA, playerX, playerY, -1, h, w);
    attroff(COLOR_PAIR(0));

    refresh();
  }

  // cleanup
  endwin();
  return 0;
}
