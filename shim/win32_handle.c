/*
 *   win32_handle.c
 *
 *   This file is part of the Emu28 macOS/Linux port (see ../CLAUDE.md).
 *   Implementation of the tagged-dispatch HANDLE declared in
 *   win32_handle.h - see that file's header comment for the design.
 *
 *   New code, not derived from any Win32 SDK header. GPL-2.0, as a
 *   derivative work of Emu28 (Copyright (C) 2002 Christoph
 *   Giesselink), since it exists solely to run Emu28's source.
 */

#include "win32_handle.h"

#include <errno.h>
#include <stdlib.h>
#include <sys/mman.h>

typedef enum {
	WIN32_HANDLE_FILE,
	WIN32_HANDLE_EVENT,
	WIN32_HANDLE_THREAD,
	WIN32_HANDLE_FILEMAP
} Win32HandleKind;

typedef struct {
	Win32HandleKind type; /* same field name as gdi.c's tag, deliberately -
	                       * same trick, same idiom */
	pthread_mutex_t mutex; /* unused for WIN32_HANDLE_FILE */
	pthread_cond_t  cond;   /* unused for WIN32_HANDLE_FILE */
	union {
		struct {
			int fd;
		} file;
		struct {
			BOOL manualReset;
			BOOL signaled;
		} event;
		struct {
			pthread_t thread;
			LPTHREAD_START_ROUTINE start;
			LPVOID param;
			DWORD result;
			BOOL finished;
		} thread;
		struct {
			int fd; /* dup()'d from the source file - a mapping outlives CloseHandle(hFile), matching real Win32 */
			size_t size;
		} filemap;
	} u;
} Win32Handle;

/* ---- file I/O ------------------------------------------------------------------- */

HANDLE CreateFile(LPCTSTR lpFileName, DWORD dwDesiredAccess, DWORD dwShareMode,
                   LPVOID lpSecurityAttributes, DWORD dwCreationDisposition,
                   DWORD dwFlagsAndAttributes, HANDLE hTemplateFile)
{
	Win32Handle *h;
	int flags, fd;

	(void)dwShareMode;
	(void)lpSecurityAttributes;
	(void)dwFlagsAndAttributes;
	(void)hTemplateFile;

	if ((dwDesiredAccess & GENERIC_READ) && (dwDesiredAccess & GENERIC_WRITE))
		flags = O_RDWR;
	else if (dwDesiredAccess & GENERIC_WRITE)
		flags = O_WRONLY;
	else
		flags = O_RDONLY;

	if (dwCreationDisposition == CREATE_ALWAYS)
		flags |= O_CREAT | O_TRUNC;

	fd = open(lpFileName, flags, 0644);
	if (fd < 0)
		return INVALID_HANDLE_VALUE;

	h = (Win32Handle *)calloc(1, sizeof(Win32Handle));
	h->type = WIN32_HANDLE_FILE;
	h->u.file.fd = fd;
	return (HANDLE)h;
}

BOOL ReadFile(HANDLE hFile, LPVOID lpBuffer, DWORD nNumberOfBytesToRead,
              LPDWORD lpNumberOfBytesRead, LPVOID lpOverlapped)
{
	Win32Handle *h = (Win32Handle *)hFile;
	ssize_t n;

	(void)lpOverlapped;
	n = read(h->u.file.fd, lpBuffer, nNumberOfBytesToRead);
	if (lpNumberOfBytesRead)
		*lpNumberOfBytesRead = (n < 0) ? 0 : (DWORD)n;
	return n >= 0;
}

BOOL WriteFile(HANDLE hFile, LPCVOID lpBuffer, DWORD nNumberOfBytesToWrite,
               LPDWORD lpNumberOfBytesWritten, LPVOID lpOverlapped)
{
	Win32Handle *h = (Win32Handle *)hFile;
	ssize_t n;

	(void)lpOverlapped;
	n = write(h->u.file.fd, lpBuffer, nNumberOfBytesToWrite);
	if (lpNumberOfBytesWritten)
		*lpNumberOfBytesWritten = (n < 0) ? 0 : (DWORD)n;
	return n >= 0;
}

