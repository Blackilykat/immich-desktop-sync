#pragma once

#include "state.h"
#include <curl/curl.h>
typedef struct tracked_directory {
	char* directory;
	char* album;
	directory_state state;
} tracked_directory;

typedef struct config {
	char* apiKey;
	char* immichURL;
	int trackedDirectoriesLen;
	tracked_directory* trackedDirectories;
	CURL* handle;
} config;

config readConfig();
