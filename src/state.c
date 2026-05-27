#include "state.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
/*
For each tracked directory, the program stores a file called `.immich-desktop-sync-state`.
Storage is structured as a sequence of files. A file is composed as follows:
- 8 bytes for the modification date
- A null-terminated string containing the filename.

NON RECURSIVE
*/

#define STATE_FILENAME ".immich-desktop-sync-state"
#define STATE_TEMP_FILENAME ".immich-desktop-sync-state.tmp"


directory_state readState(char* directory) {
	directory_state state = {};

	char* filename = malloc(strlen(directory) + strlen(STATE_FILENAME) + 1);
	strcpy(filename, directory);
	strcat(filename, STATE_FILENAME);

	FILE* file = fopen(filename, "rb");
	free(filename);

	if(file == NULL) {
		return state;
	}

	while(1) {
		long timestamp;
		int res = fread(&timestamp, 8, 1, file);
		if(res < 1) {
			break;
		}

		char filename[FILENAME_MAX];

		int c;
		for(int i = 0; i < FILENAME_MAX; i++) {
			c = fgetc(file);
			if(c <= 0 || c == EOF) {
				filename[i] = 0;
				break;
			}
			filename[i] = c;
		}
		if(c == EOF) {
			break;
		}

		char* allocatedFilename = malloc(strlen(filename) + 1);
		strcpy(allocatedFilename, filename);

		state.count++;
		state.data = realloc(state.data, sizeof(tracked_file) * state.count);
		state.data[state.count - 1] = (tracked_file) {
			.time = timestamp,
			.filename = allocatedFilename
		};
	}

	return state;
}

void writeState(char* directory, directory_state state) {
	char* filename = malloc(strlen(directory) + strlen(STATE_TEMP_FILENAME) + 1);
	strcpy(filename, directory);
	strcat(filename, STATE_TEMP_FILENAME);

	FILE* file = fopen(filename, "wb");

	if(file == NULL) {
		fprintf(stderr, "Failed to write state file: %s\n", filename);
		return;
	}

	for(int i = 0; i < state.count; i++) {
		tracked_file trackedFile = state.data[i];

		if(fwrite(&trackedFile.time, 8, 1, file) < 1) {
			fprintf(stderr, "Failed to write timestamp in state file: %s\n", filename);
			fclose(file);
			remove(filename);
			free(filename);
			return;
		}

		int filenameBytes = strlen(trackedFile.filename) + 1;
		if(fwrite(trackedFile.filename, 1, filenameBytes, file) < filenameBytes) {
			fprintf(stderr, "Failed to write filename in state file: %s\n", filename);
			fclose(file);
			remove(filename);
			free(filename);
			return;
		}
	}

	fsync(fileno(file));
	fclose(file);

	char* finalFilename = malloc(strlen(directory) + strlen(STATE_FILENAME) + 1);
	strcpy(finalFilename, directory);
	strcat(finalFilename, STATE_FILENAME);

	remove(finalFilename);
	rename(filename, finalFilename);

	free(filename);
}

