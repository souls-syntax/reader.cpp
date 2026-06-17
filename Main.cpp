// ====================
// The purpose of this project is to
// make a reader so that I can read
// novels even faster than my already
// amazing speed.(200k words/day) to (300k words/day)
//
// Rsearch objective:
// Terminal: RSVP (Rapid Serial Visual Presentation)
//
// Project Name: fastAsFKReader(FAFK)
// Author : Aakarsh Kashyap
// [ 2026-06-17 ]
// ====================

// Steps
//
// 1. Get to the raw mode
// 2. Get cursor to center of the terminal
// 3. Parse the whole file into the memory and save it into a vector
// of strings.
// 4. Figure out the central letter of the string
// 5. Print that word with the central letter coloured
// 6. We will take a timer args which will make word rendering fast or slow but
// it's fine for now
// 7. The size of the word is smt dependent on user terminal zoomin we will not
// try to render bigger font or anything
// 8. End of the program.

/* Include */
#include <asm-generic/ioctls.h>
#include <cerrno>
#include <cstdint>
#include <termios.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <cstring>
#include <string>
#include <vector>

/* Define */
#define CTRL_KEY(k) ((k) & 0x1f)


/** data **/

typedef struct editorConfig
{
    int screenrows;
    int screencols;
    struct termios org_termios{};

} editorConfig_t;

editorConfig_t E;

/** terminal **/
void die(const char *s)
{
    write(STDOUT_FILENO, "\x1b[2J", 4);
    write(STDOUT_FILENO, "\x1b[H", 3);

    perror(s);
    exit(1);
}

void disableRawMode()
{
    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &E.org_termios) == -1) die("tcsetattr") ;
}

void enableRawMode()
{
    if(tcgetattr(STDIN_FILENO, &E.org_termios) == -1) die("tcgetattr");
    // At exit(program exits) runs the function
    atexit(disableRawMode);

    struct termios raw = E.org_termios;


    // Iflags are for input flags
    // XON is used to control and disable C-s and C-q they are used for software flow control
    // CR for carriage return NL for new line
    raw.c_iflag &= ~(IXON | ICRNL | BRKINT | INPCK | ISTRIP);

    // turning off the post processing of output
    raw.c_oflag &= ~(OPOST);

    raw.c_cflag |= (CS8);
    // lflags is for mislaneous things
    // ISIG flag disable SIGINT C-c and SIGSTP C-z
    // ECHO flag disable return output to screen as we type sudo uses this
    // ICANON is used to turn off the canonical/cooked mode so we can process per byte instead of per line
    // IEXTEN is used to diable C-v
    raw.c_lflag &= ~(ECHO | ICANON | IEXTEN |ISIG);


    // c_cc is a control character array;
    // vmin sets the number of bytes needed before read can return
    raw.c_cc[VMIN] = 0;
    // Sets max time before read returns
    raw.c_cc[VTIME] = 1;

    // TCSAFLUSH is used to flus all the things after thsi func call
    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) == -1) die("tcsetattr") ;
}

char editorReadKey()
{
    int nread;
    char c;
    while ((nread = read(STDIN_FILENO, &c, 1)) != 1)
    {
        if(nread == -1 && errno != EAGAIN) die("read");
    }
    return c;
}

int getCursorPosition(int *rows, int *cols)
{
    char buf[32];
    unsigned int i = 0;
    // n cmd gives the device info 6 as args give us the cursor pos
  if (write(STDOUT_FILENO, "\x1b[6n", 4) != 4)
      return -1;
  while (i < sizeof(buf) - 1)
  {
    if (read(STDIN_FILENO, &buf[i], 1) != 1)
        break;
    if (buf[i] == 'R')
        break;
    i++;
  }
  buf[i] = '\0';

  if (buf[0] != '\x1b' || buf[1] != '[')
      return -1;
  if (sscanf(&buf[2], "%d;%d", rows, cols) != 2)
      return -1;

  return 0;
}


