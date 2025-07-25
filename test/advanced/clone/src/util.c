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

#define SRCNAME "util.c"

#include "util.h"

#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>

int iszero(const void *_ptr, size_t len)
{
    const uint8_t *ptr = (uint8_t*)_ptr;
    for(size_t i = 0; i < len; i++)
        if(ptr[i])
            return 0;
    return 1;
}

int powi(int base, int power)
{
    int rc = 1;
    for(int ix = 0; ix < power; ix++)
        rc *= base;
    return rc;
}

#define istrailspace(c) (iswhspace(c) || (c) == '\n')

void stripTrailingSpaces(char *line)
{
    if(line)
    {
        char *p;
        for(p = line + strlen(line) - 1; p >= line && istrailspace(*p); p--);
        p++;
        if(istrailspace(*p))
            *p = '\0';
    }
}

char* emplaceNewlines(char *_ptr)
{
    for(char *ptr = _ptr; ptr[0]; ptr++)
    {
        if(ptr[0] == '\\' && ptr[1] == 'n')
        {
            ptr[0] = '\n';
            for(int ix = 1; ptr[ix]; ix++)
                ptr[ix] = ptr[ix+1];
        }
    }
    return _ptr;
}

int strnconsume2(char const **a, char const *b, ssize_t len, int flags, char const *t)
{
   size_t l2 = 0;
    if(a == NULL || *a == NULL || b == NULL)
        return -1000;
    if(len < 0)
        len = strlen(b);
    if((flags&1) && isalnum((unsigned char)((*a)[len])))
        return -1001;
    int result = strncmp(*a, b, len);
    if(result == 0 && t)
        result = strncmp((*a)+len, t, (l2=strlen(t)));
    if(result == 0)
        *a += (len + l2);
    return result;
}

int strnconsume(char const **a, char const *b, ssize_t len)
{
    return strnconsume2(a, b, len, 0, NULL);
}

int strnconsumealnum(char const **a, char const *b, ssize_t len)
{
    return strnconsume2(a, b, len, 1, NULL);
}

int strnconsumetrail(char const **a, char const *b, char const *t)
{
    return strnconsume2(a, b, -1, 0, t);
}

bool memberofexnul(char ch, char const *list)
{
    if( ! list)
        return false;
    for( ; *list != '\0'; list++)
    {
        if(ch == *list)
            return true;
    }
    return false;
}

bool memberof(char ch, char const *list)
{
    if(ch == '\0')
        return true; // every list ends in '\0'
    else
        return memberofexnul(ch, list);
}

char const *strnchr(char const *str, int ch, size_t slen)
{
    char const *result = NULL;
    for(char const *ptr = str; ptr < str + slen && *ptr != '\0'; ptr++)
    {
        if(*ptr == ch)
        {
            result = ptr;
            break;
        }
    }
    return result;
}

int highbit(uint8_t octet)
{
    for(int ix = 7; ix >= 0; ix--)
    {
        if(octet & (1 << ix))
            return ix;
    }
    return -1;
}

int lowbit(uint8_t octet)
{
    for(int ix = 0; ix <= 7; ix++)
    {
        if(octet & (1 << ix))
            return ix;
    }
    return -1;
}

int countbit(uint8_t octet)
{
    int count = 0;
    for(uint8_t flag = 0x80; flag > 0; flag >>= 1)
    {
        if(octet & flag)
            count++;
    }
    return count;
}

void trace_fmt(FILE *fp, char const *fmt, ...)
{
    if(fp)
    {
        va_list vl;
        va_start(vl, fmt);
        vfprintf(fp, fmt, vl);
        va_end(vl);
        fflush(fp);
    }
}

void trace_close(FILE *fp)
{
    if(fp)
    {
        tracef(fp, "%s", "TRACE END");
        fflush(fp);
        fclose(fp);
        fp = NULL;
    }
}

void trace_fmt2(FILE *fp, FILE *fp2, char const *fmt, ...)
{
    if(fp)
    {
        va_list vl;
        va_start(vl, fmt);
        vfprintf(fp, fmt, vl);
        va_end(vl);
        fflush(fp);
    }
    if(fp2 && fp2 != fp)
    {
        va_list vl;
        va_start(vl, fmt);
        vfprintf(fp2, fmt, vl);
        va_end(vl);
        fflush(fp2);
    }
}

void trace_close2(FILE *fp, FILE *fp2)
{
    if(fp)
    {
        tracef(fp, "%s", "TRACE END");
        fflush(fp);
        fclose(fp);
    }
    if(fp2 && fp2 != fp)
    {
        tracef(fp2, "%s", "TRACE END");
        fflush(fp2);
        fclose(fp2);
    }
}
