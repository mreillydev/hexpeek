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

#define SRCNAME "hexpeek_constants.c"

#include <hexpeek.h>

/**
 * @file hexpeek_constants.c
 * @brief String constants and lookup tables for various data representations.
 */

//---------------------------- Basic Information -----------------------------//

char const LicenseString[] =
"Copyright 2020, 2025 Michael Reilly (mreilly@mreilly.dev).\n"
"\n"
"Redistribution and use in source and binary forms, with or without\n"
"modification, are permitted provided that the following conditions\n"
"are met:\n"
"1. Redistributions of source code must retain the above copyright\n"
"   notice, this list of conditions and the following disclaimer.\n"
"2. Redistributions in binary form must reproduce the above copyright\n"
"   notice, this list of conditions and the following disclaimer in the\n"
"   documentation and/or other materials provided with the distribution.\n"
"3. Neither the names of the copyright holders nor the names of the\n"
"   contributors may be used to endorse or promote products derived from\n"
"   this software without specific prior written permission.\n"
"\n"
"THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS\n"
"``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT\n"
"LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A\n"
"PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDERS\n"
"OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,\n"
"EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,\n"
"PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;\n"
"OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,\n"
"WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR\n"
"OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF\n"
"ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.\n"
;

#ifdef HEXPEEK_MODIFIED
    #define MOD_STRING HEXPEEK_MODIFIED 
#else
    #define MOD_STRING ""
#endif

#ifdef HEXPEEK_BETA
    #define BETA_STRING " [beta]"
#else
    #define BETA_STRING ""
#endif

char const VersionShort[] = HEXPEEK_PROGRAM_VERSION "\n";

#define VERSION_STRING \
    PRGNM MOD_STRING " version " HEXPEEK_PROGRAM_VERSION BETA_STRING "\n"

char const VersionLong[] =
VERSION_STRING
;

#define COPYRIGHT_SUMMARY \
"Copyright 2020, 2025 Michael Reilly." " ALL WARRANTIES DISCLAIMED.\n"

char const AuthorshipString[] =
COPYRIGHT_SUMMARY
"Do you like this software? Visit " HEXPEEK_URL " for more!\n"
;

char const UsageStringShort[] =
"usage: " PRGNM " [OPTIONS] INFILE0 [INFILE1]\n"
"  Open up to " MS(MAX_INFILES)
              " files and do as specified by options or enter prompt mode.\n"
"Basic options:\n"
"  -h , -help      Print short or long help text; then exit.\n"
"  -v , -version   Print short or long version string; then exit.\n"
"  -license        Print license text; then exit.\n"
"  -r              Open subsequent infiles read-only.\n"
"  -x <CMDS>       Execute semicolon-delimited commands.\n"
"  -dump , -list   Dump a whole single file in hexadecimal.\n"
"  -pack           Treat infile as a "PRGNM" dump and pack it back into binary.\n"
"  -diff           Diff two files in hexadecimal.\n"
"  -s <START>      With -dump or -diff, start output at given file offset.\n"
"  -l <LEN>        Like -s, but stop output after <LEN> octets are processed.\n"
"  -o <OUTFILE>    Write output to the given file.\n"
"  -b              Switch to bits display mode.\n"
"  -c <WIDTH>      Set column width.\n"
"  -g <WIDTH>      Set group width.\n"
"  -p              Output data in plain mode.\n"
"  -recover        Prompt to revert operations recorded in backup files.\n"
;

char const UsageStringLong[] =
"NAME\n"
"\n"
"    "PRGNM" - edit, dump, pack, and diff binary files in hex and bits\n"
"\n"
"SYNOPSIS\n"
"\n"
"    "PRGNM" [OPTIONS] INFILE0 [INFILE1]\n"
"    "viwNM" [OPTIONS] INFILE0 [INFILE1]\n"
"    "DmpNM" [OPTIONS] INFILE0 [INFILE1]\n"
"    "lstNM" [OPTIONS] INFILE0 [INFILE1]\n"
"    "pckNM" [OPTIONS] INFILE0 [INFILE1]\n"
"    "dffNM" [OPTIONS] INFILE0 [INFILE1]\n"
"\n"
"DESCRIPTION\n"
"\n"
"    "PRGNM" (TM) is a binary editor designed for efficient operation on huge\n"
"    files, but works on any file. It is not plagued by size-related crashes,\n"
"    freezes, and glitches because it does not attempt to map files into its\n"
"    memory; instead, it operates on files directly to fulfill user commands.\n"
"\n"
"    "PRGNM" has four main modes of operation: prompt, command, pack, and\n"
"    recovery. Prompt mode is entered by default, subject to OPTIONS. When\n"
"    invoked as "viwNM", "DmpNM", "lstNM", "pckNM", or "dffNM", "PRGNM" runs as\n"
"    if given one of the flags -r, -dump, -list, -pack, or -diff respectively.\n"
"\n"
"    Except when specifically indicated otherwise, "PRGNM" input and output are\n"
"    in unprefixed hexadecimal. This allows the user to work in hexadecimal\n"
"    without needing to prefix every number with \"0x\".\n"
"\n"
#ifdef HEXPEEK_BETA
"    This is an experimental beta release. Program functionality may change in\n"
"    future releases.\n"
"\n"
#endif
"    Thank you for trying "PRGNM". Send your comments to " HEXPEEK_EMAIL ".\n"
"\n"
"OPTIONS\n"
"\n"
"    Flags starting with \"+\" invert the respective flags with \"-\".\n"
"\n"
"    -h              Print usage text about commonly used options; then exit.\n"
"\n"
"    -help           Print more complete help text; then exit.\n"
"\n"
"    -v              Print short version string; then exit.\n"
"\n"
"    -version        Print long version string; then exit.\n"
"\n"
"    -license        Print license text; then exit.\n"
"\n"
"    -d <FD>         Treat <FD>, a _decimal_ integer, as an already-open file\n"
"                    descriptor and use it as the next file. Functionality on\n"
"                    non-seekable files is limited (see LIMITATIONS below).\n"
"\n"
"    -r              Open subsequent infiles read-only.\n"
"\n"
"    -w              Open subsequent infiles writeable.\n"
"\n"
"    -W              Like -w, but do not creat infiles that do not exist.\n"
"\n"
"    +ik             Disable insert and kill commands.\n"
"\n"
"    -x <CMDS>       Execute semicolon-delimited commands (see COMMANDS below).\n"
"\n"
"    -dump , -list   Dump a whole single infile. Same as \"-x 0:max\".\n"
"\n"
"    -pack           Treat infile as a "PRGNM" dump and pack it back into the\n"
"                    outfile as binary. For best results, run with the same\n"
"                    option flags with which the original dump was created.\n"
"\n"
"    -diff           Diff two files. Same as \"-x '$0@0:max~$1@0:max'\".\n"
"\n"
"    -s <START>      With -dump or -diff, start output at given file offset.\n"
"\n"
"    -l <LEN>        Like -s, but stop output after <LEN> octets are processed.\n"
"\n"
"    -o <OUTFILE>    Write output to the given file.\n"
"\n"
"    [-|+]SET [ARG]  Set the given setting (for all applicable display modes).\n"
"                    SET may be one of: endian, hex, bits, rlen, slen, line,\n"
"                    cols, group, margin, scalar, prefix, autoskip, diffskip,\n"
"                    text, and ruler. These options accept the same arguments\n"
"                    and have the same effect as the commands of the same names\n"
"                    (see COMMANDS below).\n"
"\n"
"    -b , -c , -g    Synonyms of -bits , -cols , and -group respectively.\n"
"\n"
"    -p              Output data in plain mode, which is the equivalent of\n"
"                    \"-c 0 -g 0 -margin 0 +autoskip +diffskip +text +ruler\".\n"
"\n"
"    +lineterm       Skip line breaks in output.\n"
"\n"
"    -format <FMT>   Specify group delimiters. This both controls print output\n"
"                    and allows the delimiters to be silently ignored in data\n"
"                    input. Default: \"" GroupFmtLiTern " "
                              GroupFmtGroup "\", which prints a leading space\n"