DWORD SetFilePointer(HANDLE hFile, LONG lDistanceToMove, LONG *lpDistanceToMoveHigh, DWORD dwMoveMethod)
{
	Win32Handle *h = (Win32Handle *)hFile;
	off_t pos;
	int whence;

	(void)lpDistanceToMoveHigh;
	switch (dwMoveMethod) {
	case FILE_CURRENT: whence = SEEK_CUR; break;
	case FILE_END:      whence = SEEK_END; break;
	default:            whence = SEEK_SET; break;
	}

	pos = lseek(h->u.file.fd, lDistanceToMove, whence);
	return (pos == (off_t)-1) ? INVALID_SET_FILE_POINTER : (DWORD)pos;
}

DWORD GetFileSize(HANDLE hFile, LPDWORD lpFileSizeHigh)
{
	Win32Handle *h = (Win32Handle *)hFile;
	struct stat st;

	if (fstat(h->u.file.fd, &st) != 0)
		return INVALID_SET_FILE_POINTER;
	if (lpFileSizeHigh)
		*lpFileSizeHigh = (DWORD)((uint64_t)st.st_size >> 32);
	return (DWORD)st.st_size;
}

BOOL SetEndOfFile(HANDLE hFile)
{
	Win32Handle *h = (Win32Handle *)hFile;
	off_t pos = lseek(h->u.file.fd, 0, SEEK_CUR); /* truncate at the current file pointer, matching real Win32 */

	return pos != (off_t)-1 && ftruncate(h->u.file.fd, pos) == 0;
}

BOOL FlushFileBuffers(HANDLE hFile)
{
	Win32Handle *h = (Win32Handle *)hFile;

	return fsync(h->u.file.fd) == 0;
}

/* ---- file mapping ------------------------------------------------------------------
 * MapViewOfFile hands back a bare pointer (matching real Win32), not a
 * tagged HANDLE, so UnmapViewOfFile can't recover the mapping's length
 * from the pointer itself the way CloseHandle reads a type tag - a
 * small side list of active mappings does that instead. Linear search
 * is fine here: FILES.C never has more than one or two mappings open
 * at once. */

typedef struct MappedRegion {
	LPVOID addr;
	size_t len;
	struct MappedRegion *next;
} MappedRegion;

static MappedRegion *g_mappedRegions = NULL;

HANDLE CreateFileMapping(HANDLE hFile, LPVOID lpAttributes, DWORD flProtect,
                          DWORD dwMaximumSizeHigh, DWORD dwMaximumSizeLow, LPCTSTR lpName)
{
	Win32Handle *src = (Win32Handle *)hFile;
	Win32Handle *h;
	int dupFd;
	struct stat st;

	(void)lpAttributes;
	(void)flProtect;     /* only the PAGE_READONLY/read-only path is used anywhere in this codebase */
	(void)dwMaximumSizeHigh;
	(void)lpName;         /* named/shared mappings aren't used anywhere in this codebase */

	if (fstat(src->u.file.fd, &st) != 0)
		return NULL;

	dupFd = dup(src->u.file.fd);
	if (dupFd < 0)
		return NULL;

	h = (Win32Handle *)calloc(1, sizeof(Win32Handle));
	h->type = WIN32_HANDLE_FILEMAP;
	h->u.filemap.fd = dupFd;
	h->u.filemap.size = dwMaximumSizeLow ? dwMaximumSizeLow : (size_t)st.st_size; /* 0 = map the whole file */
	return (HANDLE)h;
}

