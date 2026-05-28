#include "config.h"
#include "state.h"
#include "json.h"
#include <bits/posix1_lim.h>
#include <curl/easy.h>
#include <dirent.h>
#include <stddef.h>
#include <stdio.h>
#include <curl/curl.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <stdbool.h>
#include <time.h>
#include <unistd.h>

#define ASSETS_ENDPOINT "/api/assets"

bool uploadToImmich(char* filename, char* absolutePath, long modificationDate, char* album, config* config);
void updateTrackedDirectory(tracked_directory* trackedDirectory, config* config);

int main() {
	printf("omg hi\n");

	config conf = readConfig();

	// TODO: maybe dont print api key lol
	printf("Got config with api key %s and url %s.\n", conf.apiKey, conf.immichURL);

	curl_global_init(CURL_GLOBAL_ALL);

	conf.handle = curl_easy_init();

	for(int i = 0; i < conf.trackedDirectoriesLen; i++) {
		printf("%s -> %s\n", conf.trackedDirectories[i].directory, conf.trackedDirectories[i].album);

		conf.trackedDirectories[i].state = readState(conf.trackedDirectories[i].directory);

		updateTrackedDirectory(&conf.trackedDirectories[i], &conf);
	}
}

void updateTrackedDirectory(tracked_directory* trackedDirectory, config* config) {
	char* directory = trackedDirectory->directory;

	int directoryLength = strlen(directory);
	DIR* dir = opendir(directory);

	struct dirent* entry;

	char fullFilename[FILENAME_MAX];
	strcpy(fullFilename, directory);

	while((entry = readdir(dir)) != NULL) {
		// TODO: check whether unknown must be supported. FAT32 filesystems should be supported but idk what they return for this.
		//       see: man readdir
		if(entry->d_type != DT_REG && entry->d_type != DT_LNK && entry->d_type != DT_UNKNOWN) continue;

		fullFilename[directoryLength] = 0;
		strcat(fullFilename, entry->d_name);

		struct stat attr;
		stat(fullFilename, &attr);

		long modificationDate = attr.st_mtim.tv_sec;

		bool found = false;
		for(int i = 0; i < trackedDirectory->state.count; i++) {
			tracked_file* trackedFile = &trackedDirectory->state.data[i];
			if(strcmp(entry->d_name, trackedFile->filename) != 0) continue;

			found = true;

			if(modificationDate != trackedFile->time) {
				// file was updated, reupload it to Immich
				if(uploadToImmich(entry->d_name, fullFilename, modificationDate, trackedDirectory->album, config)) {
					trackedFile->time = modificationDate;
				}
			}

			break;
		}

		if(!found) {
			// File was added, upload it to immich
			if(uploadToImmich(entry->d_name, fullFilename, modificationDate, trackedDirectory->album, config)) {
				trackedDirectory->state.count++;
				trackedDirectory->state.data = realloc(trackedDirectory->state.data, sizeof(tracked_file) * trackedDirectory->state.count);
				trackedDirectory->state.data[trackedDirectory->state.count - 1] = (tracked_file) {
					.filename = entry->d_name,
					.time = modificationDate
				};
			}
		}
	}

	writeState(directory, trackedDirectory->state);
}

static size_t curl_write_to_newly_allocated_string(char* contents, size_t size, size_t nmemb, void* vdest) {
	char** dest = (char**) vdest;
	*dest = malloc(strlen(contents) + 1);
	strcpy(*dest, contents);
	return size * nmemb;
}