"                    before each group except the zeroth group (if margin > 0,\n"
"                    a space will be printed as part of the margin separator).\n"
"\n"
#ifdef HEXPEEK_UNIQUE_INFILES
"    -unique         Skip uniqueness check - assume all infiles are unique.\n"
"                    See warning in LIMITATIONS.\n"
"\n"
#endif
"    +tty            Assume standard streams are not terminals.\n"
"\n"
"    -pedantic       Generate a user-level error if filezone information is\n"
"                    unspecified or ambiguous (instead of auto-inferring what to\n"
"                    do) or if a print or diff (except with \":max\") attempts\n"
"                    to read beyond end of file (instead of printing nothing).\n"
"\n"
"    -permissive     Interpret \"diff\" as an insert command in interactive mode,\n"
"                    allow write commands with \"max\", and prompt user whether\n"
"                    to do recovery if a backup file is missing or inaccessible.\n"
"\n"
"    [-|+]strict     Toggle strict failure mode which, when enabled, causes\n"
"                    "PRGNM" to fail for user-level errors (like a malformed\n"
"                    command string). Enabled by default when "PRGNM" is run\n"
"                    non-interactively.\n"
"\n"
"    -backup <DEPTH> Backup depth may be 0 (to disable), max ("
                                            MS(MAX_BACKUP_DEPTH) "), or any\n"
"                    number therebetween. The default depth is "
                                      MS(DEFAULT_BACKUP_DEPTH) ".\n"
"\n"
"    -backup sync    Aggressively sync backup to disk.\n"
"\n"
"    -recover        Prompt to revert operations recorded in backup files.\n"
"\n"
#ifdef HEXPEEK_TRACE
"    -trace <FILE>   Trace to the given file.\n"
"\n"
#endif
"    --              Denote end of option flags.\n"
;

//------------------------------- Help Strings -------------------------------//

char const HelpCmdList[] =
"help <topic> shows helptext about a specific topic.\n"
"help -all shows helptext about all available topics.\n"
"Available help topics:\n"
"    quit, stop, help, files, reset, settings, endian, hex, bits, rlen, slen,\n"
"    line, cols, group, margin, scalar, prefix, autoskip, diffskip, text, ruler,\n"
"    Numeric, print, offset, search, ~, replace, insert, kill, ops, undo.\n"
;

char const HelpCmdHdr[] = "COMMANDS\n\n";

