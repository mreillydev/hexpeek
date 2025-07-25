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

#define SRCNAME "hexpeek.c"

#include <hexpeek.h>

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdarg.h>
#include <stdbool.h>
#include <libgen.h>
#include <inttypes.h>
#include <limits.h>
#include <fcntl.h>
#include <string.h>
#include <strings.h>
#include <ctype.h>
#include <errno.h>

/**
 * @file hexpeek.c
 * @brief Main program file.
 */

#ifdef HEXPEEK_PLUGINS
    #warning "Plugins enabled. Are you sure this is what you want?"
#endif

#define InferredFzErrString \
    "incompletely specified filezone (+pedantic allows)\n"
#define InferredDeleteLengthErrString \
    "excessive delete length (+pedantic allows)\n"
#define AmbiguousDiffString \
    "\"diff\" is ambiguous; try \"d i ff\" or \"~\""

// Early declarations
void printFiles(bool header);
rc_t closeFiles(rc_t rc);

/**
 * @brief Introductory output showing version, author, and possibly open files
 *
 * @param[in] files Boolean whether to display information about open files
 */
void introduce(bool files)
{
    if(interactive())
    {
        console("%s", VersionLong);
        console("%s", AuthorshipString);
        if(files)
            printFiles(true);
    }
}

/**
 * @brief Display usage information.
 *
 * @param[in] full Show long vs short usage string
 */
void usage(bool full)
{
    if(full)
        console("%s", UsageStringLong);
    else
        console("%s", UsageStringShort);
}

/**
 * @brief Return count of open infiles.
 */
int fileCount()
{
    int counter = 0;
    for(int fi = 0; fi < MAX_INFILES; fi++)
    {
        if(DT_FD(fi) >= 0)
            counter++;
    }
    return counter;
}

/**
 * @brief Return true for read commands that affect the file offset.
 *
 * @param[in] cmd Command index.
 */
bool streamableCommand(int cmd)
{
    switch(cmd)
    {
    case CMD_PRINT:
    case CMD_OFFSET:
    case CMD_SEARCH:
    case CMD_DIFF:
        return true;
    default:
        return false;
    }
}

/**
 * @brief Return true for write commands that affect the file offset.
 *
 * @param[in] cmd Command index.
 */
bool writeableCommand(int cmd)
{
    switch(cmd)
    {
    case CMD_REPLACE:
    case CMD_INSERT:
    case CMD_KILL:
        return true;
    default:
        return false;
    }
}

/**
 * @brief Return true for read or write commands that affect the file offest.
 *
 * @param[in] cmd Command index.
 */
bool seekableCommand(int cmd)
{
    return streamableCommand(cmd) || writeableCommand(cmd);
}

/**
 * @brief Return true if string begins with characters specifying a filezone.
 *
 * @param str User command string input.
 * @return Boolean if string begins with characters specifying a filezone.
 */
bool startOfFz(char const *str)
{
    return (*str == '-' || isxdigit((unsigned char)*str) ||
            strncmp(str, FZ_LEN, strlen(FZ_LEN)) == 0);
}

/**
 * @brief Convert string input into a FileZone structure representing a region
 *        of a hexpeek infile.
 *
 * @param[in] str User string input data.
 * @param[in] maxlevel Limit complexity of filezone that can be specified; if
 *            negative, a sensible default will be used.
 * @param[out] zone A FileZone representing a file region, initialized by this
 *             function.
 * @param[out] post Remaining string data after conversion.
 */
rc_t ascertainFileZone(char const *str, int maxlevel,
                       FileZone *zone, char const **post)
{
    rc_t rc = RC_UNSPEC;
    bool start_mandatory = false;

    FileZone_init(zone);

    if(maxlevel < 0)
        maxlevel = 2;

    for(int level = 0; level <= maxlevel; )
    {
        stripLeadingSpaces(str);

        switch(level)
        {
        case 0:
            if(*str == FZ_CTRL[0])
            {
                str++;
                char *endptr = NULL;
                long tmpl = -1;
                if( ! isxdigit((unsigned char)*str) ||
                   (tmpl = strtol(str, &endptr, Params.scalar_base)) < 0 ||
                   tmpl >= MAX_INFILES ||
                   DT_FD(tmpl) < 0 ||
                   str == endptr)
                {
                    rc = RC_USER;
                    malcmd("invalid file number\n");
                    goto end;
                }
                zone->fi = (int)tmpl;
                str = endptr;
                level = 1;
                break;
            }
            else if(Params.infer || fileCount() == 1)
            {
                zone->fi = 0;
            }
            else
            {
                rc = RC_USER;
                prohibcmd(InferredFzErrString);
                goto end;
            }
        case 1:
            if(*str == FZ_CTRL[1])
            {
                str++;
                start_mandatory = true;
            }
            if(startOfFz(str))
            {
                rc = strtooff(str, &str, &zone->start, zone->fi);
                if(rc)
                    goto end;
                level = 2;
                break;
            }
            else if(*str == FZ_CTRL[1])
            {
                str++;
                zone->start = DT_AT(zone->fi);
            }
            else if(start_mandatory)
            {
                rc = RC_USER;
                malcmd("invalid file offset after '%c'\n", FZ_CTRL[1]);
                goto end;
            }
            else if(Params.infer)
            {
                zone->start = DT_AT(zone->fi);
            }
            else
            {
                rc = RC_USER;
                prohibcmd(InferredFzErrString);
                goto end;
            }
        case 2:
            if(*str == FZ_CTRL[2])
            {
                str++;
                assert(zone->len < 0);
                rc = strtooff(str, &str, &zone->len, -1);
                if(rc)
                    goto end;
                level = 3;
                break;
            }
            if(*str == FZ_CTRL[3])
            {
                str++;
                hoff_t limit = 0;
                assert(zone->len < 0);
                if(strnconsume(&str, FZ_MAX, -1) == 0)
                {
                    zone->len = HOFF_MAX;
                    zone->tolerate_eof = true;
                }
                else
                {
                    rc = strtooff(str, &str, &limit, zone->fi);
                    if(rc)
                        goto end;
                    if(limit < zone->start)
                    {
                        if(Params.infer)
                            limit = zone->start;
                        else
                        {
                            rc = RC_USER;
                            malcmd("filezone limit less than offset\n");
                            goto end;
                        }
                    }
                    zone->len = limit;
                    if(zone->start >= 0)
                        zone->len -= zone->start;
                }
                level = 3;
                break;
            }
            if( ! Params.infer)
            {
                rc = RC_USER;
                prohibcmd(InferredFzErrString);
                goto end;
            }
        default:
            rc = RC_OK;
            goto end; // No match, stop
        }
    }

    rc = RC_OK;

end:
    if(rc == RC_OK && post)
        *post = str;
    return rc;
}

/**
 * @brief Convert user string input data into a ConvertedText structure
 *        representing a FileZone plus input binary data for file operations.
 *
 * @param[in] str User string input data
 * @param[in] memlim If negative, convertText() won't do EOF validation of the
 *            filezone, assuming that will be done later as applicable.
 * @param[in] maxlim Fail if memory input data or filezone is length is > maxlim
 * @param[in] deflen Default result filezone length to use if not specified
 * @param[in] masking Toggle allowing (limited) regex input.
 * @param[out] result ConvertedText structure in which to store output.
 * @return RC_OK on success, a hexpeek error code on failure
 */
rc_t convertText(char const *str, hoff_t memlim, hoff_t maxlim, hoff_t deflen,
                 bool masking, ConvertedText *result)
{
    rc_t rc = RC_UNSPEC;
    bool literal = false;
    size_t s_len = 0;

    assert(str);
    assert(result);
    assert(maxlim >= memlim);
    assert(maxlim >= 0);

    ConvertedText_init(result);

    stripLeadingSpaces(str);
    s_len = strlen(str);
    if(s_len <= 0)
    {
        rc = RC_USER;
        malcmd("empty argument\n");
        goto end;
    }

    if(memberofexnul(*str, FZ_PREF) || memlim < 0)
    {
        rc = ascertainFileZone(str, -1, &result->fz, &str);
        if(rc)
            goto end;
        if(*str != '\0')
        {
            rc = RC_USER;
            malcmd("unexpected text after filezone argument\n");
            goto end;
        }
        if(result->fz.start == HOFF_NIL)
            result->fz.start = 0;
        if(result->fz.len == HOFF_NIL)
        {
            result->fz.len = deflen;
            result->fz.tolerate_eof = true;
        }
        if(result->fz.len == 0)
        {
            // no-op
            rc = RC_OK;
            goto end;
        }
        if(memlim >= 0 &&
           filesize(result->fz.fi) - result->fz.start < result->fz.len)
        {
            rc = RC_USER;
            malcmd("cannot read beyond file length\n");
            goto end;
        }
        if(result->fz.len <= memlim)
            result->mem.count = result->fz.len;
        literal = false;
    }
    else
    {
        result->mem.count = maxOctetWidth(s_len);
        literal = true;
    }

    if(result->mem.count)
    {
        result->mem.sz = MAX(result->mem.count, BUFSZ);
        result->mem.octets_mal = malloc(result->mem.sz);
        if( ! result->mem.octets_mal)
        {
            if(literal)
            {
                rc = RC_USER;
                prerr("error allocating memory (excessive input length)\n");
            }
            else
            {
                rc = RC_CRIT;
                prerr("error allocating memory: %s\n", strerror(errno));
            }
            goto end;
        }
        memset(result->mem.octets_mal, 0, result->mem.sz);
        if(masking)
        {
            result->mem.masks_mal = malloc(result->mem.sz);
            if( ! result->mem.masks_mal)
            {
                if(literal)
                {
                    rc = RC_USER;
                    prerr("error allocating memory (excessive input length)\n");
                }
                else
                {
                    rc = RC_CRIT;
                    prerr("error allocating memory: %s\n", strerror(errno));
                }
                goto end;
            }
            memset(result->mem.masks_mal, 0xFF, result->mem.sz);
        }
        if(literal)
        {
            rc = textToOctetArray(str, DispMode, &result->mem.count,
                                  result->mem.octets_mal,
                                  result->mem.masks_mal);
        }
        else
        {
            hoff_t before_at = -1;
            SAVE_OFFSET(DT_FD(result->fz.fi), before_at);
            rc = readat(DT_FD(result->fz.fi), result->fz.start,
                        result->mem.octets_mal, result->mem.count);
            RESTORE_OFFSET(DT_FD(result->fz.fi), before_at);
        }
        checkrc(rc);
    }

    if(result->mem.count > maxlim || result->fz.len > maxlim)
    {
        rc = RC_USER;
        malcmd("excessive input length\n");
        goto end;
    }

    rc = RC_OK;

end:
    if(rc)
    {
        result->mem.sz = 0;
        result->mem.count = 0;
        if(result->mem.octets_mal)
        {
            free(result->mem.octets_mal);
            result->mem.octets_mal = NULL;
        }
        if(result->mem.masks_mal)
        {
            free(result->mem.masks_mal);
            result->mem.masks_mal = NULL;
        }
    }
    return rc;
}

