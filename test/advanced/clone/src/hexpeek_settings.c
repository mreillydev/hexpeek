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

#define SRCNAME "hexpeek_settings.c"

#include <hexpeek.h>

#include <stdlib.h>
#include <libgen.h>
#include <limits.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>

/**
 * @file hexpeek_settings.c
 * @brief Conversion functions of user string input into hexpeek commands and
 *        runtime settings.
 */

/**
 * @brief Determine the required character display output width for a given
 *        part of the screen given a certain mode and octet line width.
 *
 * @param[in] part Either margin (0), octets (1), or text (2)
 * @param[in] formode A display mode, e.g. MODE_*
 * @param[in] linewh A line width for which to calculate the display size
 * @return Total width necessary to display the given part
 */
hoff_t outputWidth(int part, int formode, hoff_t linewh)
{
    hoff_t result = 0;

    switch(part)
    {
    case 0:
        if(Params.margin)
            result += (Params.margin + strlen(MarginPost));
        break;
    case 1:
        if(Params.mode_groups[formode])
        {
            hoff_t seps =  linewh / Params.mode_groups[formode] +
                          (linewh % Params.mode_groups[formode] ? 1 : 0);
            if(seps > 0)
                result += (strlen(GroupPre(0)) + strlen(GroupTerm));
            seps--;
            if(seps > 0)
                result += (strlen(GroupPre(1)) + strlen(GroupTerm)) * seps;
        }
        else
        {
            result += strlen(GroupPre(0));
        }
        result += linewh * MODE_CHCNT(formode);
        break;
    case 2:
        if(Params.print_text)
            result += 2 + linewh;
        break;
    default:
        die();
    }

    assert(result >= 0);
    return result;
}

/**
 * @brief Determine the required character display output width for the whole
 *        line given a certain mode and octet line width.
 *
 * @param[in] formode A display mode, e.g. MODE_*
 * @param[in] linewh A line width for which to calculate the display size
 * @return Total width necessary to display the whole line
 */
static hoff_t totalWidth(int formode, hoff_t linewh)
{
    hoff_t result = 0;
    for(int part = 0; part < 3; part++)
        result += outputWidth(part, formode, linewh);
    return result;
}

/**
 * @brief Ascertain whether the provided string corresponds to a shared command
 *        (commands that can either be given as user commands or as command
 *        line flags) and, if so, which command.
 *
 * @param[in] str String to evaluate
 * @param[out] subtype Pointer to subtype of this command
 * @param[out] post Pointer to string location not consumed by this command
 * @return Returns the command, e.g. CMD_*
 */