char const * HelpText[] =
{
"" // CMD_NONE
,
"    q[uit]          Quit the program.\n"
,
"    stop            Exit without unlinking backup files.\n"
,
"    h[elp] [TOPIC]  Show general or specific helptext.\n"
,
"    files           List open files with their paths (with C escapes for\n"
"                    non-printable characters), writeability, lengths, and\n"
"                    current offsets.\n"
,
"    reset [$FILE]   Reset the current offset of FILE if specified, otherwise\n"
"                    reset the current offsets of all open infiles.\n"
,
"    settings        List the values of various settings.\n"
,
"    endian<b|l>     Set big- (default) or little-endian mode. Little-endian\n"
"                    mode is not compatible with zero group width.\n"
,
"    hex[l|u]        Switch to hexadecimal display mode. The optional last\n"
"                    character may be used to set the case of hex numerals to\n"
"                    lower (default) or upper.\n"
,
"    bits            Switch to bits display mode. This affects file data only,\n"
"                    not scalar (control) information.\n"
,
"    rlen <WIDTH>    Set default read length for the current display mode.\n"
,
"    slen <WIDTH>    Set search print length for the current display mode.\n"
"                    If 0, print only address of search match.\n"
,
"    line <WIDTH>    Set line width for the current display mode; 0 disables.\n"
,
"    cols <WIDTH>    Set rlen, slen, and line.\n"
,
"    group <WIDTH>   Set group width for the current display mode; 0 disables.\n"
"                    This option controls how many octets are printed between\n"
"                    spaces (or other delimiters specified by -format).\n"
,

"    margin <WIDTH>  Set octet width of printed file offset; 0 disables; \"full\"\n"
"                    sets to maximum available width (default).\n"
,
"    scalar <BASE>   Interpret scalar input according to the given <BASE>. Valid\n"
"                    arguments are "
                  MS(DEF_SCALAR_BASE) " (default) and 0. If 0, scalar input are\n"
"                    interpreted as decimal unless prefixed with \"0x\" (hex) or\n"
"                    \"0\" (octal). This flag does not affect scalar output.\n"
,
"    [+]prefix       Print scalars with a \"0x\" prefix (default off).\n"
,
"    [+]autoskip     Toggle autoskip mode. If on (default when interactive),\n"
"                    repeated lines in dumps are replaced with \""
                                                  AutoskipOutput "\".\n"
,
"    [+]diffskip     Toggle diffskip mode. If on, identical lines in diffs are\n"
"                    skipped.\n"
,
"    [+]text[=CODE]  Toggle dump of text characters in a column to the right of\n"
"                    print output. The optional argument controls the character\n"
"                    encoding and should be ascii or ebcdic. Defaults on when\n"
"                    interactive and line width is non-zero.\n"
,
"    [+]ruler        Toggle octet ruler.\n"
,
"    Numeric         [+][$FILE@][HEXOFF][,HEXLEN][+][SUBCOMMAND]\n"
"                    [+][$FILE@][HEXOFF][:HEXLIM][+][SUBCOMMAND]\n"
"                       ^--------filezone-------^\n"
"\n"
"        Execute a subcommand over a given filezone. If specified, FILE must be\n"
"        a numeric index; otherwise, file $0 will be used.\n"
"\n"
"        HEXOFF may be used to specify:\n"
"                    1. an absolute file offset if positive,\n"
"                    2. a relative offset from file end if negative,\n"
"                    3. \"len\" (or \"-0\") for one past last known file byte, or\n"
"                    4. the current offset if \"@@\" or not given.\n"
"\n"
"        HEXLEN is an optional length argument to the subcommand.\n"
"\n"
"        HEXLIM specifies a non-inclusive upper bound, and may be:\n"
"                    1. numeric (equivalent to HEXLEN := HEXLIM - HEXOFF),\n"
"                    2. \"len\" (or \"-0\") for known file length, or\n"
"                    3. \"max\" to read maximum possible data.\n"
"        \"max\" may differ from \"len\" on non-regular files and is not allowed\n"
"        with write commands.\n"
"\n"
"        SUBCOMMAND may be one of: p, /, ~, r, i, k, their long forms, and\n"
"        offset. If no subcommand is specified, an implicit print is done.\n"
"\n"
"        If \"+\" precedes the filezone, file offset will be incremented before\n"
"        subcommand is run by the number of octets to be processed. If instead\n"
"        \"+\" follows, file offset will be incremented after subcommand is run\n"
"        by the number of octets processed.\n"
"\n"
"        An empty line at the prompt is equivalent to \"+\", and may be used to\n"
"        page through a file.\n"
,
"    p[rint][v] , v\n"
"\n"
"        Output data starting at file offset. If HEXLEN is specified, that many\n"
"        octets are read; otherwise, the default number of octets are read. The\n"
"        output includes a left margin with file offset information. When \"p\"\n"
"        is given explicitly, offset start is outputted before file data.\n"
"\n"
"        Including \"v\" prints verbosely: each output line shows the file offset\n"
"        and data for just one octet with hexadecimal, decimal, octal, bits,\n"
"        high bit/low bit/bit count, and text formats shown side-by-side.\n"
"\n"
"        If autoskip is enabled, repeated lines are replaced with a single \""
                                                             AutoskipOutput "\".\n"
,
"    offset\n"
"\n"
"        Seek to the filezone offset and print it. Useful in scripts.\n"
,
"    search <PATTERN> , /<PATTERN>\n"
"\n"
"        Search for the argument data within the specified filezone (or to file\n"
"        end if unspecified). A valid PATTERN is either:\n"
"            (1) fully specified octets in hexadecimal or bits (depending on\n"
"                mode), any number of spaces between octets, and the \".\"\n"
"                character (which  matches any value); or\n"
"            (2) a filezone of the form described above, in which case data\n"
"                from that zone is used as search input. If filezone length is\n"
"                unspecified, the default length of 1 is used.\n"
"\n"
"        If the search succeeds, the file offset is set to the beginning of the\n"
"        first found match; unless \"+\" follows the filezone, in which case the\n"
"        file offset is set to immediately _after_ the first found match or\n"
"        to immediately _after_ the search area if there was no match.\n"
,
"    ~[ ][FILEZONE]\n"
"\n"
"        Perform a diff of two filezones. If no argument is given and two files\n"
"        are open, the diff is done between the two files. If two octets at a\n"
"        given relative offset are the same, they are printed as underscores.\n"
"        If diffskip is enabled, identical lines are not printed.\n"
"\n"
"    /~[ ][FILEZONE]\n"
"\n"
"        Search for the next difference between two filezones.\n"
,
"    r[eplace ]<PATTERN>\n"
"\n"
"        Replace octets in the filezone with the argument data. The argument\n"
"        is of the same form as for the search command, but the \".\" matching\n"
"        character is not recognized. If HEXLEN is specified and is greater than\n"
"        the octet length of the input data, the data will be repeated to fill\n"
"        HEXLEN octets.\n"
,
"    i[nsert ]<PATTERN>\n"
"\n"
"        Like replace, but expand file at file offset by number of octets to\n"
"        be written, thus preserving existing data.\n"
,
"    k[ill] , delete\n"
"\n"
"        Remove the data in the specified filezone. If HEXLEN is unspecified,\n"
"        one octet will be removed. Note that a space is required between any\n"
"        numeric portion of the command and \"delete\".\n"
,
"    ops\n"
"\n"
"        Show operations available to be undone. For each operation the depth,\n"
"        operation number, and command string are printed.\n"
,
"    u[ndo] [DEPTH]\n"
"\n"
"        Undo the number of operations specified by DEPTH (defaults to 1).\n"
,
};

char const HelpOther[] =
"EXAMPLES\n"
"\n"
"    0               From beginning of file, print default number of octets.\n"
"\n"
"    10,40           Print 0x40 octets starting at file offset 0x10.\n"
"\n"
"    ,2p             Print two octets starting at current file offset.\n"
"\n"
"    0:max           Print out whole file.\n"
"\n"
"    7:1C/.1         Within domain starting at file offset 0x7 and ending at 0x1B\n"
"                    inclusively, search for the first octet the second nibble\n"
"                    of which is 1.\n"
"\n"
"    $0@0,10~$1@0,10 Diff the first 0x10 octets of two files.\n"
"\n"
"    0:100 r ff      Set the first 0x100 octets to 0xff.\n"
"\n"
"    0:len r 0       Zero out the whole file (final argument may be \"0\" or \"00\").\n"
"\n"
"    100 r 1122      Replace the octet at file offset 0x100 with value 0x11 and\n"
"                    the octet at file offset 0x101 with value 0x22.\n"
"\n"
"    -1r33           Replace the last octet in the file with the value 0x33.\n"
"\n"
"    i 44            Insert one octet with value 0x44 before the current offset.\n"
"\n"
"    len i 5566      Append two octets to file with values 0x55 and 0x66.\n"
"\n"
"    k               Remove one octet at the current file offset.\n"
"\n"
"    1:3k            Remove the first and second octets of the file.\n"
"\n"
"    -1k             Remove the last octet of the file.\n"
"\n"
"    20:60 r @30,3   Replace the 0x40 octet chunk starting at 0x20 with the\n"
"                    values located in 0x30 through 0x32 repeated.\n"
"\n"
"    3 i @0:len      Insert a copy of the whole file starting at offset 3.\n"
"\n"
"DEFAULTS\n"
"\n"
"    Unless set on the command line, column width defaults to the greatest\n"
"    power of 2 number of octets that fit in an "
                              MS(TERMINAL_WIDTH) " character terminal.\n"