/**
 * @brief Convert raw binary octet input data from 1 or 2 (for diffs) buffers
 *        into a human readable text output array.
 *
 * @param[in] in0 Binary input data array
 * @param[in] in1 If non-NULL, do a diff with in1 on the right
 * @param[in] len0 Length to use for in0
 * @param[in] len1 Length to use for in1
 * @param[out] out Array (up to 2 depth corresponding to in0 and in1) of text
 *             string output representing the binary input data
 * @param[in] lim Array (up to 2 depth corresponding to in0 and in1) of
 *            pointers for sanity checks on writing to out
 * @param[out] samep Set false if buffers differ or in1 is NULL, otherwise do
 *             no set (should be initialized true before first call to this
 *             function).
 */
void convertBinary_(uint8_t *in0, uint8_t *in1, hoff_t len0, hoff_t len1,
                    char **out, char **lim, bool *samep)
{
    uint8_t *in[] = {  in0,  in1 };
    hoff_t len[]  = { len0, len1 };
    hoff_t ix = 0;
    char const **datasrc =
        (DispMode == MODE_HEX ? (Params.hexlower ? BinLookup_hexl :
                                                   BinLookup_hexu)
                              : BinLookup_bits);

    assert(out[0] + DISP_CHCNT * len[0] + 1 <= lim[0]);

    if(in[1])
    {
        assert(out[1] + DISP_CHCNT * len[1] + 1 <= lim[1]);
        for(ix = 0; ix < MIN(len[0], len[1]); ix++)
        {
            if(in[0][ix] == in[1][ix])
            {
                memset(out[0], '_', DISP_CHCNT);
                memset(out[1], '_', DISP_CHCNT);
            }
            else
            {
                *samep = false;
                memcpy(out[0], datasrc[in[0][ix]], DISP_CHCNT);
                memcpy(out[1], datasrc[in[1][ix]], DISP_CHCNT);
            }
            out[0] += DISP_CHCNT;
            out[1] += DISP_CHCNT;
        }
        int which = (ix < len[0] ? 0 : 1);
        for( ; ix < len[which]; ix++)
        {
            *samep = false;
            memcpy(out[which], datasrc[in[which][ix]], DISP_CHCNT);
            out[which] += DISP_CHCNT;
        }
        if(len[0] > 0)
            *out[0] = '\0';
        if(len[1] > 0)
            *out[1] = '\0';
    }
    else
    {
        *samep = false;
        for(ix = 0; ix < len[0]; ix++)
        {
            memcpy(out[0], datasrc[in[0][ix]], DISP_CHCNT);
            out[0] += DISP_CHCNT;
        }
        *out[0] = '\0';
    }
}

/**
 * @brief Convert raw binary octet buffer data into a human readable text
 *        output array.
 *
 * @param[in] in0 Binary input data array
 * @param[in] len0 Length to use for in0
 * @param[out] out Textual output array representing the binary input data
 * @param[in] lim Pointer for sanity check assertions on writing to out
 * @param[out] samep Set false if buffers differ or in1 is NULL, otherwise do
 *             no set (should be initialized true before first call to this
 *             function).
 */
hoff_t convertBinary(uint8_t *in, hoff_t len, char *out, char *lim)
{
    char *start = out;
    bool unused = false;
    convertBinary_(in, NULL, len, 0, &out, &lim, &unused);
    return (hoff_t)(out - start);
}

/**
 * @brief Convert human readable text input data to a ParsedCommand structure.
 *
 * @param[in] cmdstr User text input string.
 * @param[in] full_validate If false, only do basic command determination
 * @param[out] ppr Pointer to a ParsedCommand structure, which is independently
 *             initialized by this function.
 * @return RC_OK on success, a hexpeek error code on failure
 */
rc_t ascertainCommand(char const *cmdstr, bool full_validate, ParsedCommand *ppr)
{
    rc_t rc = RC_UNSPEC;
    int filezone_given = 0;
    hoff_t defl = HOFF_NIL;
    bool check_spaces = true;

    assert(cmdstr);
    assert(ppr);

    traceEntry("'%s'", cmdstr);

    // Initial setup
    ParsedCommand_init(ppr);
    ppr->origcmd = cmdstr;
    stripLeadingSpaces(cmdstr);

    // Main command multiplexer
    ppr->cmd = ascertainShared(cmdstr, &ppr->subtype, &cmdstr);
    while(ppr->cmd == CMD_NONE)
    {
        stripLeadingSpaces(cmdstr);

        if(interactive() && ( ! Params.permissive ) &&
           strncmp(cmdstr, "diff", 4) == 0 &&
           (iswhspace(cmdstr[4]) || cmdstr[4] == '\0'))
        {
            rc = RC_USER;
            prohibcmd(AmbiguousDiffString "\n");
            goto end;
        }

        if(strnconsume(&cmdstr, "quit", 4) == 0)
            ppr->cmd = CMD_QUIT;
        else if(strnconsume(&cmdstr, "stop", 4) == 0)
            ppr->cmd = CMD_STOP;
        else if(strnconsume(&cmdstr, "help", 4) == 0)
            ppr->cmd = CMD_HELP;
        else if(strnconsume(&cmdstr, "files", 5) == 0)
            ppr->cmd = CMD_FILES;
        else if(strnconsume(&cmdstr, "reset", 5) == 0)
            ppr->cmd = CMD_RESET;
        else if(strnconsume(&cmdstr, "settings", 8) == 0)
            ppr->cmd = CMD_SETTINGS;
        else if(strnconsume(&cmdstr, "print", 5) == 0)
        {
            // Explicit print
            ppr->cmd = CMD_PRINT;
            ppr->print_off = true;
            ppr->print_verbose = (strnconsume(&cmdstr, "v", 1) == 0);
        }
        else if(strnconsume(&cmdstr, "offset", 6) == 0)
            ppr->cmd = CMD_OFFSET;
        else if(strnconsume(&cmdstr, "search", 6) == 0)
            ppr->cmd = CMD_SEARCH;
        else if(strnconsume(&cmdstr, "replace", 7) == 0)
            ppr->cmd = CMD_REPLACE;
        else if(strnconsume(&cmdstr, "insert", 6) == 0)
            ppr->cmd = CMD_INSERT;
        else if(strnconsume(&cmdstr, "kill", 4) == 0)
            ppr->cmd = CMD_KILL;
        else if(strnconsume(&cmdstr, "delete", 6) == 0)
            ppr->cmd = CMD_KILL;
        else if(strnconsume(&cmdstr, "ops", 3) == 0)
            ppr->cmd = CMD_OPS;
        else if(strnconsume(&cmdstr, "undo", 4) == 0)
            ppr->cmd = CMD_UNDO;
        else if(strnconsume(&cmdstr, "q", 1) == 0)
            ppr->cmd = CMD_QUIT;
        else if(strnconsume(&cmdstr, "h", 1) == 0)
            ppr->cmd = CMD_HELP;
        else if(strnconsume(&cmdstr, "p", 1) == 0)
        {
            // Explicit print
            ppr->cmd = CMD_PRINT;
            ppr->print_off = true;
            ppr->print_verbose = (strnconsume(&cmdstr, "v", 1) == 0);
        }
        else if(strnconsume(&cmdstr, "v", 1) == 0)
        {
            // Implicit print
            ppr->cmd = CMD_PRINT;
            ppr->print_verbose = true;
        }
        else if(strnconsume(&cmdstr, "/~", 2) == 0)
        {
            ppr->cmd = CMD_DIFF;
            ppr->diff_srch = true;
            check_spaces = false;
        } 
        else if(strnconsume(&cmdstr, "/", 1) == 0)
        {
            ppr->cmd = CMD_SEARCH;
            check_spaces = false;
        }
        else if(strnconsume(&cmdstr, "~", 1) == 0)
        {
            ppr->cmd = CMD_DIFF;
            check_spaces = false;
        }
        else if(strnconsume(&cmdstr, "r", 1) == 0)
        {
            ppr->cmd = CMD_REPLACE;
            check_spaces = false;
        }
        else if(strnconsume(&cmdstr, "i", 1) == 0)
        {
            ppr->cmd = CMD_INSERT;
            check_spaces = false;
        }
        else if(strnconsume(&cmdstr, "k", 1) == 0)
            ppr->cmd = CMD_KILL;
        else if(strnconsume(&cmdstr, "u", 1) == 0)
            ppr->cmd = CMD_UNDO;
        else if(strnconsume(&cmdstr, "+", 1) == 0)
        {
            if(filezone_given)
            {
                if(ppr->incr_post)
                {
                    rc = RC_USER;
                    malcmd("duplicate '+'\n");
                    goto end;
                }
                ppr->incr_post = true;
            }
            else
            {
                if(ppr->incr_pre)
                {
                    rc = RC_USER;
                    malcmd("duplicate '+'\n");
                    goto end;
                }
                ppr->incr_pre = true;
            }
        }
        else if(startOfFz(cmdstr) || memberofexnul(*cmdstr, FZ_CTRL))
        {
            if(filezone_given)
            {
                rc = RC_USER;
                malcmd("duplicate filezone input\n");
                goto end;
            }
            rc = ascertainFileZone(cmdstr, -1, &ppr->fz, &cmdstr);
            if(rc)
                goto end;
            if( ! memberof(*cmdstr, "+pvos/~rik "))
            {
                rc = RC_USER;
                malcmd("unexpected text after filezone input\n");
                goto end;
            }
            filezone_given = 1;
        }
        else if(filezone_given || ppr->incr_pre || ppr->incr_post ||
                ppr->fz.len >= 0)
        {
            // Implicit print
            ppr->cmd = CMD_PRINT;
        }
        else
        {
            rc = RC_USER;
            prerr("unrecognized command. Try 'help'.\n");
            goto end;
        }
    }

    // Check for write to eof commands
    if( ( ! Params.permissive ) && ppr->fz.len == HOFF_MAX &&
       (ppr->cmd == CMD_REPLACE || ppr->cmd == CMD_INSERT))
    {
        rc = RC_USER;
        malcmd("write to max is prohibited; try \"len\"\n");
        goto end;
    }

    // Automatically post-increment reads on non-seekable files
    if(streamableCommand(ppr->cmd) && ppr->fz.fi>=0 && ! isseekable(ppr->fz.fi))
        ppr->incr_post = true;

    // Return early if requested
    if( ! full_validate)
    {
        rc = RC_OK;
        goto end;
    }

    // Validate incrementation
    if( (ppr->incr_pre || ppr->incr_post) &&
        (ppr->cmd == CMD_KILL || ! seekableCommand(ppr->cmd)) )
    {
        rc = RC_USER;
        malcmd("invalid subcommand with '+'\n");
        goto end;
    }

    // Validate spaces before argument
    if(check_spaces)
    {
        switch(ppr->cmd)
        {
        // Commands which require space when optional argument is given
        case CMD_HELP:
        case CMD_RESET:
        case CMD_UNDO:
            if(*cmdstr != '\0' && ! iswhspace(*cmdstr))
            {
                rc = RC_USER;
                malcmd("space required before optional argument\n");
                goto end;
            }
            break;
        // Commands which require space before mandatory argument
        case CMD_RLEN:
        case CMD_SLEN:
        case CMD_LINE:
        case CMD_COLS:
        case CMD_GROUP:
        case CMD_MARGIN:
        case CMD_SCALAR:
        case CMD_SEARCH:
        case CMD_REPLACE:
        case CMD_INSERT:
            if( ! iswhspace(*cmdstr))
            {
                rc = RC_USER;
                malcmd("space required before mandatory argument\n");
                goto end;
            }
            break;
        // Commands which require no trailing text
        default:
            if(*cmdstr != '\0')
            {
                rc = RC_USER;
                malcmd("trailing text\n");
                goto end;
            }
            break;
        }
    }

    // Validate seekable command
    if(seekableCommand(ppr->cmd))
    {
        if(ppr->fz.fi < 0)
        {
            if(Params.infer || fileCount() == 1)
                ppr->fz.fi = 0;
            else
            {
                rc = RC_USER;
                prohibcmd(InferredFzErrString);
                goto end;
            }
        }
        if(ppr->fz.start == HOFF_NIL)
        {
            if(Params.infer)
                ppr->fz.start = DT_AT(ppr->fz.fi);
            else
            {
                rc = RC_USER;
                prohibcmd(InferredFzErrString);
                goto end;
            }
        }
        if(ppr->fz.start == HOFF_NIL)
        {
            ppr->incr_pre = false;
            ppr->fz.start = 0;
        }
    }
    else if(filezone_given)
    {
        rc = RC_USER;
        malcmd("invalid subcommand after Hexoff\n");
        goto end;
    }

    // Cleanup and record argument string
    stripLeadingSpaces(cmdstr);
    ppr->arg_t = cmdstr;

    // Convert argument
    switch(ppr->cmd)
    {
    case CMD_SEARCH:
        rc = convertText(ppr->arg_t, SRCHSZ, SRCHSZ, 1, true, &ppr->arg_cv);
        if(rc)
            goto end;
        break;
    case CMD_DIFF:
        if(ppr->fz.len != HOFF_NIL)
            defl = ppr->fz.len;
        else if(ppr->diff_srch)
            defl = HOFF_MAX;
        else
            defl = DispPrDef;
        if(*ppr->arg_t == '\0' && fileCount() == 2)
        {
            if( ! Params.infer)
            {
                rc = RC_USER;
                prohibcmd(InferredFzErrString);
                goto end;
            }
            ppr->fz.len = defl;
            ppr->arg_cv.fz.fi = FILE_INDEX_LATER;
            break;
        }
        rc = convertText(ppr->arg_t, -1, HOFF_MAX, defl, false, &ppr->arg_cv);
        if(rc)
            goto end;
        break;
    case CMD_REPLACE:
    case CMD_INSERT:
        rc = convertText(ppr->arg_t,
#ifdef HEXPEEK_ALWAYS_FILECPY
                         0,
#else
                         BUFSZ,
#endif
                         HOFF_MAX, 1, false, &ppr->arg_cv);
        if(rc)
            goto end;
        break;
    }

    // Set default filezone length
    if(ppr->fz.len == HOFF_NIL)
    {
        switch(ppr->cmd)
        {
        case CMD_PRINT:
            ppr->fz.len = DispPrDef;
            break;
        case CMD_OFFSET:
        case CMD_KILL:
            ppr->fz.len = 1;
            break;
        case CMD_SEARCH:
            ppr->fz.len = HOFF_MAX;
            break;
        case CMD_DIFF:
            ppr->fz.len = ppr->arg_cv.fz.len;
            break;
        case CMD_REPLACE:
        case CMD_INSERT:
            ppr->fz.len = (ppr->arg_cv.mem.count > 0 ? ppr->arg_cv.mem.count :
                                                       ppr->arg_cv.fz.len);
            break;
        }
        ppr->fz.tolerate_eof = true;
    }

    rc = RC_OK;

end:
    if(rc)
        ParsedCommand_init(ppr);
    traceExit(TRC_rc, rc);
    return rc;
}