bool uploadToImmich(char* filename, char* absolutePath, long modificationDate, char* album, config* config) {
	curl_easy_reset(config->handle);

	printf("Uploading file %s to album %s...\n", absolutePath, album);

	char* url = malloc(strlen(config->immichURL) + strlen(ASSETS_ENDPOINT) + 1);
	strcpy(url, config->immichURL);
	strcat(url, ASSETS_ENDPOINT);

	char* header = malloc(strlen("x-api-key: ") + strlen(config->apiKey) + 1);
	strcpy(header, "x-api-key: ");
	strcat(header, config->apiKey);

	struct curl_slist* headers = curl_slist_append(NULL, header);

	curl_mime* multipart = curl_mime_init(config->handle);

	curl_mimepart* part = curl_mime_addpart(multipart);
	curl_mime_name(part, "deviceAssetId");
	curl_mime_data(part, absolutePath, CURL_ZERO_TERMINATED);

	part = curl_mime_addpart(multipart);
	curl_mime_name(part, "filename");
	curl_mime_data(part, filename, CURL_ZERO_TERMINATED);

	char hostname[HOST_NAME_MAX + 1];
	gethostname(hostname, HOST_NAME_MAX);
	// "The result is null-terminated if LEN is large enough for the full name and the terminator."
	hostname[HOST_NAME_MAX] = 0;


	part = curl_mime_addpart(multipart);
	curl_mime_name(part, "deviceId");
	curl_mime_data(part, hostname, CURL_ZERO_TERMINATED);

	// Set both the creation and modification date to the file's modification date.
	// This is because file modifications get treated as newly uploaded files by this program.

	char time[25];

	// statically allocated struct
	struct tm* time_data = gmtime(&modificationDate);

	// TODO: fix warnings here
	snprintf(time, 25, "%04d-%02d-%02dT%02d:%02d:%02d.000Z",
	  time_data->tm_year + 1900,
	  time_data->tm_mon,
	  time_data->tm_mday,
	  time_data->tm_hour,
	  time_data->tm_min,
	  time_data->tm_sec);

	part = curl_mime_addpart(multipart);
	curl_mime_name(part, "fileCreatedAt");
	curl_mime_data(part, time, CURL_ZERO_TERMINATED);

	part = curl_mime_addpart(multipart);
	curl_mime_name(part, "fileModifiedAt");
	curl_mime_data(part, time, CURL_ZERO_TERMINATED);

	part = curl_mime_addpart(multipart);
	curl_mime_name(part, "assetData");
	curl_mime_filedata(part, absolutePath);

	char* responseBody;

	curl_easy_setopt(config->handle, CURLOPT_URL, url);
	curl_easy_setopt(config->handle, CURLOPT_HTTPHEADER, headers);
	curl_easy_setopt(config->handle, CURLOPT_MIMEPOST, multipart);
	curl_easy_setopt(config->handle, CURLOPT_WRITEFUNCTION, curl_write_to_newly_allocated_string);
	curl_easy_setopt(config->handle, CURLOPT_WRITEDATA, &responseBody);

	CURLcode response = curl_easy_perform(config->handle);

	long responseCode;
	curl_easy_getinfo(config->handle, CURLINFO_RESPONSE_CODE, &responseCode);

	if(response != CURLE_OK) {
		printf("oh fuck %s\n", curl_easy_strerror(response));
		if(responseBody != NULL) free(responseBody);
		return false;
	}

	if(responseCode < 200 || responseCode > 299) {
		if(responseBody != NULL) free(responseBody);
		return false;
	}

	char* id = extractJsonRootValueFrom("id", responseBody);

	free(responseBody);

	if(id == NULL) {
		printf("Could not get ID of uploaded media :(\n");
		return false;
	}
	
	curl_mime_free(multipart);

	// - Add the file to the album - //

	char body[49]; // 12 bytes of json + 36 bytes of uuid + 1 terminating byte

	snprintf(body, 49, "{\"ids\":[\"%s\"]}", id);

	headers = curl_slist_append(headers, "Content-Type: application/json");

	free(url);
	url = malloc(strlen(config->immichURL) + strlen("/api/albums") + 36 + strlen("/assets") + 1);
	strcpy(url, config->immichURL);
	strcat(url, "/api/albums/");
	strcat(url, album);
	strcat(url, "/assets");

	curl_easy_setopt(config->handle, CURLOPT_URL, url);
	curl_easy_setopt(config->handle, CURLOPT_HTTPHEADER, headers);
	curl_easy_setopt(config->handle, CURLOPT_POSTFIELDS, body);
	curl_easy_setopt(config->handle, CURLOPT_CUSTOMREQUEST, "PUT");

	response = curl_easy_perform(config->handle);

	curl_easy_getinfo(config->handle, CURLINFO_RESPONSE_CODE, &responseCode);

	if(response != CURLE_OK) {
		printf("oh fuck v2 %s\n", curl_easy_strerror(response));
		if(responseBody != NULL) free(responseBody);
		return false;
	}

	if(responseCode < 200 || responseCode > 299) {
		if(responseBody != NULL) free(responseBody);
		return false;
	}

	free(responseBody);
	curl_slist_free_all(headers);
	free(header);
	free(url);
	return true;
}
