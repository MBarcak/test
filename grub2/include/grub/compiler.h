/* compiler.h - macros for various compiler features */
/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2002,2003,2005,2006,2007,2008,2009,2010,2014  Free Software Foundation, Inc.
 *
 *  GRUB is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  GRUB is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with GRUB.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef GRUB_COMPILER_HEADER
#define GRUB_COMPILER_HEADER	1

/* GCC version checking borrowed from glibc. */
#if defined(__GNUC__) && defined(__GNUC_MINOR__)
#  define GNUC_PREREQ(maj,min) \
	((__GNUC__ << 16) + __GNUC_MINOR__ >= ((maj) << 16) + (min))
#else
#  define GNUC_PREREQ(maj,min) 0
#endif

/* Does this compiler support compile-time error attributes? */
#if GNUC_PREREQ(4,3)
#  define ATTRIBUTE_ERROR(msg) \
	__attribute__ ((__error__ (msg)))
#else
#  define ATTRIBUTE_ERROR(msg) __attribute__ ((noreturn))
#endif

#if GNUC_PREREQ(4,4)
#  define GNU_PRINTF gnu_printf
#else
#  define GNU_PRINTF printf
#endif

#if GNUC_PREREQ(3,4)
#  define WARN_UNUSED_RESULT __attribute__ ((warn_unused_result))
#else
#  define WARN_UNUSED_RESULT
#endif

#if defined(__clang__) && defined(__clang_major__) && defined(__clang_minor__)
#  define CLANG_PREREQ(maj,min) \
          ((__clang_major__ > (maj)) || \
	   (__clang_major__ == (maj) && __clang_minor__ >= (min)))
#else
#  define CLANG_PREREQ(maj,min) 0
#endif

#if defined(__GNUC__)
#  define grub_absolute_pointer(val)					\
({									\
	unsigned long __ptr;						\
	__asm__ ("" : "=r"(__ptr) : "0"((void *)(val)));		\
	(void *) (__ptr);						\
})
#else
#  define grub_absolute_pointer(val) ((void *)(val))
#endif

#endif /* ! GRUB_COMPILER_HEADER */