LPVOID MapViewOfFile(HANDLE hFileMappingObject, DWORD dwDesiredAccess,
                      DWORD dwFileOffsetHigh, DWORD dwFileOffsetLow, SIZE_T dwNumberOfBytesToMap)
{
	Win32Handle *h = (Win32Handle *)hFileMappingObject;
	size_t len = dwNumberOfBytesToMap ? dwNumberOfBytesToMap : h->u.filemap.size;
	off_t offset = ((off_t)dwFileOffsetHigh << 32) | dwFileOffsetLow;
	void *addr;
	MappedRegion *region;

	(void)dwDesiredAccess; /* only FILE_MAP_READ is used anywhere in this codebase */

	if (len == 0)
		return NULL; /* mmap()ing a zero-length region is undefined - real Win32 also fails this */

	addr = mmap(NULL, len, PROT_READ, MAP_PRIVATE, h->u.filemap.fd, offset);
	if (addr == MAP_FAILED)
		return NULL;

	region = (MappedRegion *)malloc(sizeof(MappedRegion));
	region->addr = addr;
	region->len = len;
	region->next = g_mappedRegions;
	g_mappedRegions = region;
	return addr;
}

BOOL UnmapViewOfFile(LPCVOID lpBaseAddress)
{
	MappedRegion **link = &g_mappedRegions;

	while (*link) {
		if ((*link)->addr == lpBaseAddress) {
			MappedRegion *dead = *link;

			munmap(dead->addr, dead->len);
			*link = dead->next;
			free(dead);
			return TRUE;
		}
		link = &(*link)->next;
	}
	return FALSE; /* not a live mapping - matches real Win32's failure return */
}

/* ---- events --------------------------------------------------------------------- */

static Win32Handle *alloc_sync_handle(Win32HandleKind type)
{
	Win32Handle *h = (Win32Handle *)calloc(1, sizeof(Win32Handle));

	h->type = type;
	pthread_mutex_init(&h->mutex, NULL);
	pthread_cond_init(&h->cond, NULL);
	return h;
}

HANDLE CreateEvent(LPVOID lpEventAttributes, BOOL bManualReset, BOOL bInitialState, LPCTSTR lpName)
{
	Win32Handle *h = alloc_sync_handle(WIN32_HANDLE_EVENT);

	(void)lpEventAttributes;
	(void)lpName; /* named/shared events aren't used anywhere in this codebase */
	h->u.event.manualReset = bManualReset;
	h->u.event.signaled = bInitialState;
	return (HANDLE)h;
}

BOOL SetEvent(HANDLE hEvent)
{
	Win32Handle *h = (Win32Handle *)hEvent;

	pthread_mutex_lock(&h->mutex);
	h->u.event.signaled = TRUE;
	if (h->u.event.manualReset)
		pthread_cond_broadcast(&h->cond); /* every waiter should see it */
	else
		pthread_cond_signal(&h->cond); /* only one waiter should consume it */
	pthread_mutex_unlock(&h->mutex);
	return TRUE;
}

BOOL ResetEvent(HANDLE hEvent)
{
	Win32Handle *h = (Win32Handle *)hEvent;

	pthread_mutex_lock(&h->mutex);
	h->u.event.signaled = FALSE;
	pthread_mutex_unlock(&h->mutex);
	return TRUE;
}

/* ---- threads ---------------------------------------------------------------------- */

static void *thread_trampoline(void *arg)
{
	Win32Handle *h = (Win32Handle *)arg;
	DWORD result = h->u.thread.start(h->u.thread.param);

	pthread_mutex_lock(&h->mutex);
	h->u.thread.result = result;
	h->u.thread.finished = TRUE;
	pthread_cond_broadcast(&h->cond);
	pthread_mutex_unlock(&h->mutex);
	return NULL;
}

