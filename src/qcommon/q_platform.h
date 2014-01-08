/*
===========================================================================
Copyright (C) 1999-2005 Id Software, Inc.

This file is part of Quake III Arena source code.

Quake III Arena source code is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.

Quake III Arena source code is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Quake III Arena source code; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
===========================================================================
*/

#pragma once

#if (defined _M_IX86 || defined __i386__) && !defined(C_ONLY)
	#define id386
#endif

#if (defined(powerc) || defined(powerpc) || defined(ppc) || defined(__ppc) || defined(__ppc__)) && !defined(C_ONLY)
	#define idppc
	#if defined(__VEC__)
		#define idppc_altivec
		#ifdef MACOS_X  // Apple's GCC does this differently than the FSF.
			#define VECCONST_UINT8(a,b,c,d,e,f,g,h,i,j,k,l,m,n,o,p) (vector unsigned char) (a,b,c,d,e,f,g,h,i,j,k,l,m,n,o,p)
		#else
			#define VECCONST_UINT8(a,b,c,d,e,f,g,h,i,j,k,l,m,n,o,p) (vector unsigned char) {a,b,c,d,e,f,g,h,i,j,k,l,m,n,o,p}
		#endif
	#endif
#endif

#if defined(__sparc__) && !defined(C_ONLY)
	#define idsparc
#endif

