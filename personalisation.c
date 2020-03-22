#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>

#define BUFSIZE 64

#define USER_CACHE_LAUNCH "/home/user/.cache/launch"
#define DEFAULT_THEME "/usr/share/themes/default"
#define ETC_HILDON_THEME "/etc/hildon/theme"

int main(int argc, char **argv)
{
	int ret = EXIT_FAILURE;
	char *temp_file = NULL;
	char *temp_file_usr_share = NULL;
	char *theme_name;
	DIR *cachedir = NULL;
	struct dirent *dir;

	if (argc != 2) {
		fprintf(stderr, "usage: personalisation name_of_theme\n");
		return ret;
	}
	// TODO: Sanitize theme name to a single word?

	theme_name = argv[1];
	temp_file = strdup("/tmp/themeXXXXXX");

	if (mkstemp(temp_file) == -1) {
		fprintf(stderr, "Could not mkstemp: %s\n", strerror(errno));
		goto done;
	}
	if (unlink(temp_file)) {
		fprintf(stderr,
			"Could not remove just created temporary file\n");
		goto done;
	}

	if (mkdir("/etc/hildon", 0755)) {
		fprintf(stderr, "Could not create /etc/hildon\n");
		goto done;
	}

	if (symlink(DEFAULT_THEME, temp_file)) {
		fprintf(stderr, "Could not symlink temporary file: %s\n",
			strerror(errno));
		goto done;
	}

	if (rename(temp_file, "/etc/hildon/theme")) {
		fprintf(stderr, "Could not rename temporary file: %s\n",
			strerror(errno));
		goto done;
	}

	temp_file_usr_share = strdup("/tmp/themeXXXXXX");
	if (mkstemp(temp_file_usr_share) == -1) {
		fprintf(stderr, "Could not mkstemp: %s\n", strerror(errno));
		goto done;
	}
	if (unlink(temp_file_usr_share)) {
		fprintf(stderr,
			"Could not remove just created temporary file\n");
		goto done;
	}

	if (symlink(theme_name, temp_file_usr_share)) {
		fprintf(stderr, "Could not symlink temporary file: %s\n",
			strerror(errno));
		goto done;
	}

	if (rename(temp_file_usr_share, DEFAULT_THEME)) {
		fprintf(stderr, "Could not rename temporary file: %s\n",
			strerror(errno));
		goto done;
	}
	// We're OK once we change the symlinks
	ret = EXIT_SUCCESS;

	cachedir = opendir(USER_CACHE_LAUNCH);
	if (!cachedir) {
		fprintf(stderr, "Could not open/clear cache dir\n");
		goto done;
	}

	while ((dir = readdir(cachedir))) {
		const char *name = dir->d_name;
		if (strncmp(".", name, strlen(".")) == 0) {
			// Skip anything starting with '.'
			continue;
		}

		int chars_req = strlen(name) + strlen(USER_CACHE_LAUNCH) + 2;
		char *cache_launch_dir_entry = malloc(sizeof(char) * chars_req);
		if (!cache_launch_dir_entry) {
			fprintf(stderr,
				"Unable to allocate memory for path to remove call\n");
			goto done2;
		}
		snprintf(cache_launch_dir_entry, chars_req, "%s/%s",
			 "/home/user/.cache/launch", name);
		if (remove(cache_launch_dir_entry)) {
			fprintf(stderr, "Cannot remove %s: %s\n",
				cache_launch_dir_entry, strerror(errno));
		}
		free(cache_launch_dir_entry);

	}

 done2:
	closedir(cachedir);

 done:

	if (temp_file) {
		free(temp_file);

	}
	if (temp_file_usr_share) {
		free(temp_file_usr_share);
	}

	return ret;
}
