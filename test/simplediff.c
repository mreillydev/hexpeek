// Copyright 2020 Michael Reilly (mreilly@mreilly.dev).
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#define BUFSZ 4096

void stripCR(ssize_t *cnt, char *buf)
{
    if(*cnt >= 2 && buf[*cnt - 2] == '\r' && buf[*cnt - 1] == '\n')
    {
        buf[*cnt - 2] = '\n';
        buf[*cnt - 1] = '\0';
        *cnt = (*cnt) - 1;
    }
}

int main(int argc, char **argv)
{
    int rc = 2, mode = 0;
    size_t sz0 = BUFSZ, sz1 = BUFSZ;
    char *buf0 = NULL, *buf1 = NULL;
    FILE *fp0 = NULL, *fp1 = NULL;

    if(argc < 3)
        goto end;
    if(strcmp(argv[1], "-binary") == 0 || strcmp(argv[1], "--") == 0)
    {
        mode = 0;
        argc--;
        argv++;
    }
    else if(strcmp(argv[1], "-text") == 0)
    {
        mode = 1;
        argc--;
        argv++;
    }

    if(argc < 3)
        goto end;
    if(strcmp(argv[1], "/dev/null") == 0 || strcmp(argv[2], "/dev/null") == 0)
        mode = 0;

    fp0 = fopen(argv[1], "r");
    if( ! fp0)
        goto end;

    fp1 = fopen(argv[2], "r");
    if( ! fp1)
        goto end;

    buf0 = malloc(sz0);
    if( ! buf0)
        goto end;

    buf1 = malloc(sz1);
    if( ! buf1)
        goto end;

    for(;;)
    {
        ssize_t n0 = 0, n1 = 0;
        if(mode == 1)
        {
            n0 = getline(&buf0, &sz0, fp0);
            if(n0 < 0 && errno)
                goto end;
            n1 = getline(&buf1, &sz1, fp1);
            if(n1 < 0 && errno)
                goto end;
            stripCR(&n0, buf0);
            stripCR(&n1, buf1);
        }
        else
        {
            n0 = fread(buf0, sizeof *buf0, sz0, fp0);
            n1 = fread(buf1, sizeof *buf1, sz1, fp1);
            if(ferror(fp0) || ferror(fp1))
                goto end;
        }
        if(n0 != n1)
        {
            rc = 1;
            goto end;
        }
        if(n0 <= 0)
        {
            rc = 0;
            goto end;
        }
        if(memcmp(buf0, buf1, n0))
        {
            rc = 1;
            goto end;
        }
    }

end:
    if(fp0)
        fclose(fp0);
    if(fp1)
        fclose(fp1);
    if(buf0)
        free(buf0);
    if(buf1)
        free(buf1);
    return rc;
}