/**
 * @brief Print help text.
 *
 * @param[in] topic Query string for a help text topic. Can be a hexpeek
 *            command or some special reserved strings like -all, diff,
 *            Numeric, etc.
 */
void help(char const *topic)
{
    int idx = CMD_NONE;
    if(topic && *topic != '\0')
    {
        if(strcasecmp(topic, "-all") == 0)
        {
            for(idx = CMD_MIN; idx <= CMD_MAX; idx++)
                console("%s%s", HelpText[idx], idx < CMD_MAX ? "\n" : "");
        }
        else if(strcasecmp(topic, "diff") == 0)
        {
            console(AmbiguousDiffString "\n");
        }
        else
        {
            if(strncmp(topic, "endian", 6) == 0)
                idx = CMD_ENDIAN;
            else if(strncasecmp(topic, "Numeric", 7) == 0 ||
                    strncasecmp(topic, "Filezone", 8) == 0 ||
                    strncasecmp(topic, "HEXOFF", 6) == 0 ||
                    strncasecmp(topic, "HEXLEN", 6) == 0 ||
                    strncasecmp(topic, "HEXLIM", 6) == 0 ||
                    (memberofexnul(topic[0], FZ_CTRL) && topic[1] == '\0'))
                idx = CMD_NUMERIC;
            else
            {
                ParsedCommand pc;
                rc_t rc = ascertainCommand(topic, false, &pc);
                if(rc)
                    idx = CMD_NONE;
                else if(pc.cmd == CMD_PRINT && ! (pc.print_off ||
                                                  pc.print_verbose))
                    idx = CMD_NUMERIC;
                else
                    idx = pc.cmd;
            }

            if(idx > CMD_NONE && idx <= CMD_MAX)
                console("%s", HelpText[idx]);
        }
    }
    else
    {
        console("%s", HelpCmdList);
    }
}

/**
 * @brief Generate a nicely formatted string representing a file name given
 *        a hexpeek file index and backup index. Store the results in an
 *        appropriate part of Params global variable.
 *
 * @param[in] fi Hexpeek infile file index [0, MAX_INFILES).
 * @param[in] bidx Backup index, or negative if this is not a backup file.
 */
void genFormattedFileName(int fi, int bidx)
{
    size_t length = 1; // '\0'
    int printed = 0;
    char *prefix = "";
    char **useptr = NULL;
    char const *usepath = NULL;
    int usefd = -1;

    assert(fi >= 0 && fi < MAX_INFILES);

    if(bidx < 0)
    {
        useptr  = &Params.infiles[fi].name_mal;
        usepath = Params.infiles[fi].path;
        usefd   = Params.infiles[fi].fd;
    }
    else
    {
        prefix  = "backup ";
        useptr  = &Params.infiles[fi].bk_name_mal[bidx];
        usepath = Params.infiles[fi].bk_path_mal[bidx];
        usefd   = Params.infiles[fi].bk_fds[bidx];
    }

    length += strlen(prefix);

    if(usepath)
    {
        usepath = cleanstring(usepath);
        length += 4 + 1 + 1 + strlen(usepath) + 1;
        *useptr = Malloc(length);
        printed = snprintf(*useptr, length, "%sfile \"%s\"", prefix, usepath);
    }
    else
    {
        length += 10 + 1 + MAX_DEC;
        *useptr = Malloc(length);
        printed = snprintf(*useptr, length, "%sdescriptor %d", prefix, usefd);
    }

    (*useptr)[length - 1] = '\0';
    assert(printed > 0 && printed < length);
}

#define BACKUP_EXT_MAX(s) (1 + ((s) ? strlen((s)) : MAX_DEC+1+MAX_DEC) + \
                           2 + MAX_DEC + 1 + strlen(BACKUP_EXT) + 1)
#define BACKUP_EXT_FILE   ".%s"           ".f%d." BACKUP_EXT
#define BACKUP_EXT_FD     ".%" PRIdMAX "-%d.d%d." BACKUP_EXT

/**
 * @brief Generate a path name for a backup file to be created.
 *
 * @param[in] dname Path of infile to which this backup will correspond.
 * @param[in] dfd File descriptor of corresponding infile.
 * @param[in] bidx Backup index in domain [0, BACKUP_FILE_COUNT).
 * @return Malloc-d buffer of path text.
 */
char *genBackupName(char const *dname, int dfd, int bidx)
{
    int printed = 0;
    size_t totlen = BACKUP_EXT_MAX(dname), avail = 0;
    char *tot_mal = NULL;

    tot_mal = Malloc(totlen);

    if(dname)
    {
        strncpy(tot_mal, dname, totlen);

        size_t namelen = strlen(dname);
        char basebuf[namelen + 1], bname[namelen + 3];
        strncpy(basebuf, dname, sizeof basebuf);
        basebuf[sizeof basebuf - 1] = '\0';
        strncpy(bname, basename(basebuf), sizeof bname);
        bname[sizeof bname - 1] = '\0';

        char *at = tot_mal + namelen - strlen(bname);
        assert(at >= tot_mal);

        avail = totlen + at - tot_mal;
        printed = snprintf(at, totlen + avail, BACKUP_EXT_FILE, bname, bidx);
    }
    else
    {
        avail = totlen;
        printed = snprintf(tot_mal, avail, BACKUP_EXT_FD, (intmax_t)getppid(),
                           dfd, bidx);
    }

    tot_mal[totlen - 1] = '\0';
    assert(printed > 0 && printed < avail);
    return tot_mal;
}

/**
 * @brief Open backup files for a given infile file index.
 *
 * @param[in] df Infile file index
 * @param[in] flags Flags to pass to hexpeek_open(), e.g. O_CREAT
 * @return RC_OK on success, a hexpeek error code on failure
 */
rc_t openBackupFiles(int df, int flags)
{
    rc_t rc = RC_UNSPEC;

    for(int bidx = 0; bidx < BACKUP_FILE_COUNT; bidx++)
    {
        // Get the backup file name
        char *bpath = genBackupName(DT_PATH(df), DT_FD(df), bidx);

        // If read-only, only check for existence 
        if(flags == 0)
        {
            if(pathsize(bpath) > 0)
            {
                prerr("warning: backup file \"%s\" already exists\n",
                      cleanstring(bpath));
            }
            free(bpath);
            bpath = NULL;
            continue;
        }

        // Backup code requires pre-reading the file
        if( ! isseekable(df))
        {
            rc = RC_CRIT;
            prerr("cannot backup non-seekable %s\n", DT_NAME(df));
            goto end;
        }

        // Save the backup file name
        BK_PATH(df, bidx) = bpath;
        genFormattedFileName(df, bidx);

        // Creat and/or open backup file
        if(flags & O_CREAT)
        {
            switch(pathsize(BK_PATH(df, bidx)))
            {
            case -1:
                break;
            case 0:
                unlink(BK_PATH(df, bidx));
                break;
            default:
                rc = RC_CRIT;
                prerr("%s already exists; either run '" PRGNM " -recover'"
                      " or delete it\n", BK_NAME(df, bidx));
                goto end;
            }

            errno = 0;
            rc = hexpeek_open(BK_PATH(df, bidx), flags, PERM, &BK_FD(df, bidx));
            if(rc)
                goto end;

            if(Params.backup_sync)
            {
                rc = hexpeek_sync(BK_FD(df, bidx));
                if(rc)
                    goto end;

                rc = hexpeek_syncdir(BK_PATH(df, bidx));
                if(rc)
                    goto end;
            }
        }
        else if(flags & O_RDWR)
        {
            rc = hexpeek_open(BK_PATH(df, bidx), flags, 0, &BK_FD(df, bidx));
            if(rc)
            {
                if(Params.permissive)
                {
                    if(consoleAsk("Proceed with recovery"))
                    {
                        rc = RC_DONE;
                        goto end;
                    }
                    else
                    {
                        rc = RC_UNSPEC;
                    }
                }
                else
                {
                    goto end;
                }
            }
        }

        // Trace it
        trace("ix=[%d,%d], path='%s', fd=%d\n", df, bidx,
              BK_PATH(df, bidx), BK_FD(df, bidx));
    }

    rc = RC_OK;

end:
    return rc;
}

