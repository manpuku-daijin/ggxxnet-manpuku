#pragma once

//******************************************************************
// include
//******************************************************************
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdio.h>

#define ZLIB_WINAPI 1
#ifdef MANPUKU
#include "../zlib-1.2.8/zlib.h"
#else
#include "../zlib125/zlib.h"
#endif // #ifdef MANPUKU


//******************************************************************
// prototypes
//******************************************************************
int  zfsize(FILE* p_fp);
int  zfsig(FILE* p_fp);
void zfread(char* p_ptr, int p_size, FILE* p_fp);
void zfwrite(char* p_ptr, int p_size, FILE* p_fp, int p_sig);

int zmsize(char* p_inptr);
int zmread(char* p_inptr, int p_insize, char* p_outptr, int p_outsize);
int zmwrite(char* p_inptr, int p_insize, char* p_outptr, int p_outsize);
