#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "inoue.h"

enum cfgword {
	CFG_NONE = 0,
	CFG_USER,
	CFG_FORMAT,
	CFG_40L,
	CFG_BLITZ,
	CFG_LEAGUE,
	CFG_USERAGENT,
	CFG_APIURL,
	CFG_NEXT,
};

static enum cfgword
read_word(const char *c, const char **n)
{
	const char *words[] = {
		"user", "saveas", "40l", "blitz", "league", "useragent", "useapi", "also", NULL
	};
	for (int i = 0; words[i] != NULL; i++) {
		int l = strlen(words[i]);
		if (memcmp(c, words[i], l) == 0 && (isspace(c[l]) || c[l] == '\0')) {
			if (n)
				*n = &c[l+1];
			return i+1;
		}
	}
	return CFG_NONE;
}

char *
read_string(const char **_c)
{
	const char *c = *_c;
	while (isspace(*c) && *c) c++;
	if (!c) {
		fprintf(stderr, "config: expected value\n");
		return NULL;
	}
	char buf[1024];
	int bufc = 0;
	if (*c == '"') { // quoted
		c++;
		for (;;) {
			char ch = *c;
			if (!ch) {
				fprintf(stderr, "config: no matching quote while reading value\n");
				return NULL;
			}
			if (ch == '\\' && *(c+1)) {
				buf[bufc++] = *(c+1);
				c += 2;
				continue;
			}
			if (ch == '"') {
				buf[bufc] = 0;
				*_c = c+1;
				return strdup(buf);
			}
			buf[bufc++] = ch;
			c++;
		}
	} else { // unquoted
		for (;;) {
			char ch = *c;
			if (isspace(ch) || !ch) {
				if (bufc) {
					buf[bufc] = 0;
					*_c = c;
					return strdup(buf);
				} else {
					fprintf(stderr, "config: empty value?\n");
					return NULL;
				}
			}
			buf[bufc++] = ch;
			c++;
		}
	}
}
enum tasktype {
	TSK_NONE = 0,
	TSK_40L,
	TSK_BLITZ,
	TSK_LEAGUE,
};

int
loadcfgnew(void)
{
	FILE* f = fopen("inoue.cfg", "r");
	if (!f) {
		perror("inoue: couldnt open config file");
		return 0;
	}
	buffer *b = buffer_new();
	buffer_load(b, f);
	buffer_appendchar(b, 0);
	fclose(f);
	const char *c = buffer_str(b);
	char *user = NULL;
	char *apiurl = NULL;
	char *useragent = NULL;
	char *format = NULL;
	enum tasktype type = TSK_NONE;
	int err = 1;
	for (;;) {
		while (isspace(*c) && *c) c++;
		if (*c == '#') {
			for (;;) {
				char ch = *c;
				if (!ch) break;
				c++;
				if (ch == '\n') {
					break;
				}
			}
		}
		if (!*c)
			break; // eof
		const char *next = NULL;
		enum cfgword word = read_word(c, &next);
		if (word == CFG_NONE) {
			fprintf(stderr, "config: unrecognized word at '%.10s'...\n", c);
			goto exit;
		}
		c = next;

#define READ_VALUE(__var) \
	if (__var) free(__var); \
	__var = read_string(&c); \
	if (!__var) goto exit;

#define CHECK_TYPE() if (type) { fprintf(stderr, type_err); goto exit; }
		const char *type_err = "config: you can only download one type of replay at the same time\n";

		switch (word) {
			case CFG_NONE:
				break; // unreachable
			case CFG_USER:
				READ_VALUE(user);
				break;
			case CFG_FORMAT:
				READ_VALUE(format);
				break;
			case CFG_APIURL:
				READ_VALUE(apiurl);
				break;
			case CFG_USERAGENT:
				READ_VALUE(useragent);
				break;
			case CFG_40L:
				CHECK_TYPE();
				type = TSK_40L;
				break;
			case CFG_BLITZ:
				CHECK_TYPE();
				type = TSK_BLITZ;
				break;
			case CFG_LEAGUE:
				CHECK_TYPE();
				type = TSK_BLITZ;
				break;
			case CFG_NEXT:
				printf("[process user %s for %d with fmt %s]\n", user, type, format);
				type = 0;
				if (format) {
					free(format);
					format = NULL;
				}
				break;
		}
	}
	printf("[process user %s for %d with fmt %s]\n", user, type, format);
	err = 0;
exit:
	buffer_free(b);
	return err;
}