#define CREAT_FLAGS (O_RDWR|O_CREAT|O_EXCL)
#define OPEN_FILES_NORMAL   0
#define OPEN_FILES_SKIP_BAK 1

/**
 * @brief Open the infiles and corresponding backup files per params.
 *
 * @param[in] flags If 1, don't try to open backup files.
 * @return RC_OK on success, a hexpeek error code on failure
 */
rc_t openFiles(int flags)
{
    rc_t rc = RC_UNSPEC;
    int writeMode = 0;
    struct stat info;
    char const *ftype = NULL;

    traceEntry();

    // Loop through all possible infiles
    for(int fi = 0; fi < MAX_INFILES; fi++)
    {
        // Nice name for user output
        genFormattedFileName(fi, -1);

        // Set flags
        int o_flags = Params.infiles[fi].open_flags;
        int b_flags = 0;
        if(Params.recover_interactive || Params.recover_auto)
        {
            b_flags = O_RDWR;
        }
        else if(o_flags & O_RDWR)
        {
            b_flags = (BackupDepth > 0 ? CREAT_FLAGS : 0);
            writeMode = 1;
        }

        // If necessary, open the file
        if(DT_PATH(fi))
        {
            bool existed = (access(DT_PATH(fi), F_OK) == 0);
            rc = hexpeek_open(DT_PATH(fi), o_flags, PERM, &DT_FD(fi));
            if(rc)
                goto end;
            Params.infiles[fi].created = ((o_flags & O_CREAT) && ! existed);
        }
        else if(DT_FD(fi) < 0)
        {
            continue;
        }

        // Stat it
        if(hexpeek_stat(DT_FD(fi), &info))
        {
            rc = RC_CRIT;
            goto end;
        }
        switch(info.st_mode & S_IFMT)
        {
        case S_IFREG:  ftype = "regular file"; break;
        case S_IFDIR:  ftype = "directory"; break;
        case S_IFIFO:  ftype = "fifo"; break;
        case S_IFBLK:  ftype = "block device"; break;
        case S_IFCHR:  ftype = "character device"; break;
        case S_IFLNK:  ftype = "symlink"; break;
        case S_IFSOCK: ftype = "socket"; break;
        default:       ftype = "unknown"; break;
        }

        // Trace it
        trace("ix=%d, path='%s', fd=%d, st_dev=%" PRIdMAX ", st_ino=%" PRIdMAX
              ", st_rdev=%" PRIdMAX ", type='%s'\n",
              fi,
              DT_PATH(fi) ? DT_PATH(fi) : "", // real name
              DT_FD(fi),
              (intmax_t)info.st_dev,
              (intmax_t)info.st_ino,
              (intmax_t)info.st_rdev,
              ftype);

        // Check uniqueness amongst infiles
#ifdef HEXPEEK_UNIQUE_INFILES
        if( ! Params.assume_unique_infiles)
        {
            for(int otherfile = 0; otherfile < fi; otherfile++)
            {
                if(sameness(DT_FD(fi), DT_FD(otherfile)))
                {
                    rc = RC_CRIT;
                    prerr("cannot use %s as infile, appears to be the same as"
                          " %s (rerun with -unique to skip this check)\n",
                            DT_NAME(fi), DT_NAME(otherfile));
                    DT_FD(fi) = -1; // don't close twice
                    goto end;
                }
            }
        }
#endif

        // Open backup files
        if( ! (flags & OPEN_FILES_SKIP_BAK) )
        {
            rc = openBackupFiles(fi, b_flags);
            if(rc)
            {
                if(interactive() && writeMode)
                {
                    // Try to fallback to opening infiles read-only
                    BackupUnlinkAllowed = false;

                    rc = closeFiles(RC_OK);
                    if(rc)
                        goto end;

                    for(int fi = 0; fi < MAX_INFILES; fi++)
                    {
                        Params.infiles[fi].open_flags &= ~CREAT_FLAGS;
                        Params.infiles[fi].open_flags |= O_RDONLY;
                    }

                    rc = openFiles(OPEN_FILES_SKIP_BAK);
                    if(rc == RC_OK)
                    {
                        prerr("Infiles opened read-only (could not create backup files for write ops)\n");
                    }
                }
                goto end;
            }
        }
    }

    rc = RC_OK;

end:
    traceExit(TRC_rc, rc);
    return rc;
}

/**
 * @brief Close all open files.
 *
 * @param[in] rc What the return code would have been before calling this
 *            function. Must be <= RC_DIFF for unlink of backup files to occur.
 * @return rc; unless a close() fails, in which case return RC_CRIT
 */
rc_t closeFiles(rc_t rc)
{
    for(int fi = 0; fi < MAX_INFILES; fi++)
    {
        if(DT_FD(fi) >= 0 && close(DT_FD(fi)))
        {
            if(rc <= RC_DIFF)
                rc = RC_CRIT;
            prerr("error closing data infile: %s\n", strerror(errno));
        }
        DT_FD(fi) = -1;
        for(int bidx = 0; bidx < BACKUP_FILE_COUNT; bidx++)
        {
            if(BK_FD(fi, bidx) >= 0 && close(BK_FD(fi, bidx)))
            {
                if(rc <= RC_DIFF)
                    rc = RC_CRIT;
                prerr("error closing backup file: %s\n", strerror(errno));
            }
            BK_FD(fi, bidx) = -1;
            if(BK_PATH(fi, bidx))
            {
                if(rc <= RC_DIFF && BackupUnlinkAllowed)
                   unlink(BK_PATH(fi, bidx));
                free(BK_PATH(fi, bidx));
                BK_PATH(fi, bidx) = NULL;
            }
        }
        struct stat info;
        if(rc <= RC_DIFF && Params.infiles[fi].created && DT_PATH(fi) &&
           stat(DT_PATH(fi), &info) == 0 && info.st_size == 0)
        {
            unlink(DT_PATH(fi));
            DT_PATH(fi) = NULL;
        }
    }

    return rc;
}

/**
 * @brief Display informational output about open hexpeek infiles.
 *
 * @param[in] header Boolean whether to show a quick header.
 */
void printFiles(bool header)
{
    struct stat info;

    if(header)
        console("Open Files:\n");

    for(int fi = 0; fi < MAX_INFILES; fi++)
    {
        if(DT_FD(fi) < 0)
            continue;

        assert(hexpeek_stat(DT_FD(fi), &info) == RC_OK);

        console("%c%d | %s, %s, ", FZ_PREF[0], fi, DT_NAME(fi),
                (DT_MODE(fi) & O_RDWR) ? "writeable" : "read-only");

        if((info.st_mode & S_IFMT) == S_IFREG)
            consoleOutf(PRI_hoff " octet%s", prihcnt(info.st_size));
        else
            console("size unknown");

        console(", current offset ");
        if(DT_AT(fi) == HOFF_NIL)
            console("unset\n");
        else
            consoleOutf("%c" PRI_hoff"\n", FZ_PREF[1], prihoff(DT_AT(fi)));
    }

    consoleFlush();
}

#define hexOn() (DispMode == MODE_HEX)

#define dispName() (hexOn() ? "hexadecimal" : "bits")

/**
 * @brief Display informational about current settings.
 */
void printSettings()
{
    console("Display Mode          |  %s%s\n",
            dispName(),
            hexOn() ? (Params.hexlower ? " (lower)" : " (upper)") : "");
    console("Input Mode            |  %s\n", dispName());
    console("Endianness            |  %s\n",
            Params.endian_big ? "big" : "little");
    consoleOutf("Default print length  |  " PRI_hoff " octet%s\n",
                prihcnt(DispPrDef));
    consoleOutf("Search output length  |  " PRI_hoff " octet%s\n",
                prihcnt(DispSrchDef));
    console("Line width            |  ");
    if(DispLine)
        consoleOutf(PRI_hoff " octet%s\n", prihcnt(DispLine));
    else
        console("(all)\n");
    console("Group width           |  ");
    if(DispGroup)
        consoleOutf(PRI_hoff " octet%s\n", prihcnt(DispGroup));
    else
        console("(disabled)\n");        
}

/**
 * @brief Determine if index is a group start boundary and if so, print group
 *        pre delimiter or other specified character.
 *
 * @param[in] index File offset index
 * @param[in] toprint If non-NUL, char to print; else use group pre delimiter
 * @return Boolean if index is a group start boundary
 */
bool groupStart(hoff_t index, char toprint)
{
    hoff_t li = mod(index, DispLine);
    if(mod(li, DispGroup) == 0)
    {
        if(toprint == '\0')
            console("%s", GroupPre(li));
        else
        {
            for(size_t idx = 0; idx < strlen(GroupPre(li)); idx++)
                console("%c", toprint);
        }
        return true;
    }
    return false;
}

/**
 * @brief Determine if index is a group end boundary and, if so, print group
 *        post delimiter or other specified character. This function
 *        intentionally does not output a group post delimiter for an
 *        incomplete group because it would be incorrect to signal (with a
 *        delimiter) the end of an incomplete group.
 *
 * @param[in] index File offset index
 * @param[in] toprint If non-NUL, char to print; else use group post delimiter
 * @return Boolean if index is a group end boundary
 */
bool groupEnd(hoff_t index, char toprint)
{
    if(DispGroup == 0)
        return false;
    if((mod(index, DispLine) + 1) % DispGroup == 0)
    {
        if(toprint == '\0')
            console("%s", GroupTerm);
        else
        {
            for(size_t idx = 0; idx < strlen(GroupTerm); idx++)
                console("%c", toprint);
        }
        return true;
    }
    return false;
}

/**
 * @brief Determine if index a group start OR end boundary and, if so, print
 *        appropriate delimiters.
 *
 * @param[in] index File offset index
 * @param[in] toprint If non-NUL, char to print; else use default delimiters
 */
void groupPadding(hoff_t index, char toprint)
{
    groupStart(index, toprint);
    groupEnd(index, toprint);
}

/**
 * @brief Print ruler border (of '-' and '|' characters).
 *
 * @param[in] layer Zero for pre-ruler border, non-zero otherwise
 * @param[in] until How wide the border should be printed
 */
void printRulerBorder(int layer, int until)
{
    if(Params.margin)
    {
        for(size_t idx = 0; idx < Params.margin + strlen(MarginPost); idx++)
            console("-");
    }
    for(int idx = 0; idx < until; idx++)
    {
        groupPadding(idx, '-');
        for(int rep = 0; rep < DISP_CHCNT - 1; rep++)
            console("-");
        console(layer == 0 ? "-" : "|");
    }
    console("\n");
}

/**
 * @brief Display informational output regarding depth of lines.
 */
void printRuler()
{
    int until = MIN(DispLine, 0x100);
    if(until < 1)
        until = 0x100;

    printRulerBorder(0, until);

    if(Params.margin)
    {
        int idx = 0;
        int lim = Params.margin + strlen(MarginPost);
        if(lim >= 5)
        {
            console("Ruler");
            idx = 5;
        }
        for( ; idx < lim; idx++)
            console(" ");
    }

    uint8_t mks[until];
    for(int idx = 0; idx < until; idx++)
        mks[idx] = idx;
    endianize(mks, until);

    for(int idx = 0; idx < until; idx++)
    {
        groupPadding(idx, ' ');
        if(DispMode == MODE_HEX && (mks[idx] % 2 == 0 || DispGroup == 1))
            consoleOutf("%0*" PRI_hxpkint, DISP_CHCNT, mks[idx]);
        else if(DispMode == MODE_BITS)
            consoleOutf("%*" PRI_hxpkint, DISP_CHCNT, mks[idx]);
        else
        {
            for(int rep = 0; rep < DISP_CHCNT; rep++)
                console(" ");
        }
    }
    console("\n");

    printRulerBorder(1, until);
}

