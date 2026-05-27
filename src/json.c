/**
* Extremely basic JSON parser with only the bare minimum features implemented to work with Immich's API.
* Not even close to a proper JSON parser but I don't need one
*/

#define MAX_STRING_LEN 128

#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include "json.h"

char escapedChar(char sequence) {
	switch(sequence) {
		case 't': return '\t';
		case 'n': return '\n';
		case '"': return '"';
		case '\'': return '\'';
		case '\\': return '\\';
	}
	return sequence;
}

char* extractJsonRootValueFrom(char* key, char* json) {
	int curlyDepth = 0;
	char c;
	bool readingString = false;
	char keyBuffer[MAX_STRING_LEN];
	char valueBuffer[MAX_STRING_LEN];
	int stringBufferI = 0;
	bool stringEscaping = false;

	char* stringBuffer = keyBuffer;
	for(int i = 0; (c = json[i]) != 0; i++) {
		if(readingString) {
			if(stringEscaping) {
				c = escapedChar(c);
				stringEscaping = false;
			} else {
				if(c == '\\') {
					stringEscaping = true;
					continue;
				}

				if(c == '"') {
					readingString = false;
					stringBuffer[stringBufferI] = 0;
					stringBufferI = 0;
					continue;
				}
			}

			stringBuffer[stringBufferI++] = c;
			continue;
		}

		if(c == '{') {
			curlyDepth++;
			continue;
		}

		if(c == '}') {
			if(curlyDepth == 1 && strcmp(key, keyBuffer) == 0) {
				char* allocatedValue = malloc(strlen(valueBuffer) + 1);
				strcpy(allocatedValue, valueBuffer);
				return allocatedValue;
			}
			curlyDepth--;
			continue;
		}

		if(c == '"') {
			readingString = true;
		}

		if(c == ':') {
			stringBuffer = valueBuffer;
		}

		if(c == ',') {
			if(curlyDepth == 1 && strcmp(key, keyBuffer) == 0) {
				char* allocatedValue = malloc(strlen(valueBuffer) + 1);
				strcpy(allocatedValue, valueBuffer);
				return allocatedValue;
			}

			stringBuffer = keyBuffer;
		}
	}
	return NULL;
}
