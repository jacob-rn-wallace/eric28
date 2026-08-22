/*
 *   ini_file.c
 *
 *   This file is part of the Emu28 macOS/Linux port (see ../CLAUDE.md).
 *   Implementation of the INI-file functions declared in ini_file.h -
 *   see that file's header comment for the design.
 *
 *   New code, not derived from any Win32 SDK header. GPL-2.0, as a
 *   derivative work of Emu28 (Copyright (C) 2002 Christoph
 *   Giesselink), since it exists solely to run Emu28's source.
 */

#include "ini_file.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct IniEntry {
	char key[256];
	char value[1024];
	struct IniEntry *next;
} IniEntry;

typedef struct IniSection {
	char name[256];
	IniEntry *entries;
	struct IniSection *next;
} IniSection;

static void free_sections(IniSection *sec)
{
	while (sec) {
		IniSection *nextSec = sec->next;
		IniEntry *e = sec->entries;

		while (e) {
			IniEntry *nextE = e->next;
			free(e);
			e = nextE;
		}
		free(sec);
		sec = nextSec;
	}
}

static void trim(char *s)
{
	size_t len;
	char *start = s;

	while (*start == ' ' || *start == '\t')
		++start;
	if (start != s)
		memmove(s, start, strlen(start) + 1);

	len = strlen(s);
	while (len > 0 && (s[len - 1] == ' ' || s[len - 1] == '\t' || s[len - 1] == '\r' || s[len - 1] == '\n'))
		s[--len] = '\0';
}

/* Parses the whole file into an in-memory section list. Returns NULL
 * (an empty, valid, zero-section state - not an error) if the file
 * doesn't exist yet, matching Win32's behavior of treating a missing
 * INI file as "everything is at its default." */
static IniSection *parse_file(LPCTSTR lpFileName)
{
	FILE *f = fopen(lpFileName, "r");
	IniSection *sections = NULL, *cur = NULL;
	char line[1536];

	if (f == NULL)
		return NULL;

	while (fgets(line, sizeof(line), f)) {
		char *p = line;
		char *eq;

		trim(p);
		if (*p == '\0' || *p == ';' || *p == '#')
			continue;

		if (*p == '[') {
			char *end = strchr(p, ']');
			IniSection *sec = (IniSection *)calloc(1, sizeof(IniSection));

			if (end)
				*end = '\0';
			strncpy(sec->name, p + 1, sizeof(sec->name) - 1);
			sec->next = sections;
			sections = sec;
			cur = sec;
			continue;
		}

		eq = strchr(p, '=');
		if (eq && cur) {
			IniEntry *e = (IniEntry *)calloc(1, sizeof(IniEntry));

			*eq = '\0';
			strncpy(e->key, p, sizeof(e->key) - 1);
			strncpy(e->value, eq + 1, sizeof(e->value) - 1);
			trim(e->key);
			e->next = cur->entries;
			cur->entries = e;
		}
	}
	fclose(f);
	return sections;
}

static BOOL write_file(LPCTSTR lpFileName, IniSection *sections)
{
	/* sections/entries were built by prepending (see parse_file and
	 * set_value below), so walk them in reverse to write the file
	 * back out in something close to first-seen/insertion order. */
	FILE *f = fopen(lpFileName, "w");
	IniSection **secOrder;
	INT n = 0, i;
	IniSection *s;

	if (f == NULL)
		return FALSE;

	for (s = sections; s; s = s->next)
		++n;
	secOrder = (IniSection **)malloc(sizeof(IniSection *) * (n ? n : 1));
	for (s = sections, i = n - 1; s; s = s->next, --i)
		secOrder[i] = s;

	for (i = 0; i < n; ++i) {
		IniEntry **entOrder;
		INT m = 0, j;
		IniEntry *e;

		if (secOrder[i]->entries == NULL)
			continue; /* an emptied-out section - drop it, matching WritePrivateProfileString's own behavior */

		fprintf(f, "[%s]\n", secOrder[i]->name);

		for (e = secOrder[i]->entries; e; e = e->next)
			++m;
		entOrder = (IniEntry **)malloc(sizeof(IniEntry *) * m);
		for (e = secOrder[i]->entries, j = m - 1; e; e = e->next, --j)
			entOrder[j] = e;
		for (j = 0; j < m; ++j)
			fprintf(f, "%s=%s\n", entOrder[j]->key, entOrder[j]->value);
		free(entOrder);
		fprintf(f, "\n");
	}
	free(secOrder);
	fclose(f);
	return TRUE;
}