"\n"
"LIMITATIONS\n"
"\n"
#ifdef HEXPEEK_UNIQUE_INFILES
"    "PRGNM" requires (and attempts to enforce) that each infile refers to a\n"
"    unique file. Data corruption may result if the same file is opened as\n"
"    multiple infiles during a "PRGNM" run.\n"
"\n"
#endif
"    Functionality on non-seekable files is inherently limited because "PRGNM"\n"
"    operates on them with a one-way seek. Thus, you can not seek backwards and\n"
"    post-incrementation for reads is always in effect. Moreover, the current\n"
"    offset has no impact on write operations (a duplex connection is assumed).\n"
"    Finally, the backup function does not work with non-seekable files for\n"
"    obvious reasons.\n"
"\n"
"    The insert and kill commands are inherently inefficient because they must\n"
"    move all the data after the point of insertion or deletion. Consider\n"
"    combining repeated insertions (or kills) into one large operation to limit\n"
"    the amount of time spent in file rearrangement.\n"
"\n"
"    Maximum line, group, and search argument octet width are "
#if (MAXW_LINE == MAXW_GROUP && MAXW_LINE == SRCHSZ)
     MS(MAXW_LINE) ".\n"
#else
     MS(MAXW_LINE) ", " MS(MAXW_GROUP) ",\n    and " MS(SRCHSZ) ", respectively.\n"
#endif
"\n"

"BACKUP AND RECOVERY\n"
"\n"
"    When in write mode, unless backup depth is 0, "PRGNM" creates "
                                             MS(BACKUP_FILE_COUNT) " hidden\n"
"    backup files with file extension " BACKUP_EXT ". Before executing any\n"
"    writeable command, "PRGNM" writes information to a backup file which is\n"
"    sufficient to recover previous data file state in case of program crash or\n"
"    user error.\n"
"\n"
"    When an error occurs, use the undo command to revert it; or use stop and\n"
"    then invoke '"PRGNM" -recover'. Otherwise, on successful exit, "PRGNM"\n"
"    automatically unlinks the backup files. A redo can be performed with the\n"
"    command line history functionality (if built with support).\n"
"\n"
"VERSION\n"
"\n"
"    " VERSION_STRING
"\n"
"AUTHOR\n"
"\n"
"    " COPYRIGHT_SUMMARY
"\n"
"    " "hexpeek is a trademark of Michael Reilly." "\n"
"\n"
"SEE ALSO\n"
"\n"
"    " HEXPEEK_URL "\n"
;

//----------------------------- Conversion Data ------------------------------//

char const *BinLookup_hexl[OCTET_COUNT] =
{
    "00",     "01",     "02",     "03",     "04",     "05",     "06",     "07",     "08",     "09",     "0a",     "0b",     "0c",     "0d",     "0e",     "0f",
    "10",     "11",     "12",     "13",     "14",     "15",     "16",     "17",     "18",     "19",     "1a",     "1b",     "1c",     "1d",     "1e",     "1f",
    "20",     "21",     "22",     "23",     "24",     "25",     "26",     "27",     "28",     "29",     "2a",     "2b",     "2c",     "2d",     "2e",     "2f",
    "30",     "31",     "32",     "33",     "34",     "35",     "36",     "37",     "38",     "39",     "3a",     "3b",     "3c",     "3d",     "3e",     "3f",
    "40",     "41",     "42",     "43",     "44",     "45",     "46",     "47",     "48",     "49",     "4a",     "4b",     "4c",     "4d",     "4e",     "4f",
    "50",     "51",     "52",     "53",     "54",     "55",     "56",     "57",     "58",     "59",     "5a",     "5b",     "5c",     "5d",     "5e",     "5f",
    "60",     "61",     "62",     "63",     "64",     "65",     "66",     "67",     "68",     "69",     "6a",     "6b",     "6c",     "6d",     "6e",     "6f",
    "70",     "71",     "72",     "73",     "74",     "75",     "76",     "77",     "78",     "79",     "7a",     "7b",     "7c",     "7d",     "7e",     "7f",
    "80",     "81",     "82",     "83",     "84",     "85",     "86",     "87",     "88",     "89",     "8a",     "8b",     "8c",     "8d",     "8e",     "8f",
    "90",     "91",     "92",     "93",     "94",     "95",     "96",     "97",     "98",     "99",     "9a",     "9b",     "9c",     "9d",     "9e",     "9f",
    "a0",     "a1",     "a2",     "a3",     "a4",     "a5",     "a6",     "a7",     "a8",     "a9",     "aa",     "ab",     "ac",     "ad",     "ae",     "af",
    "b0",     "b1",     "b2",     "b3",     "b4",     "b5",     "b6",     "b7",     "b8",     "b9",     "ba",     "bb",     "bc",     "bd",     "be",     "bf",
    "c0",     "c1",     "c2",     "c3",     "c4",     "c5",     "c6",     "c7",     "c8",     "c9",     "ca",     "cb",     "cc",     "cd",     "ce",     "cf",
    "d0",     "d1",     "d2",     "d3",     "d4",     "d5",     "d6",     "d7",     "d8",     "d9",     "da",     "db",     "dc",     "dd",     "de",     "df",
    "e0",     "e1",     "e2",     "e3",     "e4",     "e5",     "e6",     "e7",     "e8",     "e9",     "ea",     "eb",     "ec",     "ed",     "ee",     "ef",
    "f0",     "f1",     "f2",     "f3",     "f4",     "f5",     "f6",     "f7",     "f8",     "f9",     "fa",     "fb",     "fc",     "fd",     "fe",     "ff",
};

