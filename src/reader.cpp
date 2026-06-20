#include "reader.h"
#include <csignal>

void die(const char *s) {
	write(STDOUT_FILENO, "\x1b[2J", 4);
	write(STDOUT_FILENO, "\x1b[H", 3);

	perror(s);
	fprintf(stderr, "[exit]: die(s) invoked, exiting..\n");
	exit(1);
}

static editor_cfg *g_cfg = nullptr;

void disable_raw_atexit() {
	if (g_cfg) {
		tcsetattr(STDIN_FILENO, TCSAFLUSH, &(g_cfg->termios_inst));
	}
}

void handle_signal(int sig) {
	disable_raw_atexit();
	_exit(128 + sig);
}

void disable_raw(editor_cfg *cfg) {
	if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &(cfg->termios_inst)))
		die("tcsetattr");
}

void enable_raw(editor_cfg *cfg) {
	if (tcgetattr(STDIN_FILENO, &(cfg->termios_inst)) == -1)
		die("tcgetattr");

	g_cfg = cfg;

	std::atexit(disable_raw_atexit);

	std::signal(SIGINT, handle_signal);
	std::signal(SIGTERM, handle_signal);
	std::signal(SIGQUIT, handle_signal);

	struct termios raw = cfg->termios_inst;
	raw.c_iflag &= ~(IXON | ICRNL | BRKINT | INPCK | ISTRIP);
	raw.c_oflag &= ~(OPOST);
	raw.c_lflag &=
		~(ECHO | ICANON | IEXTEN); // disables several keybinds in terminal

	raw.c_cc[VMIN] = 0;
	raw.c_cc[VTIME] = 1;

	if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) == -1)
		die("tcsetattr");
}

char editor_readkey() {
	int nread;
	char c;

	while ((nread = read(STDIN_FILENO, &c, 1)) != 1) {
		if (nread == -1 && errno != EAGAIN)
			die("read");
	}
	return c;
}

int get_cursor_pos(int *rows, int *cols) {
	char buff[32];
	unsigned int i = 0;

	if (write(STDOUT_FILENO, "\x1b[6n", 4) != 4) {
		return -1;
	}
	while (i < sizeof(buff) - 1) {
		if (read(STDIN_FILENO, &buff[i], 1) != 1)
			break;
		if (buff[i] == 'R')
			break;
		i++;
	}
	buff[i] = '\0';

	if (buff[0] != '\x1b' || buff[1] != '[')
		return -1;
	if (sscanf(&buff[2], "%d;%d", rows, cols) != 2)
		return -1;
	return 0;
}

int get_window_size(int *rows, int *cols) {
	struct winsize w_size;

	if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &w_size) == -1 ||
			w_size.ws_col == 0) {
		if (write(STDOUT_FILENO, "\x1b[999C\x1b[999B", 12) != 12)
			return -1;
		return get_cursor_pos(rows, cols);
	}

	*rows = w_size.ws_row;
	*cols = w_size.ws_col;

	return 0;
}

void editor_refresh_scrn(editor_cfg *cfg) {
	append_buffer::instance().append("\x1b[2J", 4);
	char buff[32];
	memset(buff, 0, sizeof(buff));

	snprintf(buff, sizeof(buff), "\x1b[%d;%dH", cfg->screenrows / 2, cfg->screencols / 2);

	append_buffer::instance().append(buff, strlen(buff));
	append_buffer::instance().write_output();
	append_buffer::instance().remove();
}

size_t split(const std::string &txt, std::vector<std::string> &strings, const std::string &delims) {
	size_t initial_pos = txt.find_first_not_of(delims, 0);
	size_t pos = txt.find_first_of(delims, initial_pos);

	while (initial_pos != std::string::npos) {
		strings.push_back(txt.substr(initial_pos, pos - initial_pos));
		initial_pos = txt.find_first_not_of(delims, pos);
		pos = txt.find_first_of(delims, initial_pos);
	}
	return strings.size();
}

void init_editor(editor_cfg *cfg) {
	if (get_window_size(&(cfg->screenrows), &(cfg->screencols)) == -1)
		die("get_window_size");
}

int is_number(char *strnum) {
	std::string str = strnum;
	std::string::const_iterator it = str.begin();
	while (it != str.end() && std::isdigit(*it)) ++it;
	return !str.empty() && it == str.end();
}
