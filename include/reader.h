/* reader.h */

#define CTRL_KEY(k) ((k) & 0x1f)

#include <asm-generic/ioctls.h>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>
#include <vector>
#include <cstdlib>


using editor_cfg = struct editor_cfg {
	int screenrows;
	int screencols;
	termios termios_inst {};
};

class append_buffer {
	private:
		char *m_b{nullptr};
		uint32_t m_len{0};
	public:
		append_buffer()	{}
		~append_buffer() = default;

		void append(const char *s, int len) {
			char *n = (char *)realloc(m_b, m_len + len);

			if (n == nullptr)
				return;
			memcpy(&n[m_len], s, len);
			m_b = n;
			m_len += len;
		}

		static append_buffer &instance() {
			static append_buffer obj;
			return obj;
		}

		void remove() {
			free(m_b);
			m_b = nullptr;
			m_len = 0;
		}

		void write_output() {
			write(STDOUT_FILENO, m_b, m_len);
		}
};

void init_editor(editor_cfg *cfg);
void die(const char *s);
void disable_raw(editor_cfg *cfg);
void enable_raw(editor_cfg *cfg);
void editor_refresh_scrn(editor_cfg *cfg);

char editor_readkey();

int get_cursor_pos(int *rows, int *cols);
int get_window_size(int *rows, int *cols);
int is_number(char *strnum);

size_t split(const std::string &txt, std::vector<std::string> &strings, const std::string &delims);