char const *BinLookup_hexu[OCTET_COUNT] =
{
    "00",     "01",     "02",     "03",     "04",     "05",     "06",     "07",     "08",     "09",     "0A",     "0B",     "0C",     "0D",     "0E",     "0F",
    "10",     "11",     "12",     "13",     "14",     "15",     "16",     "17",     "18",     "19",     "1A",     "1B",     "1C",     "1D",     "1E",     "1F",
    "20",     "21",     "22",     "23",     "24",     "25",     "26",     "27",     "28",     "29",     "2A",     "2B",     "2C",     "2D",     "2E",     "2F",
    "30",     "31",     "32",     "33",     "34",     "35",     "36",     "37",     "38",     "39",     "3A",     "3B",     "3C",     "3D",     "3E",     "3F",
    "40",     "41",     "42",     "43",     "44",     "45",     "46",     "47",     "48",     "49",     "4A",     "4B",     "4C",     "4D",     "4E",     "4F",
    "50",     "51",     "52",     "53",     "54",     "55",     "56",     "57",     "58",     "59",     "5A",     "5B",     "5C",     "5D",     "5E",     "5F",
    "60",     "61",     "62",     "63",     "64",     "65",     "66",     "67",     "68",     "69",     "6A",     "6B",     "6C",     "6D",     "6E",     "6F",
    "70",     "71",     "72",     "73",     "74",     "75",     "76",     "77",     "78",     "79",     "7A",     "7B",     "7C",     "7D",     "7E",     "7F",
    "80",     "81",     "82",     "83",     "84",     "85",     "86",     "87",     "88",     "89",     "8A",     "8B",     "8C",     "8D",     "8E",     "8F",
    "90",     "91",     "92",     "93",     "94",     "95",     "96",     "97",     "98",     "99",     "9A",     "9B",     "9C",     "9D",     "9E",     "9F",
    "A0",     "A1",     "A2",     "A3",     "A4",     "A5",     "A6",     "A7",     "A8",     "A9",     "AA",     "AB",     "AC",     "AD",     "AE",     "AF",
    "B0",     "B1",     "B2",     "B3",     "B4",     "B5",     "B6",     "B7",     "B8",     "B9",     "BA",     "BB",     "BC",     "BD",     "BE",     "BF",
    "C0",     "C1",     "C2",     "C3",     "C4",     "C5",     "C6",     "C7",     "C8",     "C9",     "CA",     "CB",     "CC",     "CD",     "CE",     "CF",
    "D0",     "D1",     "D2",     "D3",     "D4",     "D5",     "D6",     "D7",     "D8",     "D9",     "DA",     "DB",     "DC",     "DD",     "DE",     "DF",
    "E0",     "E1",     "E2",     "E3",     "E4",     "E5",     "E6",     "E7",     "E8",     "E9",     "EA",     "EB",     "EC",     "ED",     "EE",     "EF",
    "F0",     "F1",     "F2",     "F3",     "F4",     "F5",     "F6",     "F7",     "F8",     "F9",     "FA",     "FB",     "FC",     "FD",     "FE",     "FF",
};

char const *BinLookup_bits[OCTET_COUNT] =
{
    "00000000", "00000001", "00000010", "00000011", "00000100", "00000101", "00000110", "00000111", "00001000", "00001001", "00001010", "00001011", "00001100", "00001101", "00001110", "00001111",
    "00010000", "00010001", "00010010", "00010011", "00010100", "00010101", "00010110", "00010111", "00011000", "00011001", "00011010", "00011011", "00011100", "00011101", "00011110", "00011111",
    "00100000", "00100001", "00100010", "00100011", "00100100", "00100101", "00100110", "00100111", "00101000", "00101001", "00101010", "00101011", "00101100", "00101101", "00101110", "00101111",
    "00110000", "00110001", "00110010", "00110011", "00110100", "00110101", "00110110", "00110111", "00111000", "00111001", "00111010", "00111011", "00111100", "00111101", "00111110", "00111111",
    "01000000", "01000001", "01000010", "01000011", "01000100", "01000101", "01000110", "01000111", "01001000", "01001001", "01001010", "01001011", "01001100", "01001101", "01001110", "01001111",
    "01010000", "01010001", "01010010", "01010011", "01010100", "01010101", "01010110", "01010111", "01011000", "01011001", "01011010", "01011011", "01011100", "01011101", "01011110", "01011111",
    "01100000", "01100001", "01100010", "01100011", "01100100", "01100101", "01100110", "01100111", "01101000", "01101001", "01101010", "01101011", "01101100", "01101101", "01101110", "01101111",
    "01110000", "01110001", "01110010", "01110011", "01110100", "01110101", "01110110", "01110111", "01111000", "01111001", "01111010", "01111011", "01111100", "01111101", "01111110", "01111111",
    "10000000", "10000001", "10000010", "10000011", "10000100", "10000101", "10000110", "10000111", "10001000", "10001001", "10001010", "10001011", "10001100", "10001101", "10001110", "10001111",
    "10010000", "10010001", "10010010", "10010011", "10010100", "10010101", "10010110", "10010111", "10011000", "10011001", "10011010", "10011011", "10011100", "10011101", "10011110", "10011111",
    "10100000", "10100001", "10100010", "10100011", "10100100", "10100101", "10100110", "10100111", "10101000", "10101001", "10101010", "10101011", "10101100", "10101101", "10101110", "10101111",
    "10110000", "10110001", "10110010", "10110011", "10110100", "10110101", "10110110", "10110111", "10111000", "10111001", "10111010", "10111011", "10111100", "10111101", "10111110", "10111111",
    "11000000", "11000001", "11000010", "11000011", "11000100", "11000101", "11000110", "11000111", "11001000", "11001001", "11001010", "11001011", "11001100", "11001101", "11001110", "11001111",
    "11010000", "11010001", "11010010", "11010011", "11010100", "11010101", "11010110", "11010111", "11011000", "11011001", "11011010", "11011011", "11011100", "11011101", "11011110", "11011111",
    "11100000", "11100001", "11100010", "11100011", "11100100", "11100101", "11100110", "11100111", "11101000", "11101001", "11101010", "11101011", "11101100", "11101101", "11101110", "11101111",
    "11110000", "11110001", "11110010", "11110011", "11110100", "11110101", "11110110", "11110111", "11111000", "11111001", "11111010", "11111011", "11111100", "11111101", "11111110", "11111111",
};

//------------------------- Textual Representations --------------------------//

#define UNP '.'

static char asciiCodes[OCTET_COUNT] =
{
 UNP, UNP, UNP, UNP, UNP, UNP, UNP, UNP, UNP, UNP, UNP, UNP, UNP, UNP, UNP, UNP,
 UNP, UNP, UNP, UNP, UNP, UNP, UNP, UNP, UNP, UNP, UNP, UNP, UNP, UNP, UNP, UNP,
 ' ', '!', '"', '#', '$', '%', '&','\'', '(', ')', '*', '+', ',', '-', '.', '/',
 '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', ':', ';', '<', '=', '>', '?',
 '@', 'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O',
 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z', '[','\\', ']', '^', '_',
 '`', 'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o',
 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z', '{', '|', '}', '~', UNP,
 UNP, UNP, UNP, UNP, UNP, UNP, UNP, UNP, UNP, UNP, UNP, UNP, UNP, UNP, UNP, UNP,
 UNP, UNP, UNP, UNP, UNP, UNP, UNP, UNP, UNP, UNP, UNP, UNP, UNP, UNP, UNP, UNP,
 UNP, UNP, UNP, UNP, UNP, UNP, UNP, UNP, UNP, UNP, UNP, UNP, UNP, UNP, UNP, UNP,
 UNP, UNP, UNP, UNP, UNP, UNP, UNP, UNP, UNP, UNP, UNP, UNP, UNP, UNP, UNP, UNP,
 UNP, UNP, UNP, UNP, UNP, UNP, UNP, UNP, UNP, UNP, UNP, UNP, UNP, UNP, UNP, UNP,
 UNP, UNP, UNP, UNP, UNP, UNP, UNP, UNP, UNP, UNP, UNP, UNP, UNP, UNP, UNP, UNP,
 UNP, UNP, UNP, UNP, UNP, UNP, UNP, UNP, UNP, UNP, UNP, UNP, UNP, UNP, UNP, UNP,
 UNP, UNP, UNP, UNP, UNP, UNP, UNP, UNP, UNP, UNP, UNP, UNP, UNP, UNP, UNP, UNP,
};

