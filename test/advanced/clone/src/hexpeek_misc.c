// Copyright 2020, 2025 Michael Reilly (mreilly@mreilly.dev).
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

#define SRCNAME "hexpeek_misc.c"

#include <hexpeek.h>

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <limits.h>
#include <string.h>
#include <ctype.h>
#include <wchar.h>
#include <time.h>
#include <errno.h>

/**
 * @file hexpeek_misc.c
 * @brief Some string processing and other miscellaneous functions.
 */

/**
 * @brief Exit with the given return code, tracing the exit
 *
 * @param[in] result Return code to pass to exit()
 */
void terminate(int result)
{
    trace("exit(%d)\n", result);
    exit(result);
}

/**
 * @brief Underlying call for header error macros. Print the specified error
 *        message to stderr and trace file pointer.
 *
 * @param[in] file Name of the source file (usually passed by a macro)
 * @param[in] line Line of the source file (usually passed by a macro)
 * @param[in] op Error if 0, warning 1 (or otherwise currently)
 * @param[in] fmt Format string specifier for vfprintf() type functions
 * @param[in] ... vargs input for fmt
 */
void doErr(char const *file, int line, int op, char const *fmt, ...)
{
    va_list vl;
    va_start(vl, fmt);
    vfprintf(stderr, fmt, vl);
    va_end(vl);
    fflush(stderr);

    size_t len = strlen(fmt);
    char buf[len + 1];
    strncpy(buf, fmt, len + 1);
    buf[sizeof buf - 1] = '\0';
    if(len > 0 && buf[len - 1] == '\n')
        buf[len - 1] = '\0';
    TRC(Params.trace_fp, file, line, "%s reached: \"%s\"\n",
        (op == 0 ? "Error" : "Warning"), buf);
}

/**
 * @brief Underlying call for die() macro. Print an error message to stderr
 *        trace and the terminate with a critical error.
 *
 * @param[in] file Name of the source file (usually passed by a macro)
 * @param[in] line Line of the source file (usually passed by a macro)
 */
void doDie(char const *file, int line)
{
    doErr(file, line, 0, PGPRE "irrecoverable error encountered, aborting.\n");
    terminate(RC_CRIT);
}

/**
 * @brief Underlying call for assert() macro. Print a specific assertion message
 *        to stderr and then die (which logs to both stderr and trace).
 *
 * @param[in] file Name of the source file (usually passed by a macro)
 * @param[in] line Line of the source file (usually passed by a macro)
 * @param[in] msg String for assertion message
 * @param[in] exp Boolean expression (if false, assertion fails)
 * @return Always returns true (unless die() failed)
 */
bool doCheck(char const *file, int line, char const *msg, bool exp)
{
    if( ! exp)
    {
        fprintf(stderr, PGPRE "assertion '%s' failed at [%s:%d].\n",
                msg, file, line);
        doDie(file, line);
    }
    return exp;
}

/**
 * @brief Wrapper for malloc() that dies if malloc() fails.
 *
 * @param[in] sz Size of buffer to allocate
 * @return A void pointer representing the memory region allocated
 */
void *Malloc(size_t sz)
{
    void *ptr = malloc(sz);
    if( ! ptr)
    {
        prerr("error allocating memory: %s\n", strerror(errno));
        die();
    }
    memset(ptr, '\0', sz);
    return ptr;
}

/**
 * @brief Return if hexpeek is running in a mode where the user could be
 *        prompted (without determining whether we are in a terminal).
 */
bool promptable()
{
    return Params.recover_interactive ||
           ! (Params.recover_auto || Params.command || Params.do_pack);
}

/**
 * @brief Determine if the provided file descriptor references a terminal. If
 *        Params.assume_ttys is non-negative, then use that for the return code
 *        instead.
 *
 * @param[in] fd File descriptor to check for terminalness
 * @return A boolean whether fd references a terminal
 */
bool isterm(int fd)
{
    return (Params.assume_ttys < 0 ? isatty(fd) : (bool)Params.assume_ttys);
}

/**
 * @brief Return whether we are in interactive mode, which is true if hexpeek
 *        is not running in command, pack, or non-interactive recovery modes
 *        and if stdin and stdout are terminals.
 */
bool interactive()
{
    return promptable() && isterm(STDIN_FILENO) && isterm(STDOUT_FILENO);
}

