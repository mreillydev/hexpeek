// Copyright 2020, 2025 Michael Reilly <mreilly@mreilly.dev>
// 
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions
// are met:
// 1. Redistributions of source code must retain the above copyright
//    notice, this list of conditions and the following disclaimer.
// 2. Redistributions in binary form must reproduce the above copyright
//    notice, this list of conditions and the following disclaimer in the
//    documentation and/or other materials provided with the distribution.
// 3. Neither the names of the copyright holders nor the names of the
//    contributors may be used to endorse or promote products derived from
//    this software without specific prior written permission.
// 
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
// PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDERS
// OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
// EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
// OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
// WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
// OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
// ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#ifndef _UTIL_H_
#define _UTIL_H_

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

// Some constants
#if (INTMAX_MAX <= INT64_MAX)
    #define MAX_DEC 20 // 18446744073709551615
#else
    #error
#endif

// Strip text from definition (macro to string)
#define MSR(m) #m
#define MS(m) MSR(m)

// Basic math macros
#define MIN(a, b)            ( (a) < (b) ? (a) : (b) )
#define MAX(a, b)            ( (a) > (b) ? (a) : (b) )

// Distance to the next bound, exclusive of o
#define distbound(o, b)      ( ((o) % (b)) ? ((b) - (o) % (b)) : (b) )

// Distance to the next bound, inclusive of o
#define distbound_incl(o, b) ( ((o) % (b)) ? ((b) - (o) % (b)) :  0  )

// Round o up to the next multiple of b (unless it already is a multiple of b)
#define ceilbound(o, b)      ( (o) + distbound_incl(o, b) )

// Largest multiple of b not greater than l
#define _bestfit(b, l)       ( (b) >= (l) ? (l) : ((l) / (b)) * (b) )
#define bestfit(b, l)        ( (b) <= 0   ?  0  : _bestfit(b, l) )

// Test buffer for zeroness
int iszero(const void *ptr, size_t len);

// Integral power function
int powi(int base, int power);

// Pluralize the string
#define plrztn(t)            (t), ((t) == 1 ? "" : "s")

// Space removal
#define iswhspace(c)          ((c) == ' ')
#define stripLeadingSpaces(l) for( ; iswhspace(*(l)); (l)++);
void stripTrailingSpaces(char *line);

// Emplace newline control characters into string
char* emplaceNewlines(char *ptr);

// String equality
#define streq(a, b) (strcmp((a), (b)) == 0)

// Substring equality, advancing a if equal
int strnconsume(char const **a, char const *b, ssize_t len);
int strnconsumealnum(char const **a, char const *b, ssize_t len);
int strnconsumetrail(char const **a, char const *b, char const *t);

// Membership of character in the given list
bool memberof(char ch, char const *list);
bool memberofexnul(char ch, char const *list);

// Search for character within substring
char const *strnchr(char const *str, int ch, size_t slen);

// Bit information
int highbit(uint8_t octet);
int lowbit(uint8_t octet);
int countbit(uint8_t octet);

// Tracing defines
#ifndef SRCNAME
    #define SRCNAME __FILE__
#endif
#ifndef UTIL_TRACE_TIMER
    #define UTIL_TRACE_TIMER ""
#endif
#ifndef UTIL_TRACE_WIDTH
    #define UTIL_TRACE_WIDTH 0
#endif

// Tracing functions
void trace_fmt(FILE *fp, char const *fmt, ...);
void trace_close(FILE *fp);
#define TRC(p, s, l, f, ...) trace_fmt(p, "%s%s.%-*d  " f, UTIL_TRACE_TIMER, (s), UTIL_TRACE_WIDTH, (l), __VA_ARGS__)
#define TRACE(p, s, l, f, ...) trace_fmt(p, "%s%s.%-*d  %s%s" f "%s", UTIL_TRACE_TIMER, (s), UTIL_TRACE_WIDTH, (l), __VA_ARGS__)
#define tracef(p, f, ...) TRACE(p, SRCNAME, __LINE__, f, "", "", ##__VA_ARGS__, "") // -nl
#define tracefEntry(p, f, ...) TRACE(p, SRCNAME, __LINE__, f, __FUNCTION__, "(", ##__VA_ARGS__, ")\n")
#define tracefExit(p, f, ...) TRACE(p, SRCNAME, __LINE__, f, __FUNCTION__, "() returns ", ##__VA_ARGS__, "\n")

void trace_fmt2(FILE *fp, FILE *fp2, char const *fmt, ...);
void trace_close2(FILE *fp, FILE *fp2);
#define TRC2(p, p2, s, l, f, ...) trace_fmt2(p, p2, "%s%s.%-*d  " f, UTIL_TRACE_TIMER, (s), UTIL_TRACE_WIDTH, (l), __VA_ARGS__)
#define TRACE2(p, p2, s, l, f, ...) trace_fmt2(p, p2, "%s%s.%-*d  %s%s" f "%s", UTIL_TRACE_TIMER, (s), UTIL_TRACE_WIDTH, (l), __VA_ARGS__)
#define tracef2(p, p2, f, ...) TRACE2(p, p2, SRCNAME, __LINE__, f, "", "", ##__VA_ARGS__, "") // -nl
#define tracefEntry2(p, p2, f, ...) TRACE2(p, p2, SRCNAME, __LINE__, f, __FUNCTION__, "(", ##__VA_ARGS__, ")\n")
#define tracefExit2(p, p2, f, ...) TRACE2(p, p2, SRCNAME, __LINE__, f, __FUNCTION__, "() returns ", ##__VA_ARGS__, "\n")

// Define the following macros for simple tracing
//#define trace(...)      tracef(TRACE_FILE_POINTER, "" __VA_ARGS__)
//#define traceEntry(...) tracefEntry(TRACE_FILE_POINTER, "" __VA_ARGS__)
//#define traceExit(...)  tracefExit(TRACE_FILE_POINTER, "" __VA_ARGS__)
//#define closeTrace()    trace_close(TRACE_FILE_POINTER)

#endif
