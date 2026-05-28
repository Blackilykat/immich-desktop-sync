#include <bits/posix2_lim.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "config.h"

#define API_KEY_EXPECTED_LENGTH 43

#define CONFIG_TEMPLATE "\
# Remove this line once you're done setting all values.\
template=true\
\
# An immich API key.\
# Get one in Account Settings > API Keys > New API Key.\
# immich-desktop-sync requires (TODO).\
apiKey=changeMe\
\
# The URL base of your immich server.\
# This is the start of the URL you use to open Immich from your browser.\
# If the \"home\" URL is http://192.168.1.240:2283/photos, the URL base is http://192.168.1.240:2283.\
url=http://localhost\
\
# Lines starting with / or ~ are directories to track.\
# On the left, the location on your filesystem.\
# On the right, the destination album ID.\
# Multiple directories are allowed to point to the same album.\
~/Pictures=5e98144a-357d-4289-af57-f4fe0a2063ca\
\
/example=dddfbedb-aeca-4c6d-8336-ece5471bcce6\
"

void configError(int lineNumber, char* message) {
	fprintf(stderr, "There is an error at line %d of your config.\n%s\n", lineNumber, message);
	exit(1);
}

char* makeAbsolutePath(char* relativePath) {
	char* home = getenv("HOME");

	bool shouldAddTrailingSlash = relativePath[strlen(relativePath) - 1] != '/';

	char* dest = malloc(strlen(relativePath) + strlen(home) + (shouldAddTrailingSlash ? 1 : 0)); // + 1 (\0) - 1 (~) + 0-1 (trailing /)

	strcpy(dest, home);
	strcat(dest, relativePath + 1);

	int destLen = strlen(dest);

	if(shouldAddTrailingSlash) {
		dest[destLen] = '/';
		dest[destLen + 1] = 0;
	}

	// don't free the existing pointer because it is a substring of a previously allocated area.
	return dest;
}

void checkCorrectAlbumId(int lineNumber, char* albumId) {
	int length = strlen(albumId);
	if(length != 36) {
		configError(lineNumber, "Album IDs should be 36 characters long, in the format \"xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx\".");
	}

	for(int i = 0; i < length; i++) {
		char c = albumId[i];

		// these characters should be dashes
		if(i == 8 || i == 13 || i == 18 || i == 23) {
			if(c != '-') {
				configError(lineNumber, "Album IDs should be in the format \"xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx\".");
			}
			continue;
		}

		if((c < 'a' || c > 'f') && (c < '0' || c > '9')) {
			configError(lineNumber, "Album IDs should only contain characters [a-f] and [0-9].");
		}
	}
}

config readConfig() {
	char path[PATH_MAX];

	snprintf(path, PATH_MAX, "%s/.config/immich-desktop-sync.conf", getenv("HOME"));

	FILE* file = fopen(path, "r");

	if(file == NULL) {
		FILE* file = fopen(path, "w");
		if(file == NULL) {
			fprintf(stderr, "No config file found, could not create default.\n");
			fprintf(stderr, "Create a config file at ~/.config/immich-desktop-sync.conf\n");
			exit(1);
		}

		fprintf(file, CONFIG_TEMPLATE);

		fclose(file);

		fprintf(stderr, "No config file found, created template.\n");
		fprintf(stderr, "Fill in your config at ~/.config/immich-desktop-sync.conf\n");
		exit(1);
	}

	config conf = {};

	char* line = NULL;
	size_t lineSize = 0;
	int lineLength = 0;
	int lineCount = 0;

	while(getline(&line, &lineSize, file) != -1) {
		lineCount++;
		lineLength = strlen(line);

		if(line[lineLength - 1] == '\n') {
			line[lineLength - 1] = 0;
			lineLength--;
		}

		if(lineLength == 0) continue;
		if(line[0] == '#') continue;

		char* key = strtok(line, "=");
		char* value = strtok(NULL, "=");

		if(value == NULL || key == NULL || strtok(NULL, "=") != NULL) {
			configError(lineCount, "All non-comment lines must contain one key and one value, separated by an equal sign.");
		}

		if(strcmp(key, "template") == 0) {
			fprintf(stderr, "Your config seems to be a template. Fill in your config at ~/.config/immich-desktop-sync.conf\n");
			exit(1);
		}

		// valid keys and values

		if(strcmp(key, "apiKey") == 0) {
			int keyLength = strlen(value);
			if(keyLength != API_KEY_EXPECTED_LENGTH) {
				// May be because of a dumb assumption, but probably because of bad copy paste
				printf("WARN: Expected %d character long API key, found %d characters\n", API_KEY_EXPECTED_LENGTH, keyLength);
			}
			conf.apiKey = malloc(keyLength + 1);
			strcpy(conf.apiKey, value);
			continue;
		}

		if(strcmp(key, "immichURL") == 0) {
			int length = strlen(value);

			if(
				strncmp("http://", value, strlen("http://")) != 0
				&& strncmp("https://", value, strlen("https://")) != 0
			) {
				configError(lineCount, "Immich URL should start with either \"http://\" or \"https://\".");
			}

			conf.immichURL = malloc(length + 1);
			strcpy(conf.immichURL, value);
			continue;
		}

		if(key[0] == '/') {
			checkCorrectAlbumId(lineCount, value);

			bool shouldAddTrailingSlash = key[strlen(key) - 1] != '/';
			char* allocatedKey = malloc(strlen(key) + 1 + (shouldAddTrailingSlash ? 1 : 0));
			strcpy(allocatedKey, key);

			if(shouldAddTrailingSlash) {
				strcat(allocatedKey, "/");
			}

			char* allocatedValue = malloc(strlen(value) + 1);
			strcpy(allocatedValue, value);

			tracked_directory dir = {
				.directory = allocatedKey,
				.album = allocatedValue
			};

			conf.trackedDirectoriesLen++;
			conf.trackedDirectories = realloc(conf.trackedDirectories, sizeof(tracked_directory) * conf.trackedDirectoriesLen);

			conf.trackedDirectories[conf.trackedDirectoriesLen - 1] = dir;

			continue;
		}

		if(key[0] == '~') {
			if(key[1] != '/') {
				configError(lineCount, "Illegal home path without \"/\" after \"~\".");
			}

			checkCorrectAlbumId(lineCount, value);

			char* allocatedKey = makeAbsolutePath(key);
			char* allocatedValue = malloc(strlen(value) + 1);
			strcpy(allocatedValue, value);

			tracked_directory dir = {
				.directory = allocatedKey,
				.album = allocatedValue
			};

			conf.trackedDirectoriesLen++;
			conf.trackedDirectories = realloc(conf.trackedDirectories, sizeof(tracked_directory) * conf.trackedDirectoriesLen);

			conf.trackedDirectories[conf.trackedDirectoriesLen - 1] = dir;

			continue;
		}

		char* template = "Unknown config key \"%s\".";

		char* msg = malloc(strlen(template) - 2 + strlen(key));

		sprintf(msg, template, key);

		configError(lineCount, msg);
		// exit is called, memory is freed upon process termination
	}

	free(line);

	return conf;
}