/**
 * @brief snprintf() type function in which a pointer is used in place of
 *        an integral size as the boundary specification.\n 
 *        If repeatedly called like this:\n 
 *        \code{.c}
 *        buf += slprintf(buf, lim, ...)
 *        \endcode
 *        it is guaranteed that data will never written at or past lim because
 *        vsnprintf() returns at most lim - buf - 1 (its return code excludes
 *        trailing null), and buf + lim - buf - 1 == lim - 1.
 *
 * @param[out] buf Buffer to which to output data
 * @param[in] lim Pointer specifying boundary at or past which data will not
 *            be written
 * @param[in] format Format string for vsnprintf() style output
 * @param[in] ... Variadic input data
 * @return Number of characters printed (excluding null byte at end)
 */
size_t slprintf(char *buf, char *lim, char const *format, ...)
{
    assert(buf < lim);
    va_list vl;
    va_start(vl, format);
    int result = vsnprintf(buf, lim - buf, format, vl);
    va_end(vl);
    assert(result >= 0);
    return (size_t)result;
}

/**
 * @brief Convert a string to a hoff_t file offset for a given file - strtol()
 *        style function.
 *
 * @param[in] str String input data
 * @param[out] endptr If set non-NULL, address of first invalid character
 * @param[out] result hoff_t filled with file offset per the provided string
 *             (only set on success)
 * @param[in] infi Hexpeek file index for the file on which offset is to be
 *            calculated
 * @return RC_OK on success, RC_USER otherwise
 */
rc_t strtooff(char const *str, char const **endptr, hoff_t *result, int infi)
{
    rc_t rc = RC_USER;
    intmax_t tmpi = 0;
    char *endtmp = NULL;
    bool neg = false;

    assert(str);
    assert(result);

    stripLeadingSpaces(str);

    if(infi >= 0 && strncmp(str, FZ_LEN, strlen(FZ_LEN)) == 0)
    {
        if(endptr)
            *endptr = str + strlen(FZ_LEN);
        return strtooff("-0", NULL, result, infi);
    }

    if(*str == '-')
        neg = true;
    errno = 0;
    tmpi = strtoimax(str, &endtmp, Params.scalar_base);

    if(endptr)
        *endptr = endtmp;
    else if(*endtmp != '\0')
    {
        malnum("unexpected data after number\n");
        goto end;
    }
    
    if(str == endtmp)
        malnum("no numeric input found\n");
    else if(tmpi < -HOFF_MAX || (errno == ERANGE && tmpi == LLONG_MIN))
        malnum("offset subceeds minimum recognizable file offset\n");
    else if(tmpi > HOFF_MAX || (errno == ERANGE && tmpi == LLONG_MAX))
        malnum("offset exceeds maximum recognizable file offset\n");
    else
    {
        hoff_t tmpoff = (hoff_t)tmpi;
        if(neg || tmpoff < 0)
        {
            if(infi < 0)
            {
                malnum("negative length not valid\n");
                goto end;
            }
            if( ! isseekable(infi))
            {
                malnum("relative offset not valid on non-seekable file\n");
                goto end;
            }
            tmpoff += filesize(infi);
            if(tmpoff < 0)
            {
                malnum("bad negative file offset\n");
                goto end;
            }
        }
        rc = RC_OK;
        *result = tmpoff;
    }

end:
    return rc;
}

/**
 * @brief Convert a string to a non-negative hoff_t that can represent a buffer
 *        or file range size.
 *
 * @param[in] str String input data
 * @param[out] result hoff_t filled with non-negative integer per the provided
 *             string (only set on success)
 * @return RC_OK on success, RC_USER otherwise
 */
rc_t strtosz(char const *str, hoff_t *result)
{
    rc_t rc = RC_USER;
    intmax_t tmpi = 0;
    char *endtmp = NULL;

    assert(str);
    assert(result);

    errno = 0;
    tmpi = strtoimax(str, &endtmp, Params.scalar_base);

    if(str == endtmp)
        malnum("no numeric input found\n");
    else if(*endtmp != '\0')
        malnum("unexpected data after number\n");
    else if(tmpi < 0)
        malnum("negative input not valid\n");
    else if(tmpi > HOFF_MAX || errno == ERANGE)
        malnum("excessively large input\n");
    else
    {
        rc = RC_OK;
        *result = (hoff_t)tmpi;
    }

    return rc;
}