/**
 * @brief Return ASCII character representing integers 0 - 8.
 */
char bitinfo(int binfo)
{
    switch(binfo)
    {
    case  0: return '0';
    case  1: return '1';
    case  2: return '2';
    case  3: return '3';
    case  4: return '4';
    case  5: return '5';
    case  6: return '6';
    case  7: return '7';
    case  8: return '8';
    default: return '.';
    }
}

/**
 * @brief Return a margin file offset given any file offset.
 *
 * @param[in] address File offset.
 * @return Corresponding margin file offset to show on left.
 */
uintmax_t mgAddr(hoff_t address)
{
    assert(address >= 0);
    assert(Params.margin >= 0);
    uintmax_t result = (uintmax_t)address;
    if(Params.margin < sizeof Masks / sizeof Masks[0])
        result &= Masks[Params.margin];
    return result;
}

/**
 * @brief Convert a textual hexpeek dump into a binary output file descriptor.
 *
 * @param[in] inidx File index of a hexpeek infile with textual dump output
 * @param[in] outfd File descriptor to which to write binary output
 */
rc_t pack(int inidx, int outfd)
{
    rc_t rc = RC_UNSPEC;
    FILE *infp = NULL;
    char *str_mal = NULL;
    size_t str_sz = 0;
    intmax_t loop = -1;
    uint8_t *octets_mal = NULL;
    hoff_t out_at = HOFF_NIL, octets_sz = 0, last_sz = 0;
    bool skip_pending = false;

    if(Params.margin > 0 && Params.margin < HOFF_HEX_FULL_WIDTH)
        prwarn("packing file with non-full margin\n");

    infp = fdopen(DT_FD(inidx), "r");
    if( ! infp)
    {
        rc = RC_CRIT;
        prerr("error opening %s as stream: %s\n",
              DT_NAME(inidx), strerror(errno));
        goto end;
        
    }

    for(loop = 1; ; loop++)
    {
        ssize_t loopsz = -1;
        char *str = NULL;

        errno = 0;
        loopsz = getline(&str_mal, &str_sz, infp);
        if(loopsz <= 0)
        {
            if(errno)
            {
                rc = RC_CRIT;
                prerr("error reading from %s: %s\n",
                      DT_NAME(inidx), strerror(errno));
                goto end;
            }
            break;
        }
        str = str_mal;

        if(*(str + loopsz - 1) == '\n')
        {
            *(str + loopsz - 1) = '\0';
            loopsz--;
        }

        if(loopsz <= 0)
        {
            rc = RC_CRIT;
            prerr("malformed: empty line\n");
            goto end;
        }

        if(streq(str, AutoskipOutput))
        {
            if( ! Params.margin)
            {
                rc = RC_USER;
                prerr("cannot process '%s' with 0 margin\n", AutoskipOutput);
                goto end;
            }
            if(out_at < 0 || ! octets_mal)
            {
                rc = RC_USER;
                prerr("malformed: first line cannot be '%s'\n", AutoskipOutput);
                goto end;
            }
            if(skip_pending)
            {
                rc = RC_USER;
                prerr("malformed: adjacent lines with '%s'\n", AutoskipOutput);
                goto end;
            }
            skip_pending = true;
            continue;
        }

        if(Params.margin)
        {
            hoff_t addr = HOFF_NIL;
            rc = strtooff(str, (char const **)&str, &addr, -1);
            if(rc ||
               str - str_mal != Params.margin ||
               strnconsume((char const**)&str, MarginPost, strlen(MarginPost)))
            {
                if( ! rc)
                    rc = RC_USER;
                prerr("malformed file offset\n");
                goto end;
            }
            if(skip_pending)
            {
                for(hoff_t wr_at = out_at; wr_at < addr; )
                {
                    hoff_t wr_sz = MIN(last_sz, addr - wr_at);
                    if(hexpeek_write(outfd, octets_mal, wr_sz) != wr_sz)
                    {
                        rc = RC_CRIT;
                        goto end;
                    }
                    wr_at += wr_sz;
                }
                skip_pending = false;
            }
            rc = seekto(outfd, addr);
            if(rc)
                goto end;
            out_at = addr;
        }

        if(Params.print_text)
        {
            char *tmpp = str_mal + loopsz - DispLine - 2;
            if(strncmp(tmpp, "  ", 2))
            {
                rc = RC_USER;
                prerr("unexpected data where \"  \" was expected\n");
                goto end;
            }
            *tmpp = '\0';
        }

        last_sz = maxOctetWidth(str_sz);
        if(last_sz > octets_sz)
        {
            free(octets_mal);
            octets_sz = last_sz;
            octets_mal = Malloc(octets_sz);
        }
        rc = textToOctetArray(str, DispMode, &last_sz, octets_mal, NULL);
        if(rc)
            goto end;
        if(hexpeek_write(outfd, octets_mal, last_sz) != last_sz)
        {
            rc = RC_CRIT;
            goto end;
        }
        out_at += last_sz;
    }

    if(skip_pending)
    {
        rc = RC_USER;
        prerr("malformed: last line cannot be '%s'\n", AutoskipOutput);
        goto end;
    }

    rc = RC_OK;

end:
    if(rc && loop > 0)
    {
        prerr("error encountered in %s, line %" PRIdMAX " (decimal)\n",
              DT_NAME(inidx), loop);
    }
    if(str_mal)
    {
        free(str_mal);
        str_mal = NULL;
    }
    if(octets_mal)
    {
        free(octets_mal);
        octets_mal = NULL;
    }
    if(infp)
    {
        fclose(infp);
        infp = NULL;
        DT_FD(inidx) = -1;
    }
    return rc;
}

// Buffers used:
#define B_CUR 0 // current buffer - all show_*() use
#define B_RGT 1 // right buffer   - show_d() uses
#define B_OLD 2 // old buffer     - show_n() uses
#define B_NXT 3 // next buffer    - show_n() uses the length

/**
 * @brief Normal print method (line width fits into BUFSZ).
 *
 * @param[in] start Starting file offset of display region
 * @param[in] already Bytes already processed on previous calls
 * @param[in] bufs Binary data buffers (uses B_CUR and B_OLD)
 * @param[in] lens Lengths of respective buffers (uses B_CUR, B_OLD, and B_NXT)
 * @param[in] mgfmt Format string generated by hexpeek_genf()
 * @param[in] unused Unused (for API compatibility with other show_* functions)
 * @param[in,out] skip Autoskip enabled (output value is for tracking internal
 *                state for subsequent calls)
 * @return Always returns 0
 */
int show_n(hoff_t start, hoff_t already, uint8_t *bufs[4], hoff_t lens[4],
           char *mgfmt, char *unused, int *skip)
{
    int result = 0;
    const hoff_t owd = outputWidth(1, DispMode, DispLine);
    char fmtd[MAX(owd + 1, 128)];
    char enc[DispLine + 1];
    char *lim = fmtd + sizeof fmtd;
    hoff_t maxlen = lens[0];

    for(hoff_t lx = 0; lx < maxlen; )
    {
        hoff_t adj = already + lx;
        hoff_t amt = MIN(DispLine, lens[0] - lx);
        char *ptr = fmtd;

        memset(fmtd, '\0', sizeof fmtd);
        endianize(bufs[0] + lx, amt);

                    // not last line                            //
        if(*skip && (lens[B_NXT] > 0 || (lx + DispLine) < maxlen))
        {
            uint8_t *cmp = NULL;
            if(lx >= DispLine)
                cmp = bufs[0] + lx - DispLine;
            else if(lens[B_OLD] >= DispLine)
                cmp = bufs[B_OLD] + lens[B_OLD] - DispLine;
            if(cmp && amt == DispLine && memcmp(bufs[0] + lx, cmp, amt) == 0)
            {
                if(*skip != 2)
                {
                    console("%s%s", AutoskipOutput, LineTerm);
                    *skip = 2;
                }
                goto loop;
            }
            *skip = 1;
        }

        if(Params.margin)
            console(mgfmt, HoffPrefix, Params.margin, mgAddr(start + adj));

        if(DispGroup == 0)
        {
            if(lx % DispLine == 0)
                ptr += slprintf(ptr, lim, "%s", GroupPre(0));
            ptr += convertBinary(bufs[0] + lx, amt, ptr, lim);
        }
        else for(hoff_t gx = 0; gx < amt; gx += DispGroup)
        {
            ptr += slprintf(ptr, lim, "%s", GroupPre(gx));
            ptr += convertBinary(bufs[0] + lx + gx, MIN(DispGroup, amt - gx),
                                 ptr, lim);
            if(amt - gx >= DispGroup) // no delimiter if incomplete
                ptr += slprintf(ptr, lim, "%s", GroupTerm);
        }
        if(Params.print_text)
            memset(ptr, ' ', fmtd + owd - ptr);
        console("%s", fmtd);

        if(Params.print_text)
        {
            getEncoded(Params.text_encoding, bufs[0] + lx, amt, enc);
            console("  %s", enc);
        }

        console("%s", LineTerm);

loop:
        lx += DispLine;
    }

    return result;
}

#define REPBF(c) \
    for(int bf = 0; bf < 2; bf++) \
    { \
        c \
    }

/**
 * @brief Diff print method comparing data in bufs[B_CUR] and bufs[B_RGT].
 *
 * @param[in] start Starting file offset of display region
 * @param[in] already Bytes already processed on previous calls
 * @param[in] bufs Binary data buffers (uses B_CUR and B_RGT)
 * @param[in] lens Lengths of respective buffers (uses B_CUR and B_RGT)
 * @param[in] mgfmt Format string generated by hexpeek_genf()
 * @return Returns 0 if same, 1 if any difference found
 */
