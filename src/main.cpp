#include "reader.h"

int main(int argc, char *argv[]) {
	try {
		editor_cfg *cfg = (editor_cfg *)malloc(sizeof(editor_cfg));
		if (cfg == nullptr) {
			die("[err]: malloc failed\n");
		}

		enable_raw(cfg);
		init_editor(cfg);

		int speed { 250000 };

		if (argc < 2)
			die("[err]: no file provided!\n");

		if (argc > 3)
			die("[err]: too many arguments!\n");

		if (argc == 3) {
			if (!is_number(argv[2]))
				die("[err]: invalid speed interval provided!\n");
			speed = std::stoi(argv[2]);
		}

		FILE *fp = fopen(argv[1], "r");

		if (fp == nullptr)
			die("[err]: fopen failed\n");

		fseek(fp, 0, SEEK_END);
		long size = ftell(fp);
		rewind(fp);

		std::string content(size, '\0');
		fread(content.data(), 1, size, fp);

		fclose(fp);

		std::vector<std::string> data;
		split(content, data, " \t\r\n,;:!?");

		while(1) {
			for (std::string &k: data) {
				if (k.empty()) continue;
				editor_refresh_scrn(cfg);
				int mid = k.size() / 2;

				write(STDOUT_FILENO, k.substr(0, mid).c_str(), mid);
				write(STDOUT_FILENO, "\x1b[31m", 5);
				write(STDOUT_FILENO, k.substr(mid, 1).c_str(), 1);
				write(STDOUT_FILENO, "\x1b[m", 3);

				int remaining = k.size() - mid - 1;

				write(STDOUT_FILENO, k.substr(mid + 1, remaining).c_str(), remaining);
				usleep(speed);
			}
		}

		free(cfg);

	} catch (const std::exception &e) {
		die(e.what());
	} catch (...) {
		die("[err]: Unknown exception occurred\n");
	}

	return 0;
} /* main.cpp */