HANDLE CreateThread(LPVOID lpThreadAttributes, SIZE_T dwStackSize,
                     LPTHREAD_START_ROUTINE lpStartAddress, LPVOID lpParameter,
                     DWORD dwCreationFlags, LPDWORD lpThreadId)
{
	Win32Handle *h = alloc_sync_handle(WIN32_HANDLE_THREAD);

	(void)lpThreadAttributes;
	(void)dwStackSize;       /* the pthread default is fine for this codebase's threads */
	(void)dwCreationFlags;   /* CREATE_SUSPENDED isn't used anywhere in this codebase */

	h->u.thread.start = lpStartAddress;
	h->u.thread.param = lpParameter;
	if (pthread_create(&h->u.thread.thread, NULL, thread_trampoline, h) != 0) {
		free(h);
		return NULL;
	}
	if (lpThreadId)
		*lpThreadId = (DWORD)(uintptr_t)h; /* only ever used as an opaque id by this codebase, never as a real TID */
	return (HANDLE)h;
}

/* ---- the shared wait primitive ------------------------------------------------------
 * Events and threads both reduce to "wait until this mutex-guarded
 * flag becomes true" - which is exactly the polymorphism real Win32's
 * WaitForSingleObject relies on, and exactly why ENGINE.C is allowed
 * to call it on either kind without knowing which it has. */

static DWORD wait_on_flag(pthread_mutex_t *mutex, pthread_cond_t *cond, BOOL *flag, DWORD dwMilliseconds)
{
	DWORD result = WAIT_OBJECT_0;
	struct timespec deadline;

	if (dwMilliseconds != INFINITE) {
		clock_gettime(CLOCK_REALTIME, &deadline);
		deadline.tv_sec += dwMilliseconds / 1000;
		deadline.tv_nsec += (long)(dwMilliseconds % 1000) * 1000000L;
		if (deadline.tv_nsec >= 1000000000L) {
			deadline.tv_sec += 1;
			deadline.tv_nsec -= 1000000000L;
		}
	}

	pthread_mutex_lock(mutex);
	while (!*flag) {
		if (dwMilliseconds == INFINITE) {
			pthread_cond_wait(cond, mutex);
		} else if (pthread_cond_timedwait(cond, mutex, &deadline) == ETIMEDOUT) {
			result = WAIT_TIMEOUT;
			break;
		}
	}
	pthread_mutex_unlock(mutex);
	return result;
}

DWORD WaitForSingleObject(HANDLE hHandle, DWORD dwMilliseconds)
{
	Win32Handle *h = (Win32Handle *)hHandle;
	DWORD result;

	if (h->type == WIN32_HANDLE_EVENT) {
		result = wait_on_flag(&h->mutex, &h->cond, &h->u.event.signaled, dwMilliseconds);
		/* auto-reset: a successful wait consumes the signal */
		if (result == WAIT_OBJECT_0 && !h->u.event.manualReset) {
			pthread_mutex_lock(&h->mutex);
			h->u.event.signaled = FALSE;
			pthread_mutex_unlock(&h->mutex);
		}
		return result;
	}

	if (h->type == WIN32_HANDLE_THREAD)
		return wait_on_flag(&h->mutex, &h->cond, &h->u.thread.finished, dwMilliseconds);

	return WAIT_FAILED; /* a file HANDLE - never a valid WaitForSingleObject argument in this codebase */
}

/* ---- common to all three kinds -------------------------------------------------------- */

BOOL CloseHandle(HANDLE hObject)
{
	Win32Handle *h = (Win32Handle *)hObject;

	if (h == NULL)
		return FALSE;

	switch (h->type) {
	case WIN32_HANDLE_FILE:
		close(h->u.file.fd);
		break;
	case WIN32_HANDLE_FILEMAP:
		close(h->u.filemap.fd); /* any views mapped from it stay valid until their own UnmapViewOfFile, matching real Win32 */
		break;
	case WIN32_HANDLE_THREAD:
		pthread_join(h->u.thread.thread, NULL); /* real Win32 doesn't require this, but leaking the OS thread would be worse */
		pthread_mutex_destroy(&h->mutex);
		pthread_cond_destroy(&h->cond);
		break;
	case WIN32_HANDLE_EVENT:
		pthread_mutex_destroy(&h->mutex);
		pthread_cond_destroy(&h->cond);
		break;
	}
	free(h);
	return TRUE;
}
