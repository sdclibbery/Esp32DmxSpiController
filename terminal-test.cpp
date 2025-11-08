#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>
#include <sys/time.h>
#include <unistd.h>
#include <termios.h>
#include <string.h>

#include "sketch/modes.h"
#include "sketch/palettes.h"

struct termios orig_termios;
void disable_non_blocking_input() {
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios);
}
void enable_non_blocking_input() {
    struct termios new_termios;
    tcgetattr(STDIN_FILENO, &orig_termios);
    atexit(disable_non_blocking_input);
    new_termios = orig_termios;
    new_termios.c_lflag &= ~(ICANON | ECHO);
    new_termios.c_cc[VMIN] = 0;
    new_termios.c_cc[VTIME] = 0;
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &new_termios);
}

uint16_t pixelCount1 = 32;
void setPixel1 (uint16_t index, Rgb color) {
    printf("\x1b[%d;%dH", 1, index*2);
    printf("\x1b[38;2;%d;%d;%dm\u2588\u2588\x1b[0m", (uint8_t)(color.red*255.0f), (uint8_t)(color.green*255.0f), (uint8_t)(color.blue*255.0f));
}
PixelStrip pixelStrip1(pixelCount1, setPixel1);
Controls controls1(Rgb(0.0f,0.0f,0.6f),Rgb(1.0f,1.0f,1.0f));

void parseInput (Controls& controls, char* data) { // For testing
  if (data[0] == 'm') { controls.mode = atoi(&data[1]); }
  if (data[0] == 'p') { controls.palette = atoi(&data[1]); }
  if (data[0] == 'c') { controls.control = ((float)atoi(&data[1]))/255; }
  if (data[0] == 's') { controls.smooth = ((float)atoi(&data[1]))/255; }
  if (data[0] == 'r') { controls.back.red = ((float)atoi(&data[1]))/255; }
  if (data[0] == 'g') { controls.back.green = ((float)atoi(&data[1]))/255; }
  if (data[0] == 'b') { controls.back.blue = ((float)atoi(&data[1]))/255; }
  if (data[0] == 'R') { controls.fore.red = ((float)atoi(&data[1]))/255; }
  if (data[0] == 'G') { controls.fore.green = ((float)atoi(&data[1]))/255; }
  if (data[0] == 'B') { controls.fore.blue = ((float)atoi(&data[1]))/255; }
}

int main () {
  unsigned long timeMs = 0;
  unsigned int frameIntervalMs = 10;
  unsigned int numPixels = 32;
  char line[100];
  timeval timeval;
  char input_buffer[256] = {0};
  unsigned int input_index = 0;
  char key;
  int running = 1;
  enable_non_blocking_input();
  printf("\x1b[2J\x1b[H");
  while (running) {
    fflush(stdout);
    fflush(stdin);
    int bytes_read = read(STDIN_FILENO, &key, 1);
    if (bytes_read > 0) {
      input_buffer[input_index] = key;
      input_index++;
      printf("\x1b[%d;%dH", 2, input_index);
      printf("%c", key);
      if (key == '\n' || key == '\r') {
        input_buffer[input_index - 1] = '\0';
        parseInput(controls1, input_buffer);
        input_index = 0;
        memset(input_buffer, 0, sizeof(input_buffer));
        printf("\x1b[%d;%dH", 2, 0);
        printf("         ");
      }
      if (key == 'q' || key == 'Q') { running = 0; }
    }
    gettimeofday(&timeval, NULL);
    updateStrip(controls1, pixelStrip1, timeval.tv_usec);
    usleep(10000);
  }
  return 0;
}