static char const *asciiStrings[OCTET_COUNT] =
{
  "'\\0'",    "SOH",    "STX",    "ETX",    "EOT",    "ENQ",    "ACK",  "'\\a'",  "'\\b'",  "'\\t'",  "'\\n'",  "'\\v'",  "'\\f'",  "'\\r'",     "SO",     "SI",
    "DLE",    "DC1",    "DC2",    "DC3",    "DC4",    "NAK",    "SYN",    "ETB",    "CAN",     "EM",    "SUB",    "ESC",     "FS",     "GS",     "RS",     "US",
    "' '",    "'!'",   "'\"'",    "'#'",    "'$'",    "'%'",    "'&'",  "'\\''",    "'('",    "')'",    "'*'",    "'+'",    "','",    "'-'",    "'.'",    "'/'",
    "'0'",    "'1'",    "'2'",    "'3'",    "'4'",    "'5'",    "'6'",    "'7'",    "'8'",    "'9'",    "':'",    "';'",    "'<'",    "'='",    "'>'",    "'?'",
    "'@'",    "'A'",    "'B'",    "'C'",    "'D'",    "'E'",    "'F'",    "'G'",    "'H'",    "'I'",    "'J'",    "'K'",    "'L'",    "'M'",    "'N'",    "'O'",
    "'P'",    "'Q'",    "'R'",    "'S'",    "'T'",    "'U'",    "'V'",    "'W'",    "'X'",    "'Y'",    "'Z'",    "'['", "'\\\\'",    "']'",    "'^'",    "'_'",
    "'`'",    "'a'",    "'b'",    "'c'",    "'d'",    "'e'",    "'f'",    "'g'",    "'h'",    "'i'",    "'j'",    "'k'",    "'l'",    "'m'",    "'n'",    "'o'",
    "'p'",    "'q'",    "'r'",    "'s'",    "'t'",    "'u'",    "'v'",    "'w'",    "'x'",    "'y'",    "'z'",    "'{'",    "'|'",    "'}'",    "'~'",    "DEL",
     NULL,     NULL,     NULL,     NULL,     NULL,     NULL,     NULL,     NULL,     NULL,     NULL,     NULL,     NULL,     NULL,     NULL,     NULL,     NULL,
     NULL,     NULL,     NULL,     NULL,     NULL,     NULL,     NULL,     NULL,     NULL,     NULL,     NULL,     NULL,     NULL,     NULL,     NULL,     NULL,
     NULL,     NULL,     NULL,     NULL,     NULL,     NULL,     NULL,     NULL,     NULL,     NULL,     NULL,     NULL,     NULL,     NULL,     NULL,     NULL,
     NULL,     NULL,     NULL,     NULL,     NULL,     NULL,     NULL,     NULL,     NULL,     NULL,     NULL,     NULL,     NULL,     NULL,     NULL,     NULL,
     NULL,     NULL,     NULL,     NULL,     NULL,     NULL,     NULL,     NULL,     NULL,     NULL,     NULL,     NULL,     NULL,     NULL,     NULL,     NULL,
     NULL,     NULL,     NULL,     NULL,     NULL,     NULL,     NULL,     NULL,     NULL,     NULL,     NULL,     NULL,     NULL,     NULL,     NULL,     NULL,
     NULL,     NULL,     NULL,     NULL,     NULL,     NULL,     NULL,     NULL,     NULL,     NULL,     NULL,     NULL,     NULL,     NULL,     NULL,     NULL,
     NULL,     NULL,     NULL,     NULL,     NULL,     NULL,     NULL,     NULL,     NULL,     NULL,     NULL,     NULL,     NULL,     NULL,     NULL,     NULL,
};

static char ebcdicCodes[OCTET_COUNT] =
{
 UNP, UNP, UNP, UNP, UNP, UNP, UNP, UNP, UNP, UNP, UNP, UNP, UNP, UNP, UNP, UNP,
 UNP, UNP, UNP, UNP, UNP, UNP, UNP, UNP, UNP, UNP, UNP, UNP, UNP, UNP, UNP, UNP,
 UNP, UNP, UNP, UNP, UNP, UNP, UNP, UNP, UNP, UNP, UNP, UNP, UNP, UNP, UNP, UNP,
 UNP, UNP, UNP, UNP, UNP, UNP, UNP, UNP, UNP, UNP, UNP, UNP, UNP, UNP, UNP, UNP,
 ' ', UNP, UNP, UNP, UNP, UNP, UNP, UNP, UNP, UNP, UNP, '.', '<', '(', '+', '|',
 '&', UNP, UNP, UNP, UNP, UNP, UNP, UNP, UNP, UNP, '!', '$', '*', ')', ';', UNP,
 '-', '/', UNP, UNP, UNP, UNP, UNP, UNP, UNP, UNP, UNP, ',', UNP, '_', '>', '?',
 UNP, UNP, UNP, UNP, UNP, UNP, UNP, UNP, UNP, UNP, ':', '#', '@','\'', '=', '"',
 UNP, 'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', UNP, UNP, UNP, UNP, UNP, UNP,
 UNP, 'j', 'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', UNP, UNP, UNP, UNP, UNP, UNP,
 UNP, UNP, 's', 't', 'u', 'v', 'w', 'x', 'y', 'z', UNP, UNP, UNP, UNP, UNP, UNP,
 UNP, UNP, UNP, UNP, UNP, UNP, UNP, UNP, UNP, UNP, UNP, UNP, UNP, UNP, UNP, UNP,
 UNP, 'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', UNP, UNP, UNP, UNP, UNP, UNP,
 UNP, 'J', 'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', UNP, UNP, UNP, UNP, UNP, UNP,
'\\', UNP, 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z', UNP, UNP, UNP, UNP, UNP, UNP,
 '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', UNP, UNP, UNP, UNP, UNP, UNP,
};

