#include <termio.h>
#include <unistd.h>


struct termios old_tty;

void set_terminal_mode() {
    struct termios tty;
    tcgetattr(STDIN_FILENO, &old_tty);
    tty = old_tty;
    tty.c_lflag &= ICANON;
    tty.c_lflag &= ~(ECHO | ECHONL | ICANON | ISIG | IEXTEN);
    tty.c_cc[VMIN] = 1;
    tcsetattr(STDOUT_FILENO, TCSAFLUSH, &tty);
}

void reset_terminal_mode() {
    tcsetattr(STDOUT_FILENO, TCSAFLUSH, &old_tty);
}