int show_d(hoff_t start, hoff_t already, uint8_t *bufs[4], hoff_t lens[4],
           char *mgfmt) // , char *unused, int *unused1)
                        // formerly shared with show_n(), was split to optimize
{
    int result = 0;
    const hoff_t owd = outputWidth(1, DispMode, DispLine);
    char fmtd[2][MAX(owd + 1, 128)];
    char *lims[2] = { fmtd[0] + sizeof fmtd[0], fmtd[1] + sizeof fmtd[1] };
    hoff_t maxlen = MAX(lens[0], lens[1]);

    for(hoff_t lx = 0; lx < maxlen; )
    {
        hoff_t adj = already + lx;
        hoff_t amts[2] = { MIN(DispLine, lens[0] - lx),
                           MIN(DispLine, lens[1] - lx) };
        char *ptrs[2]  = { fmtd[0], fmtd[1] };
        bool same = true;

        memset(fmtd, '\0', sizeof fmtd);

        for(int bf = 0; bf < 2; bf++)
            endianize(bufs[bf] + lx, amts[bf]);

        if(DispGroup == 0)
        {
            if(lx % DispLine == 0)
REPBF(          ptrs[bf] += slprintf(ptrs[bf], lims[bf], "%s", GroupPre(0));  );
            convertBinary_(bufs[0] + lx, bufs[1] + lx,
                           MIN(DispGroup, amts[0]), MIN(DispGroup, amts[1]),
                           ptrs, lims, &same);
        }
        else for(hoff_t gx = 0; gx < MAX(amts[0], amts[1]); gx += DispGroup)
        {
REPBF(      if(gx < amts[bf])
                ptrs[bf] += slprintf(ptrs[bf], lims[bf], "%s", GroupPre(gx)); );
            convertBinary_(bufs[0] + lx + gx, bufs[1] + lx + gx,
                           MIN(DispGroup, amts[0] - gx),
                           MIN(DispGroup, amts[1] - gx),
                           ptrs, lims, &same);
REPBF(      if(amts[bf] - gx >= DispGroup) // no delimiter if incomplete
                ptrs[bf] += slprintf(ptrs[bf], lims[bf], "%s", GroupTerm);    );
        }

        if( ! same)
            result = 1;
        else if(Params.diffskip)
            goto loop;

        memset(ptrs[0], ' ', fmtd[0] + owd - ptrs[0]);

        if(Params.margin)
            console(mgfmt, HoffPrefix, Params.margin, mgAddr(start + adj));
        console("%s|%s%s", fmtd[0], fmtd[1], LineTerm);

loop:
        lx += DispLine;
    }

    return result;
}

/**
 * @brief Large print method (when line width exceeds BUFSZ). This function
 *        has some limitations compared to show_n() like lack of support for
 *        autoskip or textual output.
 *
 * @param[in] start Starting file offset of display region
 * @param[in] already Bytes already processed on previous calls
 * @param[in] bufs Binary data buffers (only uses B_CUR)
 * @param[in] lens Lengths of respective buffers (only uses B_CUR)
 * @param[in] mgfmt Format string generated by hexpeek_genf()
 * @param[in] unused Unused (for API compatibility with other show_* functions)
 * @param[in] unused1 Unused (for API compatibility with other show_* functions)
 * @return Always returns 0
 */
int show_l(hoff_t start, hoff_t already, uint8_t *bufs[4], hoff_t lens[4],
           char *mgfmt, char *unused, int *unused1)
{
    char fmtd[DISP_CHCNT + 1];
    for(hoff_t ix = 0; ix < lens[0]; ix++)
    {
        hoff_t adj = already + ix;
        if(Params.margin && mod(adj, DispLine) == 0)
            console(mgfmt, HoffPrefix, Params.margin, mgAddr(start + adj));
        if(groupStart(adj, '\0'))
            endianize(bufs[0] + ix, DispGroup);
        convertBinary(bufs[0] + ix, 1, fmtd, fmtd + sizeof fmtd);
        console("%s", fmtd);
        groupEnd(adj, '\0');
        if(mod(adj + 1, DispLine) == 0 || ix + 1 == lens[0])
            console("%s", LineTerm);
    }
    return 0;
}

/**
 * @brief Verbose print method, showing one octet on each line with offset,
 *        hexadecimal, decimal, octal, bits, bit info, and text format.
 *
 * @param[in] start Starting file offset of display region
 * @param[in] already Bytes already processed on previous calls
 * @param[in] bufs Binary data buffers (only uses B_CUR)
 * @param[in] lens Lengths of respective buffers (only uses B_CUR)
 * @param[in] mgfmt Format string generated by hexpeek_genf()
 * @param[in] unused Unused (for API compatibility with other show_* functions)
 * @param[in] unused1 Unused (for API compatibility with other show_* functions)
 * @return Always returns 0
 */
int show_v(hoff_t start, hoff_t already, uint8_t *bufs[4], hoff_t lens[4],
           char *mgfmt, char *vbfmt, int *unused1)
{
    for(hoff_t ix = 0; ix < lens[0]; ix++)
    {
        console(mgfmt, HoffPrefix, HOFF_HEX_FULL_WIDTH,
                (uintmax_t)(start + already + ix));
        console(vbfmt, bufs[0][ix], bufs[0][ix], bufs[0][ix],
                BinLookup_bits[bufs[0][ix]],
                bitinfo(highbit(bufs[0][ix])), bitinfo(lowbit(bufs[0][ix])),
                bitinfo(countbit(bufs[0][ix])),
                getEncodedVerbose(Params.text_encoding, bufs[0][ix]));
    }
    return 0;
}

#define MarginFormat  "%s%0*" PRI_hxpkmax MarginPost
#define VerboseFormat "%02" PRI_hxpkint " %03d %03o %s %c/%c/%c %s\n"

rc_t processCommand(ParsedCommand*);

/**
 * @brief Execute a print command.
 *
 * @param[in] ppc Pointer to a ParsedCommand structure.
 * @param[out] octets_processed Amount of data processed by this function
 * @return RC_OK on success; else a hexpeek error code
 */
rc_t processCommand_print(ParsedCommand const *ppc, hoff_t *octets_processed)
{
    rc_t rc = RC_UNSPEC;
    hoff_t length = ppc->fz.len;
    hoff_t line = (ppc->print_verbose ? 1 : DispLine);
    int (*subfnc)(hoff_t, hoff_t, uint8_t*[4], hoff_t[4],
                  char*, char*, int *) = NULL;
    uint8_t storage[3][BUFSZ];
    uint8_t *tmpp = NULL;
    uint8_t *rd_bufs[] = { storage[0], NULL, storage[1], storage[2] };
    hoff_t rd_lens[] = { 0, 0, 0, 0 };
    hoff_t tot = 0;
    bool eof = false;
    int toskip = (Params.autoskip && Params.margin > 0);
    // If autoskip is enabled, we need to be a little more clever, but this
    // cleverness is not very cache-friendly. Without this, a '*' might be
    // generated as the last line of output, which is invalid.
    int next = B_CUR;
    if(toskip)
    {
        trace("pre-fetching next buffer for autoskip edge case\n");
        next = B_NXT;
    }

    // Print header
    if(ppc->print_verbose)
    {
        subfnc = show_v;
        if(ppc->print_off)
        {
            console("File Offset      Hex Dec Oct Bits     H/L/C %s\n",
                    encodingName(Params.text_encoding));
        }
    }
    else
    {
        subfnc = show_n;
        if(DispLine < 1 || DispLine > MAXW_LINE)
            subfnc = show_l;
        if(ppc->print_off)
        {
            consoleOutf("At " PRI_hoff " (" PRI_hoff " octet%s requested, ",
                        prihoff(ppc->fz.start),
                        prihoff(length), length == 1 ? "" : "s");
            if(line)
                consoleOutf(PRI_hoff, prihoff(line));
            else
                console("all");
            console(" per line, %s) :\n",
                    ppc->print_verbose ? "verbose" : dispName());
        }
        if(Params.ruler)
            printRuler();
    }

    // Pre-generate format strings
    char mgfmt[strlen(MarginFormat) + 1];
    hexpeek_genf(mgfmt, MarginFormat);
    char vbfmt[strlen(VerboseFormat) + 1];
    hexpeek_genf(vbfmt, VerboseFormat);

    // Read and print
    for(bool fail = false; length > 0 || rd_lens[B_CUR] > 0; )
    {
        // Read next buffer
        if(length > 0)
        {
            hoff_t nlen = MIN(BUFSZ, length);
            rd_lens[next] = hexpeek_read(DT_FD(ppc->fz.fi), rd_bufs[next],
                                         line > 1 ? bestfit(line, nlen) : nlen);
            if(rd_lens[next] < 0)
                fail = true;
            else if(rd_lens[next])
                length -= rd_lens[next];
            else
            {
                eof = true;
                length = 0;
            }
        }
        // Print current buffer
        if(rd_lens[B_CUR] > 0)
        {
            subfnc(ppc->fz.start, tot, rd_bufs, rd_lens, mgfmt, vbfmt, &toskip);
            assert(tot <= HOFF_MAX - rd_lens[B_CUR]);
            tot += rd_lens[B_CUR];
        }
        // Fail if needed
        if(fail)
        {
            rc = RC_CRIT;
            goto end;
        }
        // Swap pointers to different underlying storage
        tmpp           = rd_bufs[B_OLD];
        rd_bufs[B_OLD] = rd_bufs[B_CUR];
        rd_bufs[B_CUR] = rd_bufs[B_NXT]; // meaningless if next == B_CUR
        rd_bufs[next]  = tmpp;
        // Swap lengths
        rd_lens[B_OLD] = rd_lens[B_CUR];
        rd_lens[B_CUR] = rd_lens[B_NXT];
        rd_lens[B_NXT] = 0;
    }

    *octets_processed = tot;
    rc = RC_OK;

    if(eof && ! ppc->fz.tolerate_eof)
    {
        rc = RC_USER;
        prerr(EofErrString, DT_NAME(ppc->fz.fi));
    }

end:
    consoleFlush();
    return rc;
}

/**
 * @brief Execute a diff command.
 *
 * @param[in] ppc Pointer to a ParsedCommand structure.
 * @param[out] octets_processed Amount of data processed by this function
 * @return RC_OK or RC_DIFF on success; else a hexpeek error code
 */
rc_t processCommand_diff(ParsedCommand const *ppc, hoff_t *octets_processed)
{
    rc_t rc = RC_UNSPEC;
    int differ = 0;
    hoff_t lengths[] = { ppc->fz.len, ppc->arg_cv.fz.len };
    hoff_t old_line = DispLine;
    uint8_t storage[2][BUFSZ];
    uint8_t *rd_bufs[] = { storage[0], storage[1], NULL, NULL };
    hoff_t rd_lens[] = { 0, 0, 0, 0 };
    FileZone const *pfzs[] = { &ppc->fz, &ppc->arg_cv.fz };
    bool eofs[] = { false, false };
    hoff_t tot = 0;

    assert(ppc->arg_cv.mem.count <= 0);
    assert(lengths[0] >= 0);
    assert(lengths[1] >= 0);

    // Silently fix column width
    if(DispLine < 1 || DispLine > MAXW_LINE)
        DispLine = MAXW_LINE;

    // Pre-generate format strings
    char mgfmt[strlen(MarginFormat) + 1];
    hexpeek_genf(mgfmt, MarginFormat);

    // Read and print a diff
    for(;;)
    {
        // Read buffers
        rd_lens[0] = rd_lens[1] = 0;
        hoff_t maxlen = bestfit(DispLine,
                                MIN(BUFSZ, MAX(lengths[0], lengths[1])));
        for(int bf = 0; bf < 2; bf++)
        {
            if(lengths[bf] > 0)
            {
                rc = seekto(DT_FD(pfzs[bf]->fi), pfzs[bf]->start + tot);
                if(rc)
                    goto end;
                rd_lens[bf] = hexpeek_read(DT_FD(pfzs[bf]->fi), rd_bufs[bf],
                                           MIN(maxlen, lengths[bf]));
                if(rd_lens[bf] < 0)
                {
                    rc = RC_CRIT;
                    goto end;
                }
                else if(rd_lens[bf] == 0)
                {
                    eofs[bf] = true;
                    lengths[bf] = 0;
                }
            }
        }
        maxlen = MAX(rd_lens[0], rd_lens[1]);
        if(maxlen <= 0)
            break;
        // Print buffers
        if(ppc->diff_srch)
        {
            hoff_t ix = 0;
            hoff_t minlen = MIN(rd_lens[0], rd_lens[1]);
            for( ; ix < minlen; ix++)
            {
                if(rd_bufs[0][ix] != rd_bufs[1][ix])
                    break;
            }
            if(ix < minlen || rd_lens[0] != rd_lens[1])
            {
                differ = 1;
                hoff_t match = ppc->fz.start + tot + ix;
                DT_AT(ppc->fz.fi) = match;
                if(DispSrchDef)
                {
                    ParsedCommand toprint;
                    ParsedCommand_init(&toprint);
                    toprint.cmd = CMD_DIFF;
                    toprint.fz.fi = ppc->fz.fi;
                    toprint.fz.start = match;
                    toprint.fz.len = DispSrchDef;
                    toprint.arg_t = "";
                    toprint.arg_cv.fz.fi = ppc->arg_cv.fz.fi;
                    toprint.arg_cv.fz.start = ppc->arg_cv.fz.start +
                                              (match - ppc->fz.start);
                    toprint.arg_cv.fz.len = DispSrchDef;                    
                    rc = processCommand(&toprint);
                }
                else
                {
                    consoleOutf(PRI_hoff "%s", prihoff(match), LineTerm);
                }
                *octets_processed = DispSrchDef; // at already moved to match
                goto end;
            }
        }
        else
        {
            if(show_d(ppc->fz.start, tot, rd_bufs, rd_lens, mgfmt))
                differ = 1;
        }
        // Adjust counters
        for(int bf = 0; bf < 2; bf++)
            lengths[bf] -= rd_lens[bf];
        assert(tot <= HOFF_MAX - maxlen);
        tot += maxlen;
    }

    *octets_processed = tot;
    rc = RC_OK;

    // Check EOF
    for(int bf = 0; bf < 2; bf++)
    {
        if(eofs[bf] && ! pfzs[bf]->tolerate_eof)
        {
            rc = RC_USER;
            prerr(EofErrString, DT_NAME(pfzs[bf]->fi));
        }
    }

end:
    DispLine = old_line;
    consoleFlush();
    if(rc == RC_OK && differ)
        rc = RC_DIFF;
    return rc;
}