#define swap(a, b) (a) ^= (b); (b) ^= (a); (a) ^= (b);

/**
 * @brief Reverse byte order if -endianl was given and group is validly set.
 *
 * @param[in,out] buf Buffer containing binary data to be byte swapped
 * @param[in] len Length of buffer
 */
void endianize(uint8_t *buf, hoff_t len)
{
    if( ! Params.endian_big && DispGroup > 0 && DispGroup <= MAXW_GROUP)
    {
        for(;;)
        {
            hoff_t lim = MIN(len, DispGroup);
            if(lim < 2)
                break;
            for(hoff_t ix = 0; ix < lim / 2; ix++)
            {
                swap(buf[ix], buf[lim - 1 - ix]);
            }
            buf += lim;
            len -= lim;
        }
    }
}

static size_t skipDelims_f(char const *str, size_t prelen, size_t postlen)
{
    if(prelen && strncmp(str, GroupPre(1), prelen) == 0)
        return prelen;
    else if(postlen && strncmp(str, GroupTerm, postlen) == 0)
        return postlen;
    else if(iswhspace(*str))
        return 1;
    return 0;
}

static size_t skipDelims_s(char const *str, size_t unused0, size_t unused1)
{
    return str[0] == GroupPre(1)[0] ? 1 : 0;
}

/**
 * @brief Convert hexadecimal or binary textual string input into a binary
 *        octet buffer.
 *
 * @param[in] str String containing user text input
 * @param[in] mode MODE_HEX or MODE_BITS
 * @param[in,out] b_sz Size of available of space in buf, set to actual size
 *                of data outputted
 * @param[out] buf Output buffer for binary octet data of size *b_sz
 * @param[out] masks Output buffer for mask data used for very limited regex
 *             support in the search command. If provided, size must be same as
 *             buf.
 * @return RC_OK on success, a hexpeek error code otherwise.
 */
rc_t textToOctetArray(char const *str, int mode, hoff_t *b_sz,
                      uint8_t *buf, uint8_t *masks)
{
    rc_t rc = RC_UNSPEC;
    hoff_t b_ix = 0;
    size_t tmpz = -1;
    uint8_t tmp8 = 0;
    const uint8_t full = (mode == MODE_HEX ? 0xF : 1);
    const int distance = CHAR_BIT / MODE_CHCNT(mode);
    size_t prelen = 0, postlen = 0;
    size_t (*sanitize)(char const *, size_t, size_t) = NULL;

    if(GroupPre(1)[0] == ' ' && GroupPre(1)[1] == '\0' && GroupTerm[0] == '\0')
    {
        prelen = 1;
        postlen = 0;
        sanitize = skipDelims_s;
    }
    else
    {
        prelen = strlen(GroupPre(1));
        postlen = strlen(GroupTerm);
        sanitize = skipDelims_f;
    }

    assert(str);
    assert(b_sz);
    assert(buf);

    if(*b_sz < 1)
    {
        rc = RC_CRIT;
        goto end;
    }

    memset(buf, 0, (size_t)*b_sz);
    if(masks)
        memset(masks, 0xFF, (size_t)*b_sz);

    strnconsume(&str, GroupPre(0), strlen(GroupPre(0)));

    while(b_ix < *b_sz && *str != '\0')
    {
        // Silently skip all group delimiters and spaces
        tmpz = sanitize(str, prelen, postlen);
        if(tmpz > 0)
        {
            str += tmpz;
            continue;
        }

        // Convert text data
        for(int c_ix = MODE_CHCNT(mode) - 1; c_ix >= 0; c_ix--, str++)
        {
            tmp8 = CharLookup[(unsigned char)*str];
            if(tmp8 > full)
            {
                if(*str == '.' && masks)
                {
                    masks[b_ix] &= ~(full << (c_ix * distance));
                    continue;
                }
                else if(*str == '\0' || iswhspace(*str))
                {
                    if(b_ix == 0 && buf[b_ix]==0 && c_ix == MODE_CHCNT(mode)-2)
                    {
                        for( ; iswhspace(*str); str++);
                        if(*str == '\0')
                            break;
                    }
                    prerr("malformed input: octets not fully specified\n");
                }
                else
                {
                    prerr("malformed input: unrecognized character '%c'\n",
                          *str);
                }
                rc = RC_USER;
                goto end;
            }
            buf[b_ix] |= (tmp8 << (c_ix * distance));
        }
        b_ix++;
    }

    if(*str != '\0')
    {
        rc = RC_USER;
        prerr("malformed input: excessive length\n");
        goto end;
    }

    if(b_ix < 1)
    {
        rc = RC_USER;
        prerr("malformed input: no data\n");
        goto end;
    }

    endianize(buf, b_ix);
    if(masks)
        endianize(masks, b_ix);

    *b_sz = b_ix;

    rc = RC_OK;

end:
    if(rc)
        *b_sz = 0;
    return rc;
}