static char const *ebcdicStrings[OCTET_COUNT] =
{
  "'\\0'",    "SOH",    "STX",    "ETX",    "SEL",  "'\\t'",    "RNL",    "DEL",     "GE",    "SPS",    "RPT",  "'\\v'",  "'\\f'",  "'\\r'",     "SO",     "SI",
    "DLE",    "DC1",    "DC2",    "DC3",    "RES",     "NL",  "'\\b'",    "POC",    "CAN",     "EM",    "UBS",    "CU1",    "IFS",    "IGS",    "IRS",    "ITB",
     "DS",    "SOS",     "FS",    "WUS",    "BYP",  "'\\n'",    "ETB",    "ESC",     "SA",    "SFE",     "SM",    "CSP",    "MFA",    "ENQ",    "ACK",  "'\\a'",
     NULL,     NULL,    "SYN",     "IR",     "PP",    "TRN",    "NBS",    "EOT",    "SBS",     "IT",    "RFF",    "CU3",    "DC4",    "NAK",     NULL,    "SUB",
    "' '",    "RSP",     NULL,     NULL,     NULL,     NULL,     NULL,     NULL,     NULL,     NULL,     NULL,    "'.'",    "'<'",    "'('",    "'+'",    "'|'",
    "'&'",     NULL,     NULL,     NULL,     NULL,     NULL,     NULL,     NULL,     NULL,     NULL,    "'!'",    "'$'",    "'*'",    "')'",    "';'",     NULL,
    "'-'",    "'/'",     NULL,     NULL,     NULL,     NULL,     NULL,     NULL,     NULL,     NULL,     NULL,    "','",     NULL,    "'_'",    "'>'",    "'?'",
     NULL,     NULL,     NULL,     NULL,     NULL,     NULL,     NULL,     NULL,     NULL,     NULL,    "':'",    "'#'",    "'@'",  "'\\''",    "'='",   "'\"'",
     NULL,    "'a'",    "'b'",    "'c'",    "'d'",    "'e'",    "'f'",    "'g'",    "'h'",    "'i'",     NULL,     NULL,     NULL,     NULL,     NULL,     NULL,
     NULL,    "'j'",    "'k'",    "'l'",    "'m'",    "'n'",    "'o'",    "'p'",    "'q'",    "'r'",     NULL,     NULL,     NULL,     NULL,     NULL,     NULL,
     NULL,     NULL,    "'s'",    "'t'",    "'u'",    "'v'",    "'w'",    "'x'",    "'y'",    "'z'",     NULL,     NULL,     NULL,     NULL,     NULL,     NULL,
     NULL,     NULL,     NULL,     NULL,     NULL,     NULL,     NULL,     NULL,     NULL,     NULL,     NULL,     NULL,     NULL,     NULL,     NULL,     NULL,
     NULL,    "'A'",    "'B'",    "'C'",    "'D'",    "'E'",    "'F'",    "'G'",    "'H'",    "'I'",     NULL,     NULL,     NULL,     NULL,     NULL,     NULL,
     NULL,    "'J'",    "'K'",    "'L'",    "'M'",    "'N'",    "'O'",    "'P'",    "'Q'",    "'R'",     NULL,     NULL,     NULL,     NULL,     NULL,     NULL,
 "'\\\\'",     NULL,    "'S'",    "'T'",    "'U'",    "'V'",    "'W'",    "'X'",    "'Y'",    "'Z'",     NULL,     NULL,     NULL,     NULL,     NULL,     NULL,
    "'0'",    "'1'",    "'2'",    "'3'",    "'4'",    "'5'",    "'6'",    "'7'",    "'8'",    "'9'",     NULL,     NULL,     NULL,     NULL,     NULL,     NULL,
};

static char unknownCodes[OCTET_COUNT] =
{
 UNP, UNP, UNP, UNP, UNP, UNP, UNP, UNP, UNP, UNP, UNP, UNP, UNP, UNP, UNP, UNP,
 UNP, UNP, UNP, UNP, UNP, UNP, UNP, UNP, UNP, UNP, UNP, UNP, UNP, UNP, UNP, UNP,
 UNP, UNP, UNP, UNP, UNP, UNP, UNP, UNP, UNP, UNP, UNP, UNP, UNP, UNP, UNP, UNP,
 UNP, UNP, UNP, UNP, UNP, UNP, UNP, UNP, UNP, UNP, UNP, UNP, UNP, UNP, UNP, UNP,
 UNP, UNP, UNP, UNP, UNP, UNP, UNP, UNP, UNP, UNP, UNP, UNP, UNP, UNP, UNP, UNP,
 UNP, UNP, UNP, UNP, UNP, UNP, UNP, UNP, UNP, UNP, UNP, UNP, UNP, UNP, UNP, UNP,
 UNP, UNP, UNP, UNP, UNP, UNP, UNP, UNP, UNP, UNP, UNP, UNP, UNP, UNP, UNP, UNP,
 UNP, UNP, UNP, UNP, UNP, UNP, UNP, UNP, UNP, UNP, UNP, UNP, UNP, UNP, UNP, UNP,
 UNP, UNP, UNP, UNP, UNP, UNP, UNP, UNP, UNP, UNP, UNP, UNP, UNP, UNP, UNP, UNP,
 UNP, UNP, UNP, UNP, UNP, UNP, UNP, UNP, UNP, UNP, UNP, UNP, UNP, UNP, UNP, UNP,
 UNP, UNP, UNP, UNP, UNP, UNP, UNP, UNP, UNP, UNP, UNP, UNP, UNP, UNP, UNP, UNP,
 UNP, UNP, UNP, UNP, UNP, UNP, UNP, UNP, UNP, UNP, UNP, UNP, UNP, UNP, UNP, UNP,
 UNP, UNP, UNP, UNP, UNP, UNP, UNP, UNP, UNP, UNP, UNP, UNP, UNP, UNP, UNP, UNP,
 UNP, UNP, UNP, UNP, UNP, UNP, UNP, UNP, UNP, UNP, UNP, UNP, UNP, UNP, UNP, UNP,
 UNP, UNP, UNP, UNP, UNP, UNP, UNP, UNP, UNP, UNP, UNP, UNP, UNP, UNP, UNP, UNP,
 UNP, UNP, UNP, UNP, UNP, UNP, UNP, UNP, UNP, UNP, UNP, UNP, UNP, UNP, UNP, UNP,
};