static IniSection *find_section(IniSection *sections, LPCTSTR name)
{
	for (; sections; sections = sections->next)
		if (strcasecmp(sections->name, name) == 0)
			return sections;
	return NULL;
}

static IniEntry *find_entry(IniSection *section, LPCTSTR key)
{
	IniEntry *e;

	if (section == NULL)
		return NULL;
	for (e = section->entries; e; e = e->next)
		if (strcasecmp(e->key, key) == 0)
			return e;
	return NULL;
}

DWORD GetPrivateProfileString(LPCTSTR lpAppName, LPCTSTR lpKeyName, LPCTSTR lpDefault,
                               LPTSTR lpReturnedString, DWORD nSize, LPCTSTR lpFileName)
{
	IniSection *sections = parse_file(lpFileName);
	IniEntry *e = find_entry(find_section(sections, lpAppName), lpKeyName);
	LPCTSTR value = e ? e->value : lpDefault;

	strncpy(lpReturnedString, value, (size_t)nSize - 1);
	lpReturnedString[nSize - 1] = '\0';
	free_sections(sections);
	return (DWORD)strlen(lpReturnedString);
}

UINT GetPrivateProfileInt(LPCTSTR lpAppName, LPCTSTR lpKeyName, INT nDefault, LPCTSTR lpFileName)
{
	char buf[32];
	char defaultStr[32];

	snprintf(defaultStr, sizeof(defaultStr), "%d", nDefault);
	GetPrivateProfileString(lpAppName, lpKeyName, defaultStr, buf, sizeof(buf), lpFileName);
	return (UINT)atoi(buf);
}

BOOL WritePrivateProfileString(LPCTSTR lpAppName, LPCTSTR lpKeyName, LPCTSTR lpString, LPCTSTR lpFileName)
{
	IniSection *sections = parse_file(lpFileName);
	IniSection *sec = find_section(sections, lpAppName);
	BOOL ok;

	if (lpKeyName == NULL) {
		/* delete the whole section */
		if (sec) {
			IniEntry *e = sec->entries;
			while (e) {
				IniEntry *next = e->next;
				free(e);
				e = next;
			}
			sec->entries = NULL;
		}
	} else if (lpString == NULL) {
		/* delete just this key */
		if (sec) {
			IniEntry **link = &sec->entries;
			while (*link) {
				if (strcasecmp((*link)->key, lpKeyName) == 0) {
					IniEntry *dead = *link;
					*link = dead->next;
					free(dead);
					break;
				}
				link = &(*link)->next;
			}
		}
	} else {
		IniEntry *e;

		if (sec == NULL) {
			sec = (IniSection *)calloc(1, sizeof(IniSection));
			strncpy(sec->name, lpAppName, sizeof(sec->name) - 1);
			sec->next = sections;
			sections = sec;
		}
		e = find_entry(sec, lpKeyName);
		if (e == NULL) {
			e = (IniEntry *)calloc(1, sizeof(IniEntry));
			strncpy(e->key, lpKeyName, sizeof(e->key) - 1);
			e->next = sec->entries;
			sec->entries = e;
		}
		strncpy(e->value, lpString, sizeof(e->value) - 1);
	}

	ok = write_file(lpFileName, sections);
	free_sections(sections);
	return ok;
}