/**
 * @brief Execute a search command.
 *
 * @param[in] ppc Pointer to a ParsedCommand structure.
 * @param[out] octets_processed Amount of data processed by this function
 * @return RC_OK on success; else a hexpeek error code
 */
rc_t processCommand_search(ParsedCommand const *ppc, hoff_t *octets_processed)
{
    rc_t rc = RC_UNSPEC;
    hoff_t match = (hoff_t)-1, prev_rd = 0;
    uint8_t rd_buf[SRCHSZ * 2];
    hoff_t const sh_cnt = ppc->arg_cv.mem.count;
    uint8_t const *sh_ptr = ppc->arg_cv.mem.octets_mal;
    uint8_t const *sh_masks = ppc->arg_cv.mem.masks_mal;

    if(sh_cnt == 0)
    {
        // no-op
        rc = RC_OK;
        goto end;
    }

    assert(sh_cnt <= SRCHSZ);
    assert(sh_ptr);
    assert(sh_masks);

    for(hoff_t uncheckable = 0; ; )
    {
        hoff_t lcl_rd = hexpeek_read(DT_FD(ppc->fz.fi),
                                     rd_buf + uncheckable,
                                     sizeof rd_buf - uncheckable);
        lcl_rd += uncheckable;
        uncheckable = sh_cnt - 1;
        if(lcl_rd < 0)
        {
            rc = RC_CRIT;
            goto end;
        }
        if(lcl_rd < sh_cnt)
        {
            prev_rd += lcl_rd;
            goto done;
        }
        for(hoff_t lcl_idx = 0; lcl_idx < lcl_rd - uncheckable; lcl_idx++)
        {
            if(prev_rd + lcl_idx + sh_cnt > ppc->fz.len)
            {
                prev_rd += lcl_idx;
                goto done;
            }
            hoff_t cmp_idx = 0;
            for( ; cmp_idx < sh_cnt; cmp_idx++)
            {
                if( sh_ptr[cmp_idx] != (rd_buf[lcl_idx + cmp_idx] &
                                        sh_masks[cmp_idx]) )
                    break;
            }
            if(cmp_idx == sh_cnt)
            {
                prev_rd += lcl_idx + 1;
                match = ppc->fz.start + prev_rd - 1;
                goto done;
            }
        }
        prev_rd += lcl_rd - uncheckable;
        if(uncheckable > 0)
            memmove(rd_buf, rd_buf + lcl_rd - uncheckable, uncheckable);
    }

done:
    if(match < 0)
    {
        if(DispSrchDef && interactive())
            console("Search failed.\n");
        *octets_processed = prev_rd;
    }
    else
    {
        DT_AT(ppc->fz.fi) = match;
        if(DispSrchDef)
        {
            ParsedCommand toprint;
            ParsedCommand_init(&toprint);
            toprint.cmd = CMD_PRINT;
            toprint.fz.fi = ppc->fz.fi;
            toprint.fz.start = match;
            toprint.fz.len = DispSrchDef;
            toprint.print_off = true;
            toprint.arg_t = "";
            rc = processCommand(&toprint);
            if(rc)
                goto end;
        }
        else
        {
            consoleOutf(PRI_hoff "%s", prihoff(match), LineTerm);
        }
        *octets_processed = sh_cnt; // file already moved to match
    }

    rc = RC_OK;

end:
    return rc;
}

/**
 * @brief Execute a change data command.
 *
 * @param[in] ppc Pointer to a ParsedCommand structure.
 * @param[out] octets_processed Amount of data processed by this function
 * @param[out] bked Boolean result if backup occurred
 * @return RC_OK on success; else a hexpeek error code
 */
rc_t processCommand_changedata(ParsedCommand *ppc, hoff_t *octets_processed,
                               bool *bked)
{
    rc_t rc = RC_UNSPEC;
    hoff_t wr_cnt = 0, wr_tot = 0;
    uint8_t *wr_ptr = NULL;

    if(ppc->arg_cv.mem.count > 0)
    {
        wr_cnt = ppc->arg_cv.mem.count;
        wr_ptr = ppc->arg_cv.mem.octets_mal;
        assert(wr_ptr);
    }
    else
    {
        wr_cnt = ppc->arg_cv.fz.len;
    }

    if(wr_cnt == 0)
    {
        // no-op
        rc = RC_OK;
        goto end;
    }

    if(wr_cnt > ppc->fz.len)
    {
        rc = RC_USER;
        malcmd("input length exceeds specified length\n");
        goto end;
    }

    if(ppc->fz.start > filesize(ppc->fz.fi))
    {
        trace("creating hole [" TRC_hoff ":" TRC_hoff "]\n",
              trchoff(filesize(ppc->fz.fi)), trchoff(ppc->fz.start));
    }

    rc = makeBackup(ppc);
    if(rc)
        goto end;
    *bked = true;

    if(ppc->cmd == CMD_INSERT)
    {
        rc = adjustSize(ppc->fz.fi, ppc->fz.start, ppc->fz.len, -1);
        if(rc)
            goto end;
    }

    if(wr_ptr)
    {
        if(ppc->fz.len >= 2 * wr_cnt)
        {
            // Optimize repeated write
            hoff_t step = wr_cnt;
            while(wr_cnt + step <= MIN(ppc->arg_cv.mem.sz, ppc->fz.len))
            {
                memcpy(wr_ptr + wr_cnt, wr_ptr, step);
                wr_cnt += step;
            }
        }
        while(ppc->fz.len > 0)
        {
            hoff_t try_len = MIN(ppc->fz.len, wr_cnt);
            if(hexpeek_write(DT_FD(ppc->fz.fi), wr_ptr, try_len) != try_len)
            {
                rc = RC_CRIT;
                goto end;
            }
            wr_tot += try_len;
            ppc->fz.len -= try_len;
            plugin(2, NULL);
        }
    }
    else
    {
#ifdef HEXPEEK_UNIQUE_INFILES
        if(ppc->cmd == CMD_INSERT && ppc->arg_cv.fz.fi == ppc->fz.fi)
#else
        #error
#endif
        {
            // ppc->fz and ppc->arg_cv.fz refer to same file, so use lclcpy()
            if(ppc->arg_cv.fz.start >= ppc->fz.start)
            {
                // source data was moved forward by above size adjustment
                ppc->arg_cv.fz.start += ppc->fz.len;
            }
            else if(ppc->fz.start - ppc->arg_cv.fz.start < ppc->arg_cv.fz.len)
            {
                hoff_t prelen = ppc->fz.start - ppc->arg_cv.fz.start;
                hoff_t postlen = ppc->arg_cv.fz.len - prelen;
                assert(prelen > 0);
                assert(postlen >= 0);
                // copy data that wasn't moved by above size adjustment
                rc = lclcpy(DT_FD(ppc->fz.fi), ppc->arg_cv.fz.start,
                            ppc->fz.start, prelen);
                if(rc)
                    goto end;
                wr_tot += prelen;
                // remaining source data was moved forward
                if(postlen > 0)
                {
                    ppc->arg_cv.fz.start += ppc->fz.len;
                    rc= lclcpy(DT_FD(ppc->fz.fi), ppc->arg_cv.fz.start + prelen,
                               ppc->fz.start + prelen, postlen);
                    if(rc)
                        goto end;
                    wr_tot += postlen;
                }
                // source data is now continuously located at ppc->fz.start
                // further reads will be from there, and further writes will
                // be to immediately thereafter
                ppc->arg_cv.fz.start = ppc->fz.start;
                ppc->fz.start += wr_tot;
                ppc->fz.len -= wr_tot;
                wr_cnt = MIN(wr_cnt, ppc->fz.len);
            }
        }

        // do primary data copy
        if(ppc->fz.len > 0)
        {
            rc = filecpy(DT_FD(ppc->arg_cv.fz.fi), ppc->arg_cv.fz.start, wr_cnt,
                         DT_FD(ppc->fz.fi),        ppc->fz.start,  ppc->fz.len);
            if(rc)
                goto end;
            wr_tot += ppc->fz.len;
        }
    }

    *octets_processed = wr_tot;

    rc = RC_OK;

end:
    return rc;
}

#define TRACE_FZ "fz=[%d, " TRC_hoff ", " TRC_hoff "]"
#define TRACE_CV "[ mem=[" TRC_hoff ", " TRC_hoff ", %p, %p], " TRACE_FZ " ]"

/**
 * @brief Execute a provided ParsedCommand structure - primary command
 *        multiplexer entry point.
 *
 * @param ppc Pointer to a ParsedCommand structure.
 * @return RC_OK, RC_DIFF, or RC_DONE on success; else a hexpeek error code
 */
