// Copyright 2025 Michael Reilly (mreilly@mreilly.dev).
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
#include <stdarg.h>
#include <inttypes.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <assert.h>

#if INTMAX_MAX == INT64_MAX
    #define MAXMAX_DEC 20 // 18446744073709551615
#else
    #error
#endif

char const *DIFF_TOOL = "";
char const *DIFF_TEXT = "";
char const *DUMP_TOOL = "";

#define min(a, b) ((a) < (b) ? (a) : (b))
#define max(a, b) ((a) > (b) ? (a) : (b))

int shell(char const *format, ...)
{
    char cmdbuf[16384];
    memset(cmdbuf, '\0', sizeof cmdbuf);

    va_list vl;
    va_start(vl, format);
    int space = vsnprintf(cmdbuf, sizeof cmdbuf, format, vl);
    cmdbuf[sizeof cmdbuf - 1] = '\0';
    va_end(vl);

    if(space < 0 || space >= sizeof cmdbuf)
    {
        fprintf(stderr, "error forming shell argument, exiting\n");
        exit(2);
    }

    return system(cmdbuf);
}

#define run(f, ...) assert(shell(f, __VA_ARGS__) == 0)

char* dumpname(char const *basepath, int chunk)
{
    assert(basepath);
    assert(chunk >= 0 && chunk < 10);
    size_t len = strlen(basepath) + 1 + 4 + 1 + 1;
    char *result = malloc(len);
    assert(result);
    assert(snprintf(result, len, "%s.dump%d", basepath, chunk) > 0);
    result[len - 1] = '\0';
    return result;
}

void gendumpfrombuffer(char const *outfile, char *inbuf, intmax_t totlen)
{
    assert(totlen > 0);
    intmax_t srclen = strlen(inbuf);
    FILE *fp = fopen(outfile, "w");
    assert(fp);
    totlen *= 2; // 2 hex printed chars per octet
    while(totlen > 0)
    {
        if(totlen < srclen)
            inbuf[totlen] = '\0';
        assert(fprintf(fp, "%s", inbuf) > 0);
        totlen -= srclen;
    }
    fclose(fp);
}

void appendzeros(char const *outfile, intmax_t length)
{
    FILE *fp = fopen(outfile, "a");
    assert(fp);
    for(intmax_t index = 0; index < length; index++)
        fprintf(fp, "00");
    fclose(fp);
}

intmax_t filesize(char const *filename)
{
    struct stat info;
    assert(stat(filename, &info) == 0);
    return (intmax_t)info.st_size;
}

void gendumpfromfile(char const *outfile, char const *infile,
                     intmax_t srcat, intmax_t srclen, intmax_t totlen)
{
    intmax_t srcsz = 0;

    assert(srcat >= 0);
    assert(srclen >= -1);
    assert(totlen >= -1);

    unlink(outfile);
    run("touch %s", outfile);

    if(srclen == -1)
    {
        // to eof
        assert(srclen == totlen);
        run("%s -s 0x%jX %s > %s", DUMP_TOOL, srcat, infile, outfile);
    }
    else if(srcat + srclen > (srcsz = filesize(infile)))
    {
        // including hole
        assert(srclen == totlen);
        run("%s -s 0x%jX %s > %s", DUMP_TOOL, srcat, infile, outfile);
        appendzeros(outfile, srcat + srclen - srcsz);
    }
    else while(totlen > 0)
    {
        // regular case
        intmax_t cpylen = min(srclen, totlen);
        run("%s -s 0x%jX -l 0x%jX %s >> %s",
            DUMP_TOOL, srcat, cpylen, infile, outfile);
        totlen -= cpylen;
    }
}

int compare(char const *path0, char const *path1)
{
    int rc = 0;
    if(shell("%s %s %s", DIFF_TOOL, path0, path1))
    {
        rc = 1;
        fprintf(stderr, " ** Files '%s' and '%s' differ!\n", path0, path1);
        if(strlen(DIFF_TEXT) > 0)
            shell("%s %s %s", DIFF_TEXT, path0, path1);
    }
    return rc;
}