static char const *unknownStrings[OCTET_COUNT] =
{
"'\\x00'","'\\x01'","'\\x02'","'\\x03'","'\\x04'","'\\x05'","'\\x06'","'\\x07'","'\\x08'","'\\x09'","'\\x0A'","'\\x0B'","'\\x0C'","'\\x0D'","'\\x0E'","'\\x0F'",
"'\\x10'","'\\x11'","'\\x12'","'\\x13'","'\\x14'","'\\x15'","'\\x16'","'\\x17'","'\\x18'","'\\x19'","'\\x1A'","'\\x1B'","'\\x1C'","'\\x1D'","'\\x1E'","'\\x1F'",
"'\\x20'","'\\x21'","'\\x22'","'\\x23'","'\\x24'","'\\x25'","'\\x26'","'\\x27'","'\\x28'","'\\x29'","'\\x2A'","'\\x2B'","'\\x2C'","'\\x2D'","'\\x2E'","'\\x2F'",
"'\\x30'","'\\x31'","'\\x32'","'\\x33'","'\\x34'","'\\x35'","'\\x36'","'\\x37'","'\\x38'","'\\x39'","'\\x3A'","'\\x3B'","'\\x3C'","'\\x3D'","'\\x3E'","'\\x3F'",
"'\\x40'","'\\x41'","'\\x42'","'\\x43'","'\\x44'","'\\x45'","'\\x46'","'\\x47'","'\\x48'","'\\x49'","'\\x4A'","'\\x4B'","'\\x4C'","'\\x4D'","'\\x4E'","'\\x4F'",
"'\\x50'","'\\x51'","'\\x52'","'\\x53'","'\\x54'","'\\x55'","'\\x56'","'\\x57'","'\\x58'","'\\x59'","'\\x5A'","'\\x5B'","'\\x5C'","'\\x5D'","'\\x5E'","'\\x5F'",
"'\\x60'","'\\x61'","'\\x62'","'\\x63'","'\\x64'","'\\x65'","'\\x66'","'\\x67'","'\\x68'","'\\x69'","'\\x6A'","'\\x6B'","'\\x6C'","'\\x6D'","'\\x6E'","'\\x6F'",
"'\\x70'","'\\x71'","'\\x72'","'\\x73'","'\\x74'","'\\x75'","'\\x76'","'\\x77'","'\\x78'","'\\x79'","'\\x7A'","'\\x7B'","'\\x7C'","'\\x7D'","'\\x7E'","'\\x7F'",
"'\\x80'","'\\x81'","'\\x82'","'\\x83'","'\\x84'","'\\x85'","'\\x86'","'\\x87'","'\\x88'","'\\x89'","'\\x8A'","'\\x8B'","'\\x8C'","'\\x8D'","'\\x8E'","'\\x8F'",
"'\\x90'","'\\x91'","'\\x92'","'\\x93'","'\\x94'","'\\x95'","'\\x96'","'\\x97'","'\\x98'","'\\x99'","'\\x9A'","'\\x9B'","'\\x9C'","'\\x9D'","'\\x9E'","'\\x9F'",
"'\\xA0'","'\\xA1'","'\\xA2'","'\\xA3'","'\\xA4'","'\\xA5'","'\\xA6'","'\\xA7'","'\\xA8'","'\\xA9'","'\\xAA'","'\\xAB'","'\\xAC'","'\\xAD'","'\\xAE'","'\\xAF'",
"'\\xB0'","'\\xB1'","'\\xB2'","'\\xB3'","'\\xB4'","'\\xB5'","'\\xB6'","'\\xB7'","'\\xB8'","'\\xB9'","'\\xBA'","'\\xBB'","'\\xBC'","'\\xBD'","'\\xBE'","'\\xBF'",
"'\\xC0'","'\\xC1'","'\\xC2'","'\\xC3'","'\\xC4'","'\\xC5'","'\\xC6'","'\\xC7'","'\\xC8'","'\\xC9'","'\\xCA'","'\\xCB'","'\\xCC'","'\\xCD'","'\\xCE'","'\\xCF'",
"'\\xD0'","'\\xD1'","'\\xD2'","'\\xD3'","'\\xD4'","'\\xD5'","'\\xD6'","'\\xD7'","'\\xD8'","'\\xD9'","'\\xDA'","'\\xDB'","'\\xDC'","'\\xDD'","'\\xDE'","'\\xDF'",
"'\\xE0'","'\\xE1'","'\\xE2'","'\\xE3'","'\\xE4'","'\\xE5'","'\\xE6'","'\\xE7'","'\\xE8'","'\\xE9'","'\\xEA'","'\\xEB'","'\\xEC'","'\\xED'","'\\xEE'","'\\xEF'",
"'\\xF0'","'\\xF1'","'\\xF2'","'\\xF3'","'\\xF4'","'\\xF5'","'\\xF6'","'\\xF7'","'\\xF8'","'\\xF9'","'\\xFA'","'\\xFB'","'\\xFC'","'\\xFD'","'\\xFE'","'\\xFF'",
};

/**
 * @brief Convert a binary octet array into a text array in a given encoding.
 *
 * @param[in] codepage Either CODEPAGE_ASCII or CODEPAGE_EBCDIC
 * @param[in] in Binary octet input array
 * @param[in] len Length of in array
 * @param[out] out Output buffer 
 * @return Always returns pointer to out buffer.
 */
char *getEncoded(int codepage, uint8_t *in, size_t len, char *out)
{
    char *datasrc = NULL;
    switch(codepage)
    {
    case CODEPAGE_EBCDIC: datasrc = ebcdicCodes; break;
    case CODEPAGE_ASCII:  datasrc = asciiCodes; break;
    default:              datasrc = unknownCodes; break;
    }
    for(size_t ix = 0; ix < len; ix++)
        out[ix] = datasrc[in[ix]];
    out[len] = '\0';
    return out;
}

/**
 * @brief Return a symbolic string representing the provided character.
 *
 * @param[in] codepage Either CODEPAGE_ASCII or CODEPAGE_EBCDIC
 * @param[in] data Input character data
 * @return Symbolic string
 */
char const *getEncodedVerbose(int codepage, uint8_t data)
{
    char const *result = NULL;
    switch(codepage)
    {
    case CODEPAGE_EBCDIC: result = ebcdicStrings[data]; break;
    case CODEPAGE_ASCII:  result = asciiStrings[data]; break;
    }
    if(result)
        return result;
    else
        return unknownStrings[data];
}

/**
 * @brief Return an encoding name for a given codepage.
 *
 * @param[in] codepage Either CODEPAGE_EBCDIC or CODEPAGE_ASCII
 * @return Returns a textual name of the given codepage
 */
char const *encodingName(int codepage)
{
    switch(codepage)
    {
    case CODEPAGE_EBCDIC: return "Ebcdic";
    case CODEPAGE_ASCII:  return "Ascii";
    default:              return "UNKNOWN";
    }
}