#ifndef __ASM_I386__ // don't include the C bits if included from qasm.h

	// for windows fastcall option
	#define QDECL
	#define QCALL

	// Win64
	#if defined(_WIN64) || defined(__WIN64__)

		#define idx64

		#undef QDECL
		#define QDECL __cdecl

		#undef QCALL
		#define QCALL __stdcall

		#if defined(_MSC_VER)
			#define OS_STRING "win_msvc64"
		#elif defined(__MINGW64__)
			#define OS_STRING "win_mingw64"
		#endif

		#define QINLINE __inline
		#define PATH_SEP '\\'

		#define Q3_LITTLE_ENDIAN

		#define DLL_EXT ".dll"

	// Win32
	#elif defined(_WIN32) || defined(__WIN32__)

		#undef QDECL
		#define QDECL __cdecl

		#undef QCALL
		#define QCALL __stdcall

		#if defined(_MSC_VER)
			#define OS_STRING "win_msvc"
		#elif defined(__MINGW32__)
			#define OS_STRING "win_mingw"
		#endif

		#define QINLINE __inline
		#define PATH_SEP '\\'

		#define Q3_LITTLE_ENDIAN

		#define DLL_EXT ".dll"

	// MAC OS X
	#elif defined(MACOS_X) || defined(__APPLE_CC__)

		// make sure this is defined, just for sanity's sake...
		#ifndef MACOS_X
			#define MACOS_X
		#endif

		#define OS_STRING "macosx"
		#define QINLINE inline
		#define PATH_SEP '/'

		#if defined(__ppc__)
			#define Q3_BIG_ENDIAN
		#elif defined(__i386__)
			#define Q3_LITTLE_ENDIAN
		#elif defined(__x86_64__)
			#define idx64
			#define Q3_LITTLE_ENDIAN
		#endif

		#define DLL_EXT ".dylib"

	// Linux
	#elif defined(__linux__) || defined(__FreeBSD_kernel__)

		#include <endian.h>

		#if defined(__linux__)
			#define OS_STRING "linux"
		#else
			#define OS_STRING "kFreeBSD"
		#endif

		#ifdef __clang__
			#define QINLINE static inline
		#else
			#define QINLINE inline
		#endif

		#define PATH_SEP '/'

		#if defined(__x86_64__)
			#define idx64
		#endif

		#if __FLOAT_WORD_ORDER == __BIG_ENDIAN
			#define Q3_BIG_ENDIAN
		#else
			#define Q3_LITTLE_ENDIAN
		#endif

		#define DLL_EXT ".so"

	// BSD
	#elif defined(__FreeBSD__) || defined(__OpenBSD__) || defined(__NetBSD__)

		#include <sys/types.h>
		#include <machine/endian.h>
		
		#ifndef __BSD__
			#define __BSD__
		#endif

		#if defined(__FreeBSD__)
			#define OS_STRING "freebsd"
		#elif defined(__OpenBSD__)
			#define OS_STRING "openbsd"
		#elif defined(__NetBSD__)
			#define OS_STRING "netbsd"
		#endif

		#define QINLINE inline
		#define PATH_SEP '/'

		#if defined(__amd64__)
			#define idx64
		#endif
		
		#if BYTE_ORDER == BIG_ENDIAN
			#define Q3_BIG_ENDIAN
		#else
			#define Q3_LITTLE_ENDIAN
		#endif
		
		#define DLL_EXT ".so"

	// SUNOS
	#elif defined(__sun)

		#include <stdint.h>
		#include <sys/byteorder.h>
		
		#define OS_STRING "solaris"
		#define QINLINE inline
		#define PATH_SEP '/'

		#if defined(_BIG_ENDIAN)
			#define Q3_BIG_ENDIAN
		#elif defined(_LITTLE_ENDIAN)
			#define Q3_LITTLE_ENDIAN
		#endif
		
		#define DLL_EXT ".so"

	// IRIX
	#elif defined(__sgi)
	
		#define OS_STRING "irix"
		#define QINLINE __inline
		#define PATH_SEP '/'

		#define Q3_BIG_ENDIAN // SGI's MIPS are always big endian
	
		#define DLL_EXT ".so"
	
	#endif

	// catch missing defines in above blocks
	#if !defined(OS_STRING)
		#error "Operating system not supported"
	#endif
	#if !defined(ARCH_STRING)
		#error "Architecture not supported"
	#endif
	#if !defined(QINLINE)
		#error "QINLINE not defined"
	#endif
	#if !defined(PATH_SEP)
		#error "PATH_SEP not defined"
	#endif
	#if !defined(DLL_EXT)
		#error "DLL_EXT not defined"
	#endif
	
	
	// endianness
	void CopyShortSwap( void *dest, void *src );
	void CopyLongSwap( void *dest, void *src );
	short ShortSwap( short l );
	int LongSwap( int l );
	float FloatSwap( const float *f );
	
	#if defined(Q3_BIG_ENDIAN) && defined(Q3_LITTLE_ENDIAN)
		#error "Endianness defined as both big and little"
	#elif defined(Q3_BIG_ENDIAN)
		#define CopyLittleShort( dest, src )	CopyShortSwap( dest, src )
		#define CopyLittleLong( dest, src )		CopyLongSwap( dest, src )
		#define LittleShort( x )				ShortSwap( x )
		#define LittleLong( x )					LongSwap( x )
		#define LittleFloat( x )				FloatSwap( &x )
		#define BigShort
		#define BigLong
		#define BigFloat
	#elif defined( Q3_LITTLE_ENDIAN )
		#define CopyLittleShort( dest, src )	memcpy(dest, src, 2)
		#define CopyLittleLong( dest, src )		memcpy(dest, src, 4)
		#define LittleShort
		#define LittleLong
		#define LittleFloat
		#define BigShort( x )					ShortSwap( x )
		#define BigLong( x )					LongSwap( x )
		#define BigFloat( x )					FloatSwap( &x )
	#elif defined( Q3_VM )
		#define LittleShort
		#define LittleLong
		#define LittleFloat
		#define BigShort
		#define BigLong
		#define BigFloat
	#else
		#error "Endianness not defined"
	#endif
	
	
	// platform string
	#if defined(NDEBUG)
		#define PLATFORM_STRING OS_STRING "-" ARCH_STRING
	#else
		#define PLATFORM_STRING OS_STRING "-" ARCH_STRING "-debug"
	#endif

#endif // __ASM_I386__