int getWindowSize(int *rows, int *cols)
{
    struct winsize ws;

    // TIOCGWINSZ stads for terminal ioctl get window size
    // returns -1 on error can even return row and cols as zero so we check both
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == -1 || ws.ws_col == 0)
    {
        if (write(STDOUT_FILENO, "\x1b[999C\x1b[999B", 12) != 12)
            return -1;
        return getCursorPosition(rows, cols);
    }
    else
    {
        *cols = ws.ws_col;
        *rows = ws.ws_row;
        return 0;
    }
}

/* append buffer */

class AppendBuf
{
private:
    char *m_b;
    uint32_t m_len;

public:
    AppendBuf() : m_b(nullptr), m_len(0) {}

    ~AppendBuf() { }

    void append(const char *s, int len)
    {
        char *num = (char*)realloc(m_b, m_len + len);

        if (num == nullptr)
            return;
        memcpy(&num[m_len], s, len);
        m_b = num;
        m_len += len;
    }

    static AppendBuf &instance()
    {
        static AppendBuf obj;
        return obj;
    }
    void remove()
    {
        free(m_b);
        m_b = nullptr;
        m_len = 0;
    }
    void write_out()
    {
        write(STDOUT_FILENO, m_b, m_len);
    }
};

/* output */


void editorRefreshScreen()
{
  // escape sequence starts with escape character \x1b is 27byte <esc> followed by [
  // is a escape sequence
  // Then we specify the command and it's args. args are given before the
  // command
  // J corresponds to clearning the screen. 2 args means all the screena 1 for
  // before the cursor and 0(default)
  // for after the cursor to EOS
    AppendBuf::instance().append("\x1b[2J", 4);
    // H for cursor pos control takes two args rowXcol. 12;4H args are seperated by ';'
    char tuff[32]; memset(tuff, 0, sizeof(tuff));
    snprintf(tuff, sizeof(tuff), "\x1b[%d;%dH", E.screenrows / 2, E.screencols / 2);
    AppendBuf::instance().append(tuff, strlen(tuff));
    AppendBuf::instance().write_out();
    AppendBuf::instance().remove();
}

/* input */

void editorProcessKeyProcess()
{
    char c = editorReadKey();

    switch (c)
    {
    case CTRL_KEY('q'):
        write(STDOUT_FILENO, "\x1b[2J", 4);
        write(STDOUT_FILENO, "\x1b[H", 3);
        exit(0);
        break;
    }
}

/* Utils */
size_t split(const std::string &txt, std::vector<std::string> &strs, char ch) {
  size_t pos = txt.find( ch );
  size_t initaiPos = 0;
  // strs.clear();

  while( pos != std::string::npos) {
    strs.push_back(txt.substr(initaiPos, pos - initaiPos));
    initaiPos = pos+1;

    pos = txt.find(ch, initaiPos);
  }
  strs.push_back( txt.substr(initaiPos));

  return strs.size();
}

/** Init  **/

void initEditor()
{
  if (getWindowSize(&E.screenrows, &E.screencols) == -1)
      die("getWindowSize");

}

int main(int argc, char *argv[])
{
    enableRawMode();
    initEditor();
    if (argc < 2) die("no file provided");

    FILE *f = fopen(argv[1], "r");
    if (!f) die("fopen");

    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    rewind(f);

    std::string content(size, '\0');
    fread(&content[0], 1, size, f);
    fclose(f);
    std::vector<std::string> lund;
    split(content, lund, ' ');

    while (1)
    {
        for (std::string &k : lund)
        {
            editorRefreshScreen();
            int mid = k.size() / 2;

            write(STDOUT_FILENO, k.substr(0, mid).c_str(), mid);

            write(STDOUT_FILENO, "\x1b[31m", 5);
            write(STDOUT_FILENO, k.substr(mid, 1).c_str(), 1);
            write(STDOUT_FILENO, "\x1b[m", 3);

            int remaining = k.size() - mid - 1;
            write(STDOUT_FILENO, k.substr(mid + 1, remaining).c_str(), remaining);

            usleep(250000);
        }
        editorProcessKeyProcess();
    }
    return 0;
}

/* CONCLUSION */
// Experiment have failed. I do not see any particular increase int my
// reading speed, infact it has become difficult to
// keep previous context in mind
// There must be something else going on in that reel
// I will try to ask about it from sei or shashy maybe there is a
// better way
//
// [ 2026-06-17 ]