rc_t processCommand(ParsedCommand *ppc)
{
    rc_t rc = RC_OK;
    hoff_t octets_processed = 0;
    bool backup_done = false;

    assert(ppc);
    assert(ppc->arg_t);

    traceEntry("origcmd='%s', cmd=%d, subtype=%d, " TRACE_FZ ", "
               "incr_pre=%d, incr_post=%d, print_off=%d, print_verbose=%d, "
               "diff_srch=%d, arg_t='%s', arg_cv=" TRACE_CV,
               ppc->origcmd,
               ppc->cmd,
               ppc->subtype,
               ppc->fz.fi, trchoff(ppc->fz.start), trchoff(ppc->fz.len),
               (int)ppc->incr_pre,
               (int)ppc->incr_post,
               (int)ppc->print_off,
               (int)ppc->print_verbose,
               (int)ppc->diff_srch,
               ppc->arg_t,
               trchoff(ppc->arg_cv.mem.sz), trchoff(ppc->arg_cv.mem.count),
                   ppc->arg_cv.mem.octets_mal, ppc->arg_cv.mem.masks_mal,
                   ppc->arg_cv.fz.fi, trchoff(ppc->arg_cv.fz.start),
                   trchoff(ppc->arg_cv.fz.len));

    // Save current offset in case restore is needed later
    if(seekableCommand(ppc->cmd))
    {
        assert(ppc->fz.fi >= 0);
        Params.infiles[ppc->fz.fi].last_at = DT_AT(ppc->fz.fi);
    }

    // Verify writeability
    if(writeableCommand(ppc->cmd))
    {
        if( ! (DT_MODE(ppc->fz.fi) & O_RDWR))
        {
            rc = RC_USER;
            prerr("file $%d opened read-only\n", ppc->fz.fi);
            goto end;
        }
        if(Params.allow_ik == false && ppc->cmd != CMD_REPLACE)
        {
            rc = RC_USER;
            prerr("insert and kill commands disabled by run settings\n");
            goto end;
        }
    }

    // Seek if needed
    if(seekableCommand(ppc->cmd))
    {
        assert(ppc->fz.fi >= 0);
        assert(ppc->fz.start >= 0);
        assert(ppc->fz.len >= 0);
        if(ppc->incr_pre)
        {
            hoff_t incr_len = ppc->fz.len;
            if(ppc->cmd == CMD_SEARCH)
                incr_len = 1;
            else if(ppc->diff_srch)
                incr_len = MAX(DispSrchDef, 1);
            assert(incr_len >= 0);
            assert(ppc->fz.start <= HOFF_MAX - incr_len);
            ppc->fz.start += incr_len;
        }
        if(ppc->arg_cv.fz.fi == FILE_INDEX_LATER)
        {
            memcpy(&ppc->arg_cv.fz, &ppc->fz, sizeof ppc->arg_cv.fz);
            ppc->arg_cv.fz.fi ^= 1;
            ppc->arg_cv.fz.tolerate_eof = true;
        }
        DT_AT(ppc->fz.fi) = ppc->fz.start;
        rc = seekto(DT_FD(ppc->fz.fi), ppc->fz.start);
        if(rc)
            goto end;
    }

    // Do specific command processing
    rc = processShared(ppc->cmd, ppc->subtype, ppc->arg_t, Params.disp_mode);
    if(rc == RC_OK)
        goto done;
    else if(rc == RC_NIL)
        rc = RC_OK;
    else
        goto end;
    if(ppc->cmd == CMD_QUIT)
    {
        rc = RC_DONE;
        goto end;
    }
    else if(ppc->cmd == CMD_STOP)
    {
        rc = RC_DONE;
        BackupUnlinkAllowed = false;
        goto end;
    }
    else if(ppc->cmd == CMD_HELP)
    {
        help(ppc->arg_t);
    }
    else if(ppc->cmd == CMD_FILES)
    {
        printFiles(false);
    }
    else if(ppc->cmd == CMD_RESET)
    {
        if(*ppc->arg_t == FZ_PREF[0])
        {
            FileZone fz;
            rc = ascertainFileZone(ppc->arg_t, 0, &fz, &ppc->arg_t);
            if(rc)
                goto end;
            if(*ppc->arg_t != '\0')
            {
                rc = RC_USER;
                malcmd("invalid argument to reset\n");
                goto end;
            }
            assert(fz.fi >= 0);
            DT_AT(fz.fi) = HOFF_NIL;
        }
        else if(*ppc->arg_t == '\0')
        {
            for(int fi = 0; fi < MAX_INFILES; fi++)
                DT_AT(fi) = HOFF_NIL;
        }
        else
        {
            rc = RC_USER;
            malcmd("invalid argument to reset\n");
            goto end;
        }
    }
    else if(ppc->cmd == CMD_SETTINGS)
    {
        printSettings();
    }
    else if(ppc->cmd == CMD_PRINT)
    {
        rc = processCommand_print(ppc, &octets_processed);
    }
    else if(ppc->cmd == CMD_OFFSET)
    {
        consoleOutf(PRI_hoff "%s", prihoff(ppc->fz.start), LineTerm);
    }
    else if(ppc->cmd == CMD_SEARCH)
    {
        rc = processCommand_search(ppc, &octets_processed);
    }
    else if(ppc->cmd == CMD_DIFF)
    {
        rc = processCommand_diff(ppc, &octets_processed);
    }
    else if(ppc->cmd == CMD_REPLACE || ppc->cmd == CMD_INSERT)
    {
        rc = processCommand_changedata(ppc, &octets_processed, &backup_done);
    }
    else if(ppc->cmd == CMD_KILL)
    {
        hoff_t fsz = filesize(ppc->fz.fi);
        if(ppc->fz.len > fsz - ppc->fz.start)
        {
            if(Params.infer)
            {
                ppc->fz.len = fsz - ppc->fz.start;
            }
            else
            {
                rc = RC_USER;
                prohibcmd(InferredDeleteLengthErrString);
                goto end;
            }
        }
        rc = makeBackup(ppc);
        if(rc)
            goto end;
        backup_done = true;
        rc = adjustSize(ppc->fz.fi, ppc->fz.start, -ppc->fz.len, -1);
        if(rc)
            goto end;
    }
    else if(ppc->cmd == CMD_OPS)
    {
        rc = recoverBackup(0, -1);
    }
    else if(ppc->cmd == CMD_UNDO)
    {
        char *endptr = NULL;
        long tmpl = 1;
        if(*ppc->arg_t != '\0')
        {
            tmpl = strtol(ppc->arg_t, &endptr, Params.scalar_base);
            if(tmpl < 0 || tmpl >= INT_MAX || ppc->arg_t == endptr)
            {
                rc = RC_USER;
                malcmd("invalid operation index\n");
                goto end;
            }
        }
        rc = recoverBackup(0, (int)tmpl);
    }
    else
    {
        rc = RC_USER;
        goto end;
    }

done:
    if(ppc->incr_post)
        DT_AT(ppc->fz.fi) = DT_AT(ppc->fz.fi) + octets_processed;
    if(writeableCommand(ppc->cmd))
        plugin(1, (void*)ppc->origcmd);
    if(ppc->fz.fi >= 0)
        Params.infiles[ppc->fz.fi].last_at = DT_AT(ppc->fz.fi);

end:
    if(ppc->fz.fi >= 0)
        DT_AT(ppc->fz.fi) = Params.infiles[ppc->fz.fi].last_at;
    if(ppc->arg_cv.mem.octets_mal)
    {
        free(ppc->arg_cv.mem.octets_mal);
        ppc->arg_cv.mem.octets_mal = NULL;
    }
    if(ppc->arg_cv.mem.masks_mal)
    {
        free(ppc->arg_cv.mem.masks_mal);
        ppc->arg_cv.mem.masks_mal = NULL;
    }
    if(backup_done)
        DT_OPCNT(ppc->fz.fi)++;
    traceExit(TRC_rc, rc);
    return rc;
}

/**
 * @brief Given user text command input, ascertain and process the command.
 *
 * @param line Text command input.
 * @param report_diff If true and last command is a diff, report diff rc
 * @return RC_OK, RC_DIFF, or RC_DONE on success; else a hexpeek error code
 */
rc_t processInput(char *line, bool report_diff)
{
    rc_t rc = RC_UNSPEC;

    if(strncmp(line, "####", 4) == 0)
    {
        fprintf(stderr, "%s", line);
    }
    else if(strncmp(line, "#", 1) == 0)
    {
        // no-op
    }
    else for(char *ptr = NULL; line; line = ptr)
    {
        ptr = strchr(line, ';');
        if(ptr)
            *(ptr++) = '\0';

        stripTrailingSpaces(line);
        ParsedCommand pc;
        rc = ascertainCommand((interactive() && *line == '\0') ? "+" : line,
                              true, &pc);
        if(rc)
            goto end;

        rc = processCommand(&pc);
        if(rc == RC_DIFF && (ptr || ! report_diff))
            rc = RC_OK;
        if(rc)
            goto end;
    }

    rc = RC_OK;

end:
    if(rc == RC_USER && ! Params.fail_strict)
        rc = RC_OK;
    return rc;
}

/**
 * @brief Main method - program entry point.
 *
 * @param argc Count of arguments
 * @param argv Array of arguments
 * @return 0 (RC_OK) on successful operation, a non-zero value otherwise
 */
int main(int argc, char **argv)
{
    rc_t rc = RC_UNSPEC;
    char *input_line = NULL;

    initialize();

    // Look for any of these flags first
    for(int idx = 1; idx < argc; idx++)
    {
        if(strcmp(argv[idx], "--") == 0)
        {
            break;
        }
        else if(strcmp(argv[idx], "-h") == 0)
        {
            rc = RC_OK;
            usage(false);
            goto end;
        }
        else if(strcmp(argv[idx], "-help") == 0)
        {
            rc = RC_OK;
            usage(true);
            console("\n%s", HelpCmdHdr);
            help("-all");
            console("\n%s", HelpOther);
            goto end;
        }
        else if(strcmp(argv[idx], "-v") == 0)
        {
            rc = RC_OK;
            console("%s", VersionShort);
            goto end;
        }
        else if(strcmp(argv[idx], "-version") == 0)
        {
            rc = RC_OK;
            console("%s", VersionLong);
            goto end;
        }
        else if(strcmp(argv[idx], "-license") == 0)
        {
            rc = RC_OK;
            console("%s", LicenseString);
            goto end;
        }
    }

    // Parse arguments
    rc = parseArgv(argc, argv);
    if(rc)
    {
        prerr("Run with -h for help with arguments.\n");
        goto end;
    }

    // Based on arguments, setup editable console
    consoleInit();

    // Validate tty
    if(Params.recover_interactive && ! interactive())
    {
        rc = RC_CRIT;
        prerr("Interactive recovery mode requires ttys.\n");
        goto end;
    }

    // Open all data and backup files
    rc = openFiles(OPEN_FILES_NORMAL);
    if(rc == RC_DONE)
    {
        rc = RC_OK;
        goto end;
    }
    if(rc)
        goto end;

    // Validate a file argument was given
    if(fileCount() <= 0)
    {
        rc = RC_USER;
        prerr("No data file to open! Run with -h for help with arguments.\n");
        goto end;
    }

    plugin(0, NULL);

    // Do requested operation
    if(Params.recover_interactive || Params.recover_auto)
    {
        introduce(false);
        rc = recoverBackup(0, INT_MAX);
    }
    else if(Params.command)
    {
        rc = processInput(Params.command, true);
        if(rc == RC_DONE)
            rc = RC_OK;
    }
    else if(Params.do_pack)
    {
        rc = pack(0, STDOUT_FILENO);
    }
    else
    {
        introduce(true);

        while((input_line = consoleIn()) != NULL)
        {
            rc = processInput(input_line, false);
            if(rc == RC_DONE)
            {
                rc = RC_OK;
                goto end;
            }
            else if(rc)
            {
                goto end;
            }
        }

        rc = RC_OK;
    }

    // Cleanup
end:
    plugin(-1, NULL);
    rc = closeFiles(rc);
    terminate(rc);
}
