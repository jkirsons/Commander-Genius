#ifndef SDL_system_h_
#define SDL_system_h_

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "SDL_stdinc.h"
#include "SDL.h"

#include "esp_vfs_fat.h"
#include "driver/sdmmc_host.h"
#include "driver/sdspi_host.h"

#include <sys/types.h>
#include <sys/stat.h>

typedef struct {
    Uint8 major;
    Uint8 minor;
    Uint8 patch;
} SDL_version;

#define SDL_MAJOR_VERSION   1
#define SDL_MINOR_VERSION   2
#define SDL_PATCHLEVEL      0

#define SDL_VERSION(x)                          \
{                                   \
    (x)->major = SDL_MAJOR_VERSION;                 \
    (x)->minor = SDL_MINOR_VERSION;                 \
    (x)->patch = SDL_PATCHLEVEL;                    \
}
const SDL_version* SDL_Linked_Version();

#define SDL_VERSIONNUM(X, Y, Z)						\
	((X)*1000 + (Y)*100 + (Z))

/** This is the version number macro for the current SDL version */
#define SDL_COMPILEDVERSION \
	SDL_VERSIONNUM(SDL_MAJOR_VERSION, SDL_MINOR_VERSION, SDL_PATCHLEVEL)

/** This macro will evaluate to true if compiled with SDL at least X.Y.Z */
#define SDL_VERSION_ATLEAST(X, Y, Z) \
	(SDL_COMPILEDVERSION >= SDL_VERSIONNUM(X, Y, Z))

int SDL_Init(Uint32 flags);
void SDL_Quit(void);

void SDL_Delay(Uint32 ms);
Uint32 SDL_GetTicks(void);

//const char *SDL_GetError(void);

#define MODE_SPI 1
void SDL_InitSD(void);

void Check(const char *str);

// File operations - safely lock/unlock display during operations...
int __mkdir(const char *path, mode_t mode);
FILE *__fopen( const char *path, const char *mode );
long __ftell( FILE *f );
int __feof ( FILE * stream );
int __fputc ( int character, FILE * stream );
int __fgetc ( FILE * stream );
size_t __fwrite ( const void * ptr, size_t size, size_t count, FILE * stream );
int __fclose ( FILE * stream );
int __fseek( FILE * stream, long int offset, int origin );
size_t __fread( void *buffer, size_t size, size_t num, FILE *stream );
int __stat(const char *path, struct stat *buf);

int __open(const char *path, int oflag, ... );
int __close(int fildes);
ssize_t __read(int fildes, void *buf, size_t nbyte);
ssize_t __write(int fildes, const void *buf, size_t nbyte);
off_t __lseek(int fd, off_t offset, int whence);
int __unlink(const char *pathname); 
#endif