int ascertainShared(char const *str, int *subtype, char const **post)
{
    assert(str);

    int cmd = CMD_NONE;
    int subtp = 0;

    if(strnconsume(&str, "endianb", 7) == 0)
    {
        cmd = CMD_ENDIAN;
        subtp = 1;
    }
    else if(strnconsume(&str, "endianl", 7) == 0)
    {
        cmd = CMD_ENDIAN;
        subtp = -1;
    }
    else if(strnconsume(&str, "hexl", 4) == 0)
    {
        cmd = CMD_HEX;
        subtp = 1;
    }
    else if(strnconsume(&str, "hexu", 4) == 0)
    {
        cmd = CMD_HEX;
        subtp = -1;
    }
    else if(strnconsume(&str, "hex", 3) == 0)
        cmd = CMD_HEX;
    else if(strnconsume(&str, "bits", 4) == 0)
        cmd = CMD_BITS;
    else if(strnconsume(&str, "rlen", 4) == 0)
        cmd = CMD_RLEN;
    else if(strnconsume(&str, "slen", 4) == 0)
        cmd = CMD_SLEN;
    else if(strnconsume(&str, "line", 4) == 0)
        cmd = CMD_LINE;
    else if(strnconsume(&str, "cols", 4) == 0)
        cmd = CMD_COLS;
    else if(strnconsume(&str, "group", 5) == 0)
        cmd = CMD_GROUP;
    else if(strnconsume(&str, "margin", 6) == 0)
        cmd = CMD_MARGIN;
    else if(strnconsume(&str, "scalar", 6) == 0)
        cmd = CMD_SCALAR;
    else if(strnconsume(&str, "prefix", 6) == 0)
    {
        cmd = CMD_PREFIX;
        subtp = 1;
    }
    else if(strnconsume(&str, "+prefix", 7) == 0)
    {
        cmd = CMD_PREFIX;
        subtp = -1;
    }
    else if(strnconsume(&str, "autoskip", 8) == 0)
    {
        cmd = CMD_AUTOSKIP;
        subtp = 1;
    }
    else if(strnconsume(&str, "+autoskip", 9) == 0)
    {
        cmd = CMD_AUTOSKIP;
        subtp = -1;
    }
    else if(strnconsume(&str, "diffskip", 8) == 0)
    {
        cmd = CMD_DIFFSKIP;
        subtp = 1;
    }
    else if(strnconsume(&str, "+diffskip", 9) == 0)
    {
        cmd = CMD_DIFFSKIP;
        subtp = -1;
    }
    else if(strnconsume(&str, "text", 4) == 0)
    {
        cmd = CMD_TEXT;
        if(strnconsume(&str, "=ascii", 6) == 0)
            subtp = CODEPAGE_ASCII;
        else if(strnconsume(&str, "=ebcdic", 7) == 0)
            subtp = CODEPAGE_EBCDIC;
        else
            subtp = CODEPAGE_NIL;
    }
    else if(strnconsume(&str, "+text", 5) == 0)
    {
        cmd = CMD_TEXT;
        if(strnconsume(&str, "=ascii", 6) == 0)
            subtp = -CODEPAGE_ASCII;
        else if(strnconsume(&str, "=ebcdic", 7) == 0)
            subtp = -CODEPAGE_EBCDIC;
        else
            subtp = -CODEPAGE_NIL;
    }
    else if(strnconsume(&str, "ruler", 5) == 0)
    {
        cmd = CMD_RULER;
        subtp = 1;
    }
    else if(strnconsume(&str, "+ruler", 6) == 0)
    {
        cmd = CMD_RULER;
        subtp = -1;
    }

    if(cmd != CMD_NONE)
    {
        if(subtype)
            *subtype = subtp;
        if(post)
            *post = str;
    }
    return cmd;
}

/**
 * @brief Set 1 or all (per formode) of the entries in an array to a given value
 *
 * @param[in] arr Array of values, must be at least of size MODE_COUNT
 * @param[in] formode Index (< MODE_COUNT) in array whose value is to be set;
 *            if negative, set the whole array.
 * @param[in] value Value to set
 * @return Returns RC_USER if bad line/group width for corresponding arrays,
 *         otherwise RC_OK.
 */
static rc_t setModeVar(hoff_t *arr, int formode, hoff_t value)
{
    rc_t rc = RC_UNSPEC;

    if(arr == Params.mode_lines)
    {
        if(value > MAXW_LINE)
        {
            rc = RC_USER;
            prerr("line width may not exceed " MS(MAXW_LINE) " octets\n");
            goto end;
        }
    }
    if(arr == Params.mode_groups)
    {
        if(value > MAXW_GROUP)
        {
            rc = RC_USER;
            prerr("group width may not exceed " MS(MAXW_GROUP) " octets\n");
            goto end;
        }
    }

    if(formode < 0)
    {
        for(formode = 0; formode < MODE_COUNT; formode++)
            arr[formode] = value;
    }
    else
    {
        assert(formode < MODE_COUNT);
        arr[formode] = value;
    }

    rc = RC_OK;

end:
    return rc;
}

