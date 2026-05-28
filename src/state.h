#pragma once

#define STATE_FILENAME ".immich-desktop-sync-state"
#define STATE_TEMP_FILENAME ".immich-desktop-sync-state.tmp"

typedef struct {
	long time;
	char* filename;
} tracked_file;

typedef struct {
	int count;
	tracked_file* data;
} directory_state;

directory_state readState(char* directory);
void writeState(char* directory, directory_state state);