/**
 * @brief Cleanup control characters in an input string.
 *
 * @param[in] op Input string
 * @return malloc()'d output with control characters cleaned up
 */
char *_cleanstring(char const *op)
{
    assert(op);

    size_t ol = strlen(op);
    char *alloc = Malloc(4 * ol + 1), *cp = alloc;
    mbstate_t state;

    memset(&state, 0, sizeof state);

    while(ol > 0)
    {
        size_t consumed = mbrtowc(NULL, op, ol, &state);

        switch(consumed)
        {
        case (size_t)-2:
            free(alloc);
            return NULL;
        case (size_t)-1:
        case 0:
junk:
            switch(op[0])
            {
            case '\\': *(cp++) = '\\'; *(cp++) = '\\'; break;
            case  '"': *(cp++) = '\\'; *(cp++) =  '"'; break;
            case '\0': *(cp++) = '\\'; *(cp++) =  '0'; break;
            case '\a': *(cp++) = '\\'; *(cp++) =  'a'; break;
            case '\b': *(cp++) = '\\'; *(cp++) =  'b'; break;
            case '\t': *(cp++) = '\\'; *(cp++) =  't'; break;
            case '\n': *(cp++) = '\\'; *(cp++) =  'n'; break;
            case '\v': *(cp++) = '\\'; *(cp++) =  'v'; break;
            case '\f': *(cp++) = '\\'; *(cp++) =  'f'; break;
            case '\r': *(cp++) = '\\'; *(cp++) =  'r'; break;
            default:
                *(cp++) = '\\';
                *(cp++) = 'x';
                *(cp++) = BinLookup_hexu[(unsigned char)op[0]][0];
                *(cp++) = BinLookup_hexu[(unsigned char)op[0]][1];
                break;
            }
            consumed = 1;
            memset(&state, 0, sizeof state);
            break;
        case 1:
            if(op[0] == '\\' || op[0] == '"' || ! isprint((unsigned char)op[0]))
                goto junk;
            // FALLTHROUGH
        default:
            memcpy(cp, op, consumed);
            cp += consumed;
            break;
        }

        assert(consumed <= ol);
        ol -= consumed;
        op += consumed;
    }

    *cp = '\0';
    return alloc;
}

/**
 * @brief Cleanup control characters in an input string, storing the output
 *        in the global malloc()'d CleanString_mal variable.
 *
 * @param[in] original_path Input string (used for path names in current code)
 * @return CleanString_mal
 */
char *cleanstring(char const *original_path)
{
    if(CleanString_mal)
    {
        free(CleanString_mal);
        CleanString_mal = NULL;
    }
    CleanString_mal = _cleanstring(original_path);
    return CleanString_mal;
}

/**
 * @brief Display visible progress on hexpeek operations
 */
void outputProgress(hoff_t complete, hoff_t total, int isbackup)
{
    if(interactive() && total > 0x10 * BUFSZ)
    {
        static uint64_t lasttm = 0;
        if(complete < 0)
        {
            console("\r100%%complete%s", isbackup ? " (backup)" : "");
            console("\r                                                   \r");
            consoleFlush();
        }
        else
        {
            struct timespec ts;
            assert(clock_gettime(CLOCK_MONOTONIC, &ts) == 0);
            uint64_t curtm = ts.tv_sec*1000 + ts.tv_nsec/1000000;
            if(curtm > lasttm + 10)
            {
                lasttm = curtm;
                console("\r%5.1f%% complete%s",
                        MIN(((double)complete * 100) / (double)total, 100.0),
                        isbackup ? " (backup)" : "");
                consoleFlush();
            }
        }
    }
}