/**
 * @brief Process commands that can either be given as user commands or as
 *        command line flags.
 *
 * @param[in] cmd Command, e.g. one of the CMD_* constants
 * @param[in] subtype Subtype of command, e.g. endianb vs endianl, hexl vs hexu
 * @param[in] arg String argument, usually a size
 * @param[in] formode MODE_HEX or MODE_BITS for which to set a given value
 * @return Returns RC_OK if command processing completed fully, otherwise a
 *         hexpeek error code.
 */
rc_t processShared(int cmd, int subtype, char const *arg, int formode)
{
    rc_t rc = RC_UNSPEC;
    hoff_t tmph = 0;

    if(cmd == CMD_ENDIAN)
    {
        Params.endian_big = (subtype > 0);
    }
    else if(cmd == CMD_HEX)
    {
        Params.disp_mode = MODE_HEX;
        if(subtype)
            Params.hexlower = (subtype > 0);
    }
    else if(cmd == CMD_BITS)
    {
        Params.disp_mode = MODE_BITS;
    }
    else if(cmd == CMD_RLEN)
    {
        if((rc = strtosz(arg, &tmph)))
            goto end;
        if((rc = setModeVar(Params.mode_print_defs, formode, tmph)))
            goto end;
    }
    else if(cmd == CMD_SLEN)
    {
        if((rc = strtosz(arg, &tmph)))
            goto end;
        if((rc = setModeVar(Params.mode_search_defs, formode, tmph)))
            goto end;
    }
    else if(cmd == CMD_LINE)
    {
        if((rc = strtosz(arg, &tmph)))
            goto end;
        if((rc = setModeVar(Params.mode_lines, formode, tmph)))
            goto end;
    }
    else if(cmd == CMD_COLS)
    {
        if((rc = strtosz(arg, &tmph)))
            goto end;
        if((rc = setModeVar(Params.mode_lines, formode, tmph)))
            goto end;
        if((rc = setModeVar(Params.mode_print_defs, formode, tmph)))
            goto end;
        if((rc = setModeVar(Params.mode_search_defs, formode, tmph)))
            goto end;
    }
    else if(cmd == CMD_GROUP)
    {
        if((rc = strtosz(arg, &tmph)))
            goto end;
        if((rc = setModeVar(Params.mode_groups, formode, tmph)))
            goto end;
    }
    else if(cmd == CMD_MARGIN)
    {
        assert(arg);
        if(streq(arg, "full"))
        {
            Params.margin = HOFF_HEX_FULL_WIDTH;
        }
        else
        {
            rc = strtosz(arg, &tmph);
            checkrc(rc);
            // Argument is taken in octet length, but saved in char length
            if(tmph >= INT_MAX / MODE_CHCNT(MODE_HEX))
            {
                rc = RC_USER;
                prerr("excessive margin width\n");
                goto end;
            }
            Params.margin = MODE_CHCNT(MODE_HEX) * (int)tmph;
        }
    }
    else if(cmd == CMD_SCALAR)
    {
        assert(arg);
        if(streq(arg, MS(DEF_SCALAR_BASE)))
            Params.scalar_base = DEF_SCALAR_BASE;
        else if(streq(arg, "0"))
            Params.scalar_base = 0;
        else
        {
            rc = RC_USER;
            prerr("invalid argument to scalar\n");
            goto end;
        }
    }
    else if(cmd == CMD_PREFIX)
    {
        Params.print_prefix = (subtype > 0);
    }
    else if(cmd == CMD_AUTOSKIP)
    {
        Params.autoskip = (subtype > 0);
    }
    else if(cmd == CMD_DIFFSKIP)
    {
        Params.diffskip = (subtype > 0);
    }
    else if(cmd == CMD_TEXT)
    {
        Params.print_text = (subtype > 0);
        subtype = abs(subtype);
        if(subtype != CODEPAGE_NIL)
            Params.text_encoding = subtype;
    }
    else if(cmd == CMD_RULER)
    {
        Params.ruler = (subtype > 0);
    }
    else
    {
        rc = RC_NIL;
        goto end;
    }

    rc = RC_OK;

end:
    return rc;
}