// <cleanup> <part> <cmd> <basepaths...>
int main(int argc, char **argv)
{
    const int paramcnt = 4;
    const int maxfiles = 2;

    assert(argc > paramcnt && argc <= paramcnt + maxfiles);

    char *endptr = NULL, *tmpval = NULL;

    tmpval = getenv("HEXPEEK_DIFFTEXT");
    if(tmpval)
        DIFF_TOOL = tmpval;
    tmpval = getenv("HEXPEEK_SHOWTEXT");
    if(tmpval)
        DIFF_TEXT = tmpval;
    tmpval = getenv("HEXPEEK_DUMPTOOL");
    if(tmpval)
        DUMP_TOOL = tmpval;

    if(strlen(DIFF_TOOL) < 1)
    {
        fprintf(stderr, "error: HEXPEEK_DIFFTEXT not set!\n");
        exit(2);
    }
    if(strlen(DUMP_TOOL) < 1)
    {
        fprintf(stderr, "error: HEXPEEK_DUMPTOOL not set!\n");
        exit(2);
    }

    long cleanup = strtol(argv[1], &endptr, 0);
    assert(*endptr == '\0');
    assert(cleanup >= 0 && cleanup <= 2);

    uintmax_t part = strtoumax(argv[2], &endptr, 0);
    assert(*endptr == '\0');
    assert(part >= 0);

    char *cmd = argv[3];
    assert(cmd);

    size_t maxpath = 0;
    for(int ix = 0; ix < maxfiles && paramcnt + ix < argc; ix++)
        maxpath = max(maxpath, strlen(argv[paramcnt + ix]));
    maxpath += 5 + MAXMAX_DEC + 1;

    char files[maxfiles][2][maxpath];
    memset(files, '\0', sizeof files);
    for(int ix = 0; ix < maxfiles && paramcnt + ix < argc; ix++)
    {
        if(cleanup == 2)
        {
            run("rm -f %s-part*", argv[paramcnt + ix]);
        }
        else
        {
            assert(access(argv[paramcnt + ix], R_OK) == 0);
            for(int jx = 1; jx >= 0 && part + jx >= 1; jx--)
            {
                assert(snprintf(files[ix][jx], maxpath, "%s-part%ju",
                                argv[paramcnt + ix], part + jx - 1) > 0);
                files[ix][jx][maxpath - 1] = '\0';
                if(jx == 1)
                {
                    assert(access(files[ix][jx], F_OK));
                    run("cp %s %s", argv[paramcnt + ix], files[ix][jx]);
                }
                else
                {
                    assert(access(files[ix][jx], F_OK) == 0);
                }
            }
        }
    }
    if(part == 0 || cleanup == 2)
        exit(0);

    // Very limited parser
    int srcfl = 0, dstfl = 0;
    intmax_t start = -1, dstlen = -1, srclen = -1, argat = -1;
    char op = '?';
    char *arglit = NULL;
    if(*cmd == '$')
    {
        cmd++;
        dstfl = strtol(cmd, &cmd, 0x10);
        assert(*cmd == '@');
        assert(dstfl >= 0 && dstfl < maxfiles);
    }
    if(*cmd == '@')
        cmd++;
    assert(isxdigit((unsigned char)*cmd));
    start = strtoimax(cmd, &cmd, 0x10);
    assert(start >= 0);
    if(*cmd == ',')
    {
        cmd++;
        dstlen = strtoimax(cmd, &cmd, 0x10);
    }
    else if(*cmd == ':')
    {
        cmd++;
        dstlen = strtoimax(cmd, &cmd, 0x10) - start;
    }
    op = *cmd;
    cmd++;
    assert(op == 'r' || op == 'i' || op == 'k');
    if(op == 'k')
    {
        assert(*cmd == '\0');
        assert(dstlen > 0);
    }
    else
    {
        arglit = cmd;
        if(*cmd == '$')
        {
            cmd++;
            srcfl = strtol(cmd, &cmd, 0x10);
            assert(*cmd == '@');
            assert(srcfl >= 0 && srcfl < maxfiles);
        }
        if(*cmd == '@')
        {
            cmd++;
            argat = strtoimax(cmd, &cmd, 0x10);
            assert(argat >= 0);
            if(*cmd == ',')
            {
                cmd++;
                srclen = strtoimax(cmd, &cmd, 0x10);
            }
            else if(*cmd == ':')
            {
                cmd++;
                srclen = strtoimax(cmd, &cmd, 0x10) - argat;
            }
            else
            {
                srclen = 1;
            }
            assert(*cmd == '\0');
        }
        else
        {
            assert(strlen(arglit) > 0);
            assert(strlen(arglit) % 2 == 0);
            srclen = strlen(arglit) / 2;
        }
        assert(srclen > 0);
        if(dstlen < 0)
            dstlen = srclen;
        else
            assert(srclen <= dstlen);
    }        

    // Comparison chart
    //----+-------+------------------------+------------------------+
    // op | chunk | before (srcfl/dstfl)   | after (dstfl)          |
    //----+-------+------------------------+------------------------+
    // r  |   1   | 0,start                | 0,start                |
    //    |   2   | arg data               | start,dstlen           |
    //    |   3   | start+dstlen:end       | start+dstlen:end       |
    //----+-------+------------------------+------------------------+
    // i  |   1   | 0,start                | 0,start                |
    //    |   2   | arg data               | start,dstlen           |
    //    |   3   | start:end              | start+dstlen:end       |
    //----+-------+------------------------+------------------------+
    // k  |   1   | 0,start                | 0,start                |
    //    |   2   | start,dstlen           | (nothing)              |
    //    |   3   | start+dstlen:end       | start:end              |
    //----+-------+------------------------+------------------------+
    for(int chunk = 1; chunk <= 3; chunk++)
    {
        char *path_bfr = files[chunk == 2 ? srcfl : dstfl][0];
        char *path_aft = files[dstfl][1];
        char *out_bfr = dumpname(path_bfr, chunk);
        char *out_aft = dumpname(path_aft, chunk);
        switch(chunk)
        {
        case 1:
            gendumpfromfile(out_bfr, path_bfr, 0, start, start);
            gendumpfromfile(out_aft, path_aft, 0, start, start);
            break;
        case 2:
            if(op == 'k')
            {
                continue;
            }
            else
            {
                if(argat >= 0)
                    gendumpfromfile(out_bfr, path_bfr, argat, srclen, dstlen);
                else
                    gendumpfrombuffer(out_bfr, arglit, dstlen);
                gendumpfromfile(out_aft, path_aft, start, dstlen, dstlen);
            }
            break;
        case 3:
            gendumpfromfile(out_bfr, path_bfr,
                            start + (op == 'i' ? 0 : dstlen), -1, -1);
            gendumpfromfile(out_aft, path_aft,
                            start + (op == 'k' ? 0 : dstlen), -1, -1);
            break;
        default:
            exit(2);
        }

        int fail = compare(out_bfr, out_aft);
        if(cleanup && fail == 0)
        {
            unlink(out_bfr);
            unlink(out_aft);
        }
        free(out_bfr);
        free(out_aft);
        if(fail)
            exit(fail);
    }

    if(cleanup)
    {
        for(int ix = 0; ix < maxfiles && paramcnt + ix < argc; ix++)
            unlink(files[ix][0]);
    }
    exit(0);
}