/**
 * @brief Generate dump or diff command strings.
 *
 * @param[in] op 1 for dumps, 2 for diffs
 * @param[in] at String representing a file offset
 * @param[in] len String representing a filezone length
 */
void generateCommand(int op, char *at, char *len)
{
    traceEntry("%d, '%s', '%s'", op, at, len);

    assert(op > 0);
    assert( ! GeneratedCommand_mal);
    assert( ! Params.command);

    if( ! at)
        at = "0";

    size_t s_len = 0;
    s_len += 3;                      // "$0@"
    s_len += strlen(at);             // <AT>
    s_len += 1;                      // "," or ":"
    if(len)
        s_len += strlen(len);        // <LEN>
    else
        s_len += (sizeof(FZ_MAX)-1); // FZ_MAX
    s_len += 1;                      // "~" or "\0"

    s_len *= op;

    GeneratedCommand_mal = Malloc(s_len);

    for(int ix = 0; ix < op; ix++)
    {
        if(ix == 0)
            strcat(GeneratedCommand_mal, "$0@");
        else
            strcat(GeneratedCommand_mal, "~$1@");
        strcat(GeneratedCommand_mal, at);
        if(len)
        {
            strcat(GeneratedCommand_mal, ",");
            strcat(GeneratedCommand_mal, len);
        }
        else
            strcat(GeneratedCommand_mal, ":" FZ_MAX);
    }

    GeneratedCommand_mal[s_len - 1] = '\0';
    Params.command = GeneratedCommand_mal;

    traceExit("'%s'", Params.command);
}

/**
 * @brief Parse the provided string representation of a decimal file descriptor.
 *
 * @return Return a decimal string, or a negative value if any invalid input
 */
int parseDescriptor(char const *string)
{
    char *endptr = NULL;
    long tmpl = strtol(string, &endptr, 10); // always decimal
    if(endptr == string || *endptr != '\0' || tmpl < 0 || tmpl > INT_MAX)
        return -1;
    return (int)tmpl;
}

// Macros for simple, but unavoidable, repetition
#define advanceArgs() \
    if(ix + 1 >= argc) \
    { \
        rc = RC_USER; \
        prerr("missing argument to '%s'\n", argv[ix]); \
        goto end; \
    } \
    ix++;
#define setupDump() \
    subsequent_open_flags = O_RDONLY; \
    do_dump = true;
#define setupPack() \
    subsequent_open_flags = O_RDONLY; \
    Params.do_pack = true;
#define setupDiff() \
    subsequent_open_flags = O_RDONLY; \
    do_diff = true;

/**
 * @brief Parse main() style arguments, setting members of Params global
 *        variable as indicated. Note that members of Params not indicated by
 *        the provided arguments are not set.
 *
 * @param[in] argc Argument count
 * @param[in] argv Argument array (do not pass read-only strings!)
 * @return Returns RC_OK if arguments processing completed successfully,
 *         otherwise a hexpeek error code.
 */
rc_t parseArgv(int argc, char **argv)
{
    rc_t rc = RC_UNSPEC;
    int ix = 0, counter = 0;
    int subsequent_open_flags = -1, file_count = 0, pending = -1;
    bool flags_done = false, do_dump = false, do_diff = false;
    bool line_z = false, group_z = false;
    char *cmd_at = NULL, *cmd_len = NULL, *found = NULL;

    assert(argv);

    // Alternate invocations
    char const *invok = basename(argv[0]);
    if(streq(invok, viwNM))
    {
        subsequent_open_flags = O_RDONLY;
    }
    else if(streq(invok, DmpNM) || streq(invok, lstNM))
    {
        setupDump();
    }
    else if(streq(invok, pckNM))
    {
        setupPack();
    }
    else if(streq(invok, dffNM))
    {
        setupDiff();
    }

    // Flag parser
    for(ix = 1; ix < argc; )
    {
        bool isfd = false;
        if(flags_done)
            goto process_infile;

        rc = plugin_argv(argc, argv, &ix);
        if(rc == RC_OK)
            continue;
        else if(rc != RC_NIL)
            goto end;

        if(streq(argv[ix], "-dump") || streq(argv[ix], "-list"))
        {
            setupDump();
        }
        else if(streq(argv[ix], "-pack"))
        {
            setupPack();
        }
        else if(streq(argv[ix], "-diff"))
        {
            setupDiff();
        }
        else if(streq(argv[ix], "-s"))
        {
            advanceArgs();
            cmd_at = argv[ix];
        }
        else if(streq(argv[ix], "-l"))
        {
            advanceArgs();
            cmd_len = argv[ix];
        }
        else if(streq(argv[ix], "-r"))
        {
            subsequent_open_flags = O_RDONLY;
            pending = file_count;
        }
        else if(streq(argv[ix], "-w"))
        {
            subsequent_open_flags = O_RDWR | O_CREAT;
            pending = file_count;
        }
        else if(streq(argv[ix], "-W"))
        {
            subsequent_open_flags = O_RDWR;
            pending = file_count;
        }
        else if(streq(argv[ix], "-ik"))
        {
            Params.allow_ik = true;
        }
        else if(streq(argv[ix], "+ik"))
        {
            Params.allow_ik = false;
        }
        else if(streq(argv[ix], "-x"))
        {
            advanceArgs();
            Params.command = argv[ix];
        }
        else if(streq(argv[ix], "-o"))
        {
            int tmpfd = -1;
            advanceArgs();
            if(streq(argv[ix], "-d"))
            {
                advanceArgs();
                tmpfd = parseDescriptor(argv[ix]);
                if(tmpfd < 0)
                {
                    rc = RC_USER;
                    prerr("bad input to '-o -d'\n");
                    goto end;
                }
            }
            else
            {
                int o_flags = O_WRONLY | O_CREAT;
                if( ! Params.do_pack)
                    o_flags |= O_TRUNC;
                rc = hexpeek_open(argv[ix], o_flags, PERM, &tmpfd);
                if(rc)
                    goto end;
            }
            if(tmpfd != STDOUT_FILENO)
                assert(dup2(tmpfd, STDOUT_FILENO) == STDOUT_FILENO);
        }
        else if(streq(argv[ix], "+lineterm"))
        {
            Params.line_term = "";
        }
        else if(streq(argv[ix], "-format"))
        {
            advanceArgs();
            if(strnconsume((char const **)&argv[ix],
                           GroupFmtLiTern, strlen(GroupFmtLiTern)) == 0)
                Params.group_pre[0] = argv[ix] + 1;
            else
                Params.group_pre[0] = argv[ix];
            Params.group_pre[1] = argv[ix];
            char *first = NULL;
            for(found = argv[ix]; ; )
            {
                found = strchr(found, GroupFmtGroup[0]);
                if( ! found)
                    break;
                if(strncmp(found, GroupFmtGroup, strlen(GroupFmtGroup)) == 0)
                {
                    if(first)
                    {
                        rc = RC_USER;
                        prerr("duplicate '%s'\n", GroupFmtGroup);
                        goto end;
                    }
                    first = found;
                    found += strlen(GroupFmtGroup);
                }                
                else if(found[1] == '%')
                {
                    memmove(found, found + 1, strlen(found));
                    found++;
                }
                else
                {
                    rc = RC_USER;
                    prerr("unrecognized format specifier\n");
                    goto end;
                }
            }
            if( ! first)
            {
                rc = RC_USER;
                prerr("format string must contain '%s'\n", GroupFmtGroup);
                goto end;
            }
            *first = '\0';
            Params.group_term = first + strlen(GroupFmtGroup);
        }
        else if(streq(argv[ix], "-pedantic"))
        {
            Params.infer = false;
            Params.tolerate_eof = false;
        }
        else if(streq(argv[ix], "+pedantic"))
        {
            Params.infer = true;
            Params.tolerate_eof = true;
        }
        else if(streq(argv[ix], "-permissive"))
        {
            Params.permissive = true;
        }
        else if(streq(argv[ix], "+permissive"))
        {
            Params.permissive = false;
        }
        else if(streq(argv[ix], "-strict"))
        {
            Params.fail_strict = true;
        }
        else if(streq(argv[ix], "+strict"))
        {
            Params.fail_strict = false;
        }
        else if(streq(argv[ix], "-unique"))
        {
            Params.assume_unique_infiles = true;
        }
        else if(streq(argv[ix], "+tty"))
        {
            Params.assume_ttys = 0;
        }
        else if(streq(argv[ix], "-backup"))
        {
            advanceArgs();
            if(streq(argv[ix], "sync"))
                Params.backup_sync = true;
            else if(streq(argv[ix], "max"))
                Params.backup_depth = MAX_BACKUP_DEPTH;
            else
            {
                char *endptr = NULL;
                long tmpl = strtol(argv[ix], &endptr, Params.scalar_base);
                if(endptr != argv[ix] && *endptr == '\0' &&
                   tmpl >= 0 && tmpl <= MAX_BACKUP_DEPTH)
                {
                    Params.backup_depth = tmpl;
                }
                else
                {
                    rc = RC_USER;
                    prerr("invalid argument to -backup\n");
                    goto end;
                }
            }
        }
        else if(streq(argv[ix], "-recover"))
        {
            if(Params.recover_interactive)
            {
                rc = RC_USER;
                prerr("duplicate -recover flag\n");
                goto end;
            }
            Params.recover_interactive = true;
        }
        else if(streq(argv[ix], "-AutoRecover"))
        {
            if(Params.recover_auto)
            {
                rc = RC_USER;
                prerr("duplicate -AutoRecover flag\n");
                goto end;
            }
            Params.recover_auto = true;
        }
#ifdef HEXPEEK_TRACE
        else if(streq(argv[ix], "-trace"))
        {
            advanceArgs();
            Params.trace_fp = fopen(argv[ix], "w");
            if( ! Params.trace_fp)
            {
                rc = RC_CRIT;
                prerr("error opening file \"%s\": %s\n", cleanstring(argv[ix]),
                      strerror(errno));
                goto end;
            }
            trace("TRACE START\n");
            trace("Invocation:");
            for(int trace_idx = 0; trace_idx < argc; trace_idx++)
                fprintf(Params.trace_fp, " '%s'", argv[trace_idx]);
            fprintf(Params.trace_fp, "\n");
            trace("HOFF_MAX = " TRC_hoff "\n", trchoff(HOFF_MAX));
        }
#endif
        else if(streq(argv[ix], "-p"))
        {
            rc = processShared(CMD_COLS, 0, "0", -1);
            checkrc(rc);
            rc = processShared(CMD_GROUP, 0, "0", -1);
            checkrc(rc);
            Params.margin     = 0;
            Params.autoskip   = false;
            Params.diffskip   = false;
            Params.print_text = false;
            Params.ruler      = false;
        }
        else if(streq(argv[ix], "-d"))
        {
            advanceArgs();
            isfd = true;
            goto process_infile;
        }
        else if(streq(argv[ix], "--"))
        {
            flags_done = true;
        }
        else if((*argv[ix] == '-' || *argv[ix] == '+') && argv[ix][1] != '\0')
        {
            int tmpcmd = CMD_NONE, tmpst = 0;
            char const *postflag = NULL;
            if(*argv[ix] == '+')
                tmpcmd = ascertainShared(argv[ix], &tmpst, &postflag);
            else
                tmpcmd = ascertainShared(argv[ix] + 1, &tmpst, &postflag);
            if(tmpcmd == CMD_NONE)
            {
                // No match, try short flags
                if(streq(argv[ix], "-b"))
                    tmpcmd = CMD_BITS;
                else if(streq(argv[ix], "-c"))
                    tmpcmd = CMD_COLS;
                else if(streq(argv[ix], "-g"))
                    tmpcmd = CMD_GROUP;
            }
            if(tmpcmd == CMD_NONE)
            {
                rc = RC_USER;
                prerr("unrecognized flag '%s'\n", argv[ix]);
                goto end;
            }
            else if(postflag && *postflag != '\0')
            {
                rc = RC_USER;
                prerr("trailing text to setting flag\n");
                goto end;
            }
            else
            {
                char const *tmparg = NULL;
                switch(tmpcmd)
                {
                case CMD_RLEN:
                case CMD_SLEN:
                case CMD_LINE:
                case CMD_COLS:
                case CMD_GROUP:
                case CMD_MARGIN:
                case CMD_SCALAR:
                    advanceArgs();
                    tmparg = argv[ix];
                    break;
                }
                rc = processShared(tmpcmd, tmpst, tmparg, -1);
                checkrc(rc);
            }
        }
        else
        {
process_infile:
            if(file_count >= MAX_INFILES)
            {
                rc = RC_USER;
                prerr("too many infiles\n");
                goto end;
            }
            if(isfd)
            {
                // non-path infile from -d
                Params.infiles[file_count].fd = parseDescriptor(argv[ix]);
                if(Params.infiles[file_count].fd < 0)
                {
                    rc = RC_USER;
                    prerr("bad input to '-d'\n");
                    goto end;
                }
            }
            else
            {
                Params.infiles[file_count].path = argv[ix];
            }
            Params.infiles[file_count].open_flags = subsequent_open_flags;
            file_count++;
        }

        ix++;
    }

    // Check hanging flags
    if(pending >= file_count)
    {
        rc = RC_USER;
        prerr("-r, -w, or -W after infiles has no effect!\n");
        goto end;
    }

    // Special operations
    if((cmd_at || cmd_len) && ! do_diff)
        do_dump = true;
    if(Params.command)
        counter++;
    if(do_dump)
        counter++;
    if(do_diff)
        counter++;
    if(Params.do_pack)
        counter++;
    if(Params.recover_interactive || Params.recover_auto)
        counter++;
    if(counter > 1)
    {
        rc = RC_USER;
        prerr("more than one of -x, -dump / -list, -diff, -pack, and -recover"
              " specified\n");
        goto end;
    }
    if(do_dump)
    {
        if(file_count > 1)
        {
            rc = RC_USER;
            prerr("cannot dump more than one file\n");
            goto end;
        }
        generateCommand(1, cmd_at, cmd_len);
    }
    else if(Params.do_pack)
    {
        if(file_count > 1)
        {
            rc = RC_USER;
            prerr("cannot pack more than one input file\n");
            goto end;
        }
    }
    else if(do_diff)
    {
        if(file_count != 2)
        {
            rc = RC_USER;
            prerr("need two files to diff\n");
            goto end;
        }
        generateCommand(2, cmd_at, cmd_len);
    }

    // Recovery mode
    if(Params.recover_interactive && Params.recover_auto)
    {
        rc = RC_USER;
        prerr("-recover and -AutoRecover conflict\n");
        goto end;
    }
    if(Params.recover_interactive || Params.recover_auto)
    {
        if(file_count > 1)
        {
            rc = RC_USER;
            prerr("only one file can be recovered at a time\n");
            goto end;
        }
        if(Params.backup_depth > 0)
        {
            rc = RC_USER;
            prerr("Recovery mode and backup depth > 0 conflict\n");
            goto end;
        }
        Params.backup_depth = 0;
        if(Params.infiles[0].open_flags < 0)
        {
            Params.infiles[0].open_flags = O_RDWR;
        }
        else if(Params.infiles[0].open_flags != O_RDWR)
        {
            rc = RC_USER;
            prerr("Recovery mode requires write permission to data file\n");
            goto end;
        }
    }
    // Set default open flags if not given
    else
    {
        if(Params.backup_depth < 0)
            Params.backup_depth = DEFAULT_BACKUP_DEPTH;
        for(int fi = 0; fi < MAX_INFILES; fi++)
        {
            if(Params.infiles[fi].path)
            {
                if(Params.infiles[fi].open_flags < 0)
                {
                    if(access(Params.infiles[fi].path, F_OK) == 0 &&
                       access(Params.infiles[fi].path, W_OK))
                        Params.infiles[fi].open_flags = O_RDONLY;
                    else
                        Params.infiles[fi].open_flags = O_RDWR | O_CREAT;
                }
            }
            else if(Params.infiles[fi].open_flags < 0)
            {
                Params.infiles[fi].open_flags =
                    fcntl(Params.infiles[fi].fd, F_GETFL);
            }
        }
    }

    // Set default print format variables if not given
    if(Params.margin < 0)
        Params.margin = HOFF_HEX_DEFAULT_WIDTH;
    for(int md = 0; md < MODE_COUNT; md++)
    {
        hoff_t divisor = (do_diff ? 2 : 1);
        for(hoff_t guess = 0x20; guess > 1; guess /= 2)
        {
            if(totalWidth(md, guess) <= TERMINAL_WIDTH)
            {
                if(Params.mode_print_defs[md] < 0)
                    Params.mode_print_defs[md]  = guess / divisor;
                if(Params.mode_search_defs[md] < 0)
                    Params.mode_search_defs[md] = guess / divisor;
                if(Params.mode_lines[md] < 0)
                    Params.mode_lines[md]       = guess / divisor;
                break;
            }
        }
    }

    // Check if line or group width is zero
    for(int md = 0; md < MODE_COUNT; md++)
    {
        if(Params.mode_lines[md] == 0)
            line_z = true;
        if(Params.mode_groups[md] == 0)
            group_z = true;
    }

    // Set other defaults
    if(Params.autoskip < 0)
        Params.autoskip = interactive();
    if(Params.print_text < 0 && ! line_z)
        Params.print_text = interactive();
    if(Params.fail_strict < 0)
        Params.fail_strict = ! interactive();
#ifdef HEXPEEK_EDITABLE_CONSOLE
    if(Params.editable_console < 0)
        Params.editable_console = interactive();
#endif

    // Non-critical warnings
    if(line_z && Params.print_text)
        prwarn("zero line width disables text output\n");
    if(group_z && ! Params.endian_big)
        prwarn("zero group width disables little endian mode\n");

    rc = RC_OK;

end:
    if(rc && ix < argc)
    {
        prerr("error while processing argument '%s' at position %d\n",
              argv[ix], ix);
    }
    if(Params.mode_print_defs[MODE_HEX] < 0)
        Params.mode_print_defs[MODE_HEX]  = 0x20;
    if(Params.mode_search_defs[MODE_HEX] < 0)
        Params.mode_search_defs[MODE_HEX]  = 0x20;
    if(Params.mode_lines[MODE_HEX] < 0)
        Params.mode_lines[MODE_HEX]       = 0x20;
    if(Params.mode_print_defs[MODE_BITS] < 0)
        Params.mode_print_defs[MODE_BITS] = 0x8;
    if(Params.mode_search_defs[MODE_BITS] < 0)
        Params.mode_search_defs[MODE_BITS] = 0x8;
    if(Params.mode_lines[MODE_BITS] < 0)
        Params.mode_lines[MODE_BITS]      = 0x8;
    return rc;
}
