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

#ifndef _HEXPEEK_H_
#define _HEXPEEK_H_

#define _FILE_OFFSET_BITS 64

#include <util.h>

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <inttypes.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

/**
 * @file hexpeek.h
 * @brief Header for declarations shared between files.
 */

//----------------------------- Build Parameters -----------------------------//

// WARNING: hexpeek is a trademark. Modified redistributions under the same
// name should uncomment HEXPEEK_MODIFIED below. Feel free to add more
// information such as "modified by X" or "modified for Y". See the root
// project directory README for more information.
#define HEXPEEK_PROGRAM_NAME    "hexpeek"
//#define HEXPEEK_MODIFIED        " [modified]"
#define HEXPEEK_PROGRAM_VERSION "1.1.20250722"
#define HEXPEEK_URL             "https://www.hexpeek.com"
#define HEXPEEK_EMAIL           "hexpeek@hexpeek.com"

//#define HEXPEEK_BETA            Set by Makefile
//#define HEXPEEK_EDITABLE_CONSOLE

#define HEXPEEK_TRACE
//#define HEXPEEK_TRACE_EXTENDED
#define HEXPEEK_UNIQUE_INFILES
//#define HEXPEEK_ALWAYS_FILECPY
//#define HEXPEEK_PLUGINS

//---------------------------- Basic Definitions -----------------------------//

#define PRGNM HEXPEEK_PROGRAM_NAME
#define viwNM "hexview"
#define DmpNM "hexDump"
#define lstNM "hexlist"
#define pckNM "hexpack"
#define dffNM "hexdiff"

#define PAGESZ      0x1000
#define BUFSZ      0x10000
#define SRCHSZ       BUFSZ
#define MAXW_LINE    BUFSZ
#define MAXW_GROUP   BUFSZ

#define DEF_SCALAR_BASE  0x10

#define MAX_INFILES 2

#define BACKUP_FILE_COUNT 2

#define PERM (S_IRUSR | S_IWUSR)

#define TERMINAL_WIDTH 80

#define FZ_PREF "$@"
#define FZ_CTRL FZ_PREF ",:-"
#define FZ_LEN  "len"
#define FZ_MAX  "max"

//------------------------------- Return Codes -------------------------------//

#define RC_NIL     -1
#define RC_OK       0
#define RC_DIFF     1
#define RC_DONE     2
#define RC_UNSPEC   3 // any function that returns this has a bug
#define RC_USER     4
#define RC_CRIT     5

#define checkrc(r)  if((r) != RC_OK) goto end;

//--------------------------- Numeric Format Modes ---------------------------//

#define MODE_HEX      0
#define MODE_BITS     1
#define MODE_COUNT    2

#define HEX_CHCNT     2
#define BITS_CHCNT    8
#define MODE_CHCNT(m) ((m) ? BITS_CHCNT : HEX_CHCNT)
#define DISP_CHCNT    MODE_CHCNT(Params.disp_mode)

//----------------------------- Command Indices ------------------------------//

#define CMD_NONE        0
#define CMD_QUIT        1
#define CMD_STOP        2
#define CMD_HELP        3
#define CMD_FILES       4
#define CMD_RESET       5
#define CMD_SETTINGS    6
#define CMD_ENDIAN      7
#define CMD_HEX         8
#define CMD_BITS        9
#define CMD_RLEN       10
#define CMD_SLEN       11
#define CMD_LINE       12
#define CMD_COLS       13
#define CMD_GROUP      14
#define CMD_MARGIN     15
#define CMD_SCALAR     16
#define CMD_PREFIX     17
#define CMD_AUTOSKIP   18
#define CMD_DIFFSKIP   19
#define CMD_TEXT       20
#define CMD_RULER      21
#define CMD_NUMERIC    22
#define CMD_PRINT      23
#define CMD_OFFSET     24
#define CMD_SEARCH     25
#define CMD_DIFF       26
#define CMD_REPLACE    27
#define CMD_INSERT     28
#define CMD_KILL       29
#define CMD_OPS        30
#define CMD_UNDO       31
#define CMD_MIN        CMD_QUIT
#define CMD_MAX        CMD_UNDO

//----------------------------- Type Definitions -----------------------------//

typedef int rc_t;
#define TRC_rc "%d"

typedef off_t hoff_t;

#define HOFF_NIL -0x2F46 // little endian 2's complement looks like bad0ffff
#define HOFF_HEX_FULL_WIDTH 0x10
#define HOFF_HEX_DEFAULT_WIDTH HOFF_HEX_FULL_WIDTH

// Indirection for platforms where printf("%jX") is not recognized
#define PRI_hxpkint      "X" "%!!hxpknum"
#define PRI_hxpkmax  PRIXMAX "%!!hxpknum"                  // @!!X_IN_PRIXMAX

#define FORMAT_HOFF(n, x) \
    ((n) < 0 ? "-" : ""), (x), ((uintmax_t)(((n) < 0) ? -(n) : (n)))
#define HoffPrefix (Params.print_prefix ? "0x" : "")
#define PRI_hoff "%s%s%" PRI_hxpkmax
#define prihoff(n) FORMAT_HOFF(n, HoffPrefix)
#define prihcnt(n) FORMAT_HOFF(n, HoffPrefix), ((n) == 1 ? "" : "s")
#define TRC_hoff "%s%s%" PRIXMAX
#define trchoff(n) FORMAT_HOFF(n, "0x")

/**
 * @struct FileAttr
 *
 * @brief Struct tracking file attributes.
 *
 * @var FileAttr::path
 * Filesystem path this struct represents.
 * @var FileAttr::name_mal
 * Malloc()-d name to display to the user representing this file.
 * @var FileAttr::open_flags
 * Flags passed to open() in opening this file.
 * @var FileAttr::fd
 * File descriptor associated with this file.
 * @var FileAttr::at
 * Current file offset tracked by hexpeek.
 * @var FileAttr::last_at
 * Last file offset.
 * @var FileAttr::track
 * Internal tracking for non-seekable files.
 * @var FileAttr::created
 * Boolean for whether this file was created by this program run.
 * @var FileAttr::opcnt
 * Backup operation counter
 * @var FileAttr::bk_path_mal
 * Array of malloc()-d path generated for a backup file.
 * @var FileAttr::bk_name_mal
 * Array of malloc()-d name to display to the user representing this file.
 * @var FileAttr::bk_fds
 * Array of file descriptors associated with corresponding backup files.
 */
typedef struct
{
    char *path;
    char *name_mal;
    int open_flags;
    int fd;
    hoff_t at;
    hoff_t last_at;
    hoff_t track; // Internal tracking for non-seekable files
    bool created;
    uint64_t opcnt;
    char *bk_path_mal[BACKUP_FILE_COUNT];
    char *bk_name_mal[BACKUP_FILE_COUNT];
    int bk_fds[BACKUP_FILE_COUNT];
} FileAttr;

/**
 * @struct Settings
 *
 * @brief Container for global settings.
 *
 * @var Settings::scalar_base
 * Interpret scalar input according to the provided base (used as third
 * argument to strtol() type functions).
 * @var Settings::disp_mode
 * Current display mode of binary octet data (MODE_HEX or MODE_BITS).
 * @var Settings::hexlower
 * Lowercase vs uppercase hexadecimal characters ('abcdef' vs 'ABCDEF').
 * @var Settings::mode_print_defs
 * Array of default print lengths for each mode.
 * @var Settings::mode_search_defs
 * Array of default search lengths for each mode.
 * @var Settings::mode_lines
 * Array of line widths for each mode.
 * @var Settings::mode_groups
 * Array of group display sizes for each mode.
 * @var Settings::endian_big
 * Byte endian display order.
 * @var Settings::margin
 * Byte width of margin offset information (shown on left side).
 * @var Settings::autoskip
 * Toggle autoskip mode (if on, repeated lines in dumps are replaced with "*").
 * @var Settings::diffskip
 * Toggle diffskip mode (if on, identical lines in diffs are skipped).
 * @var Settings::line_term
 * Line terminator string (default "\n"). Can be used to remove newlines from
 * output completely.
 * @var Settings::group_pre
 * Array of pre-group delimiter strings (position 0 is for the zeroth group on
 * a line; position 1 is for subsequent groups).
 * @var Settings::group_term
 * Post-group delimiter string.
 * @var Settings::print_text
 * Output textual data (e.g. ASCII characters) to the right of octet output
 * @var Settings::text_encoding
 * Character encoding to use for textual output display (one of CODEPAGE_*).
 * @var Settings::ruler
 * Toggle showing a horizontal ruler above octet data display.
 * @var Settings::print_prefix
 * Toggle showing "0x" prefix before margin offset display.
 * @var Settings::allow_ik
 * Toggle insert / kill commands (which adjust file size) allowed.
 * @var Settings::infer
 * Toggle whether to infer filezones not fully explicitly specified.
 * @var Settings::tolerate_eof
 * Toggle default setting for whether display commands should fail if they
 * encounter EOF prematurely.
 * @var Settings::assume_unique_infiles
 * Toggle skipping infile uniqueness check.
 * @var Settings::assume_ttys
 * Toggle assuming stdin and stdout are terminals.
 * @var Settings::recover_interactive
 * Enter interactive recovery run mode on program start.
 * @var Settings::recover_auto
 * Enter automatic recovery run mode on program start.
 * @var Settings::backup_depth
 * Minimum number of operations to backup (0 disables backup mode).
 * @var Settings::backup_sync
 * Aggressively sync backup to disk.
 * @var Settings::permissive
 * Allow weird and potentially destructive commands.
 * @var Settings::fail_strict
 * Toggle exiting the program with an error code when RC_USER is generated
 * processing user input data.
 * @var Settings::editable_console
 * Toggle enabling libedit editable console runtime usage.
 * @var Settings::command
 * Enter command run mode on program start and execute command string.
 * @var Settings::do_pack
 * Enter -pack mode to turn a hex dump back into binary file data.
 * @var Settings::infiles
 * Array of FileAttr structs representing infiles specified on the command line.
 * @var Settings::trace_fp
 * File pointer to which to write trace data.
 */
typedef struct
{
    int scalar_base;
    int disp_mode;
    int hexlower;
    hoff_t mode_print_defs[MODE_COUNT];
    hoff_t mode_search_defs[MODE_COUNT];
    hoff_t mode_lines[MODE_COUNT];
    hoff_t mode_groups[MODE_COUNT];
    bool endian_big;
    int margin;
    int autoskip;
    bool diffskip;
    char const *line_term;
    char const *group_pre[2];
    char const *group_term;
    int print_text;
    int text_encoding;
    bool ruler;
    bool print_prefix;
    bool allow_ik;
    bool infer;
    bool tolerate_eof;
    bool assume_unique_infiles;
    int assume_ttys;
    bool recover_interactive;
    bool recover_auto;
    long backup_depth;
    bool backup_sync;
    int permissive;
    int fail_strict;
    int editable_console;
    char *command;
    bool do_pack;
    FileAttr infiles[MAX_INFILES];
    FILE *trace_fp;
} Settings;

void Settings_init(Settings *st);

#define FILE_INDEX_NIL   -1
#define FILE_INDEX_LATER -2

/**
 * @struct FileZone
 *
 * @brief Struct representing a zone (region) of an infile.
 *
 * @var FileZone::fi
 * Infile file index to Params.infiles array.
 * @var FileZone::start
 * File offset representing the start of this FileZone.
 * @var FileZone::len
 * Length representing the size of this FileZone.
 * @var FileZone::tolerate_eof
 * Toggle whether display commands on this FileZone should fail if they
 * encounter EOF prematurely.
 */ 
typedef struct
{
    int fi; // index to Params.infiles[]
    hoff_t start;
    hoff_t len;
    bool tolerate_eof;
} FileZone;

void FileZone_init(FileZone *zone);

/**
 * @struct ConvertedText
 *
 * @brief Struct representing user input data converted into a format to be
 *        used for hexpeek operations.
 *
 * @var ConvertedText::mem.sz
 * Actual allocated size of arrays octets_mal and masks_mal.
 * @var ConvertedText::mem.count
 * Operational size of data in arrays octets_mal and masks_mal.
 * @var ConvertedText::mem.octets_mal
 * Malloc()-d buffer containing binary octet data for operations.
 * @var ConvertedText::mem.masks_mal
 * Malloc()-d buffer containing masks data (e.g. for search regex).
 * @var ConvertedText::fz
 * FileZone associated with this struct.
 */
typedef struct
{
    struct
    {
        hoff_t sz;
        hoff_t count;
        uint8_t *octets_mal;
        uint8_t *masks_mal;
    } mem;
    FileZone fz;
} ConvertedText;

void ConvertedText_init(ConvertedText *converted_text);

/**
 * @struct ParsedCommand
 *
 * @brief User command broken down after parsing.
 *
 * @var ParsedCommand::origcmd
 * Original user command string text.
 * @var ParsedCommand::cmd
 * Command index (one of CMD_*).
 * @var ParsedCommand::subtype
 * Subtype parameter used by some commands.
 * @var ParsedCommand::fz
 * FileZone on which this command will operate.
 * @var ParsedCommand::incr_pre
 * Toggle incrementing the file offset before command execution.
 * @var ParsedCommand::incr_post
 * Toggle incrementing the file offset after command execution.
 * @var ParsedCommand::print_off
 * Explicitly print some information about file offset, length, etc.
 * @var ParsedCommand::print_verbose
 * Toggle verbose mode printing one octet per line with lots of information.
 * @var ParsedCommand::diff_srch
 * Toggle mode which searches for differences.
 * @var ParsedCommand::arg_t
 * Remaining user input string after main command was determined.
 * @var ParsedCommand::arg_cv
 * Remaining user input string converted to a ConvertedText structure.
 */
typedef struct
{
    char const *origcmd;
    int cmd;
    int subtype; // only used by shared commands
    FileZone fz;
    bool incr_pre;
    bool incr_post;
    bool print_off;
    bool print_verbose;
    bool diff_srch;
    char const *arg_t;
    ConvertedText arg_cv;
} ParsedCommand;

void ParsedCommand_init(ParsedCommand *parsed_command);

//------------------------------ Constant Data -------------------------------//

extern char const VersionShort[];
extern char const VersionLong[];

extern char const LicenseString[];
extern char const AuthorshipString[];
extern char const UsageStringShort[];
extern char const UsageStringLong[];

extern char const HelpCmdList[];
extern char const HelpCmdHdr[];
extern char const * HelpText[];
extern char const HelpOther[];

#define OCTET_COUNT 0x100

extern char const *BinLookup_hexl[OCTET_COUNT];

extern char const *BinLookup_hexu[OCTET_COUNT];

extern char const *BinLookup_bits[OCTET_COUNT];

#define CODEPAGE_ASCII  1
#define CODEPAGE_EBCDIC 2
#define CODEPAGE_NIL    3

char *getEncoded(int codepage, uint8_t *in, size_t len, char *out);

char const *getEncodedVerbose(int codepage, uint8_t data);

char const *encodingName(int codepage);

#define EofErrString "unexpectedly reached end of file while reading from %s\n"

//----------------- Global Variables and Run-Time Constants ------------------//

extern hoff_t HOFF_MAX;

#define MASK_COUNT (sizeof(uintmax_t) * HEX_CHCNT + 1)

extern uintmax_t Masks[MASK_COUNT];

extern uint8_t CharLookup[OCTET_COUNT];

extern char *GeneratedCommand_mal;

extern char *CleanString_mal;

extern char *LnInput_mal;

extern size_t LnInputSz;

extern bool BackupUnlinkAllowed;

extern Settings Params;

void initialize();

void cleanup();

//-------------------------------- Shortcuts ---------------------------------//

#define DT_PATH(i)     Params.infiles[i].path
#define DT_NAME(i)     Params.infiles[i].name_mal
#define DT_MODE(i)     Params.infiles[i].open_flags
#define DT_FD(i)       Params.infiles[i].fd
#define DT_AT(i)       Params.infiles[i].at
#define DT_OPCNT(i)    Params.infiles[i].opcnt

#define BK_PATH(i, j)  Params.infiles[i].bk_path_mal[j]
#define BK_NAME(i, j)  Params.infiles[i].bk_name_mal[j]
#define BK_FD(i, j)    Params.infiles[i].bk_fds[j]

#define DispMode       Params.disp_mode
#define DispPrDef      Params.mode_print_defs[Params.disp_mode]
#define DispSrchDef    Params.mode_search_defs[Params.disp_mode]
#define DispLine       Params.mode_lines[Params.disp_mode]
#define DispGroup      Params.mode_groups[Params.disp_mode]
#define BackupDepth    Params.backup_depth

//---------------------------- Output Formatting -----------------------------//

#define LineTerm       Params.line_term
#define GroupPre(li)   ((li) ? Params.group_pre[1] : Params.group_pre[0])
#define GroupTerm      Params.group_term

#define PromptString   "> "
#define MarginPost     ": "
#define GroupFmtGroup  "%_g"
#define GroupFmtLiTern "%_l?"
#define DiffSplit      "|"
#define AutoskipOutput "*"

//--------------------------------- Tracing ----------------------------------//

#ifdef HEXPEEK_TRACE
    #define trace(...)      tracef(Params.trace_fp, "" __VA_ARGS__)
    #define traceEntry(...) tracefEntry(Params.trace_fp, "" __VA_ARGS__)
    #define traceExit(...)  tracefExit(Params.trace_fp, "" __VA_ARGS__)
    #define closeTrace()    trace_close(Params.trace_fp)
#else
    #define trace(...)
    #define traceEntry(...)
    #define traceExit(...)
    #define closeTrace()
#endif

//------------------------------ Miscellaneous -------------------------------//

#define mod(a, b)            ( (b) ? (a) % (b) : (a) )

void *Malloc(size_t sz);

bool promptable();

bool interactive();

size_t slprintf(char *buf, char *lim, char const *format, ...);

rc_t strtooff(char const *str, char const **endptr, hoff_t *result, int infi);

rc_t strtosz(char const *str, hoff_t *result);

void endianize(uint8_t *buf, hoff_t len);

#define maxOctetWidth(l) ( ((l) / DISP_CHCNT) + (((l) % DISP_CHCNT) ? 1 : 0) )

rc_t textToOctetArray(char const *str, int mode, hoff_t *b_sz,
                      uint8_t *buf, uint8_t *masks);

char *cleanstring(char const *original_path);

void outputProgress(hoff_t complete, hoff_t total, int isbackup);

#ifdef HEXPEEK_EDITABLE_CONSOLE
    #define progress(c, t, i) outputProgress(c, t, i)
#else
    #define progress(c, t, i)
#endif

//------------------------------ Error Handling ------------------------------//

void terminate(int result);

void doErr(char const *file, int line, int op, char const *fmt, ...);

#define PGPRE PRGNM ": "

#define prerr(...) doErr(SRCNAME, __LINE__, 0, PGPRE __VA_ARGS__)
#define malcmd(...) doErr(SRCNAME, __LINE__, 0, PGPRE "malformed command: " __VA_ARGS__)
#define malnum(...) doErr(SRCNAME, __LINE__, 0, PGPRE "malformed number: " __VA_ARGS__)
#define prohibcmd(...) doErr(SRCNAME, __LINE__, 0, PGPRE "prohibited command: " __VA_ARGS__)

#define prwarn(...) doErr(SRCNAME, __LINE__, 1, PGPRE "warning: " __VA_ARGS__)

void doDie(char const *file, int line);

#define die() doDie(SRCNAME, __LINE__)

bool doCheck(char const *file, int line, char const *msg, bool exp);

#define assert(e) doCheck(SRCNAME, __LINE__, (#e), (e))

//------------------------------- Console I/O --------------------------------//

void consoleInit();

void consoleClose();

char *consoleIn();

void console(char const *format, ...);

void hexpeek_genf(char *out, char const *in);

void consoleOutf(char const *format, ...);

void consoleFlush();

int consoleAsk(char const *format, ...);

//--------------------------------- File I/O ---------------------------------//

char const *fdname(int fd);

rc_t hexpeek_open(char const *path, int flags, mode_t mode, int *fd);

hoff_t hexpeek_seek(int descriptor, hoff_t offset, int whence);

#define SAVE_OFFSET(d, o) \
    assert(((o) = hexpeek_seek((d), 0, SEEK_CUR)) != -1);

#define RESTORE_OFFSET(d, o) \
    assert(hexpeek_seek((d), (o), SEEK_SET) == (o));

rc_t seekto(int descriptor, hoff_t offset);

ssize_t readfull(int descriptor, void *buf, size_t count);

hoff_t hexpeek_read(int descriptor, void *buf, hoff_t count);

hoff_t hexpeek_write(int descriptor, const void *buf, hoff_t count);

rc_t hexpeek_stat(int descriptor, struct stat *fileinfo);

rc_t hexpeek_sync(int descriptor);

rc_t hexpeek_syncdir(char const *path);

rc_t hexpeek_truncate(int descriptor, hoff_t len);

int sameness(int fd0, int fd1);

bool isseekable(int file_index);

hoff_t filesize(int file_index);

hoff_t pathsize(char const *path);

rc_t readat(int descriptor, hoff_t at, void *buf, hoff_t count);

rc_t writeat(int descriptor, hoff_t at, const void *buf, hoff_t count);

rc_t filecpy(int src_fd, hoff_t src_at, hoff_t src_len,
             int dst_fd, hoff_t dst_at, hoff_t dst_len);

rc_t lclcpy(int fd, hoff_t src_at, hoff_t dst_at, hoff_t length);

rc_t adjustSize(int data_fi, hoff_t pos, hoff_t amt, int backup_fd);

//------------------------------- Backup File --------------------------------//

#define BACKUP_EXT PRGNM "-backup"

#define MAX_BACKUP_DEPTH  0x20
#define DEFAULT_BACKUP_DEPTH 8

int backupFd(int data_fi);

rc_t makeBackup(ParsedCommand const *ppc);

rc_t makeAdjBackup(int data_fi, int backup_fd, hoff_t sv_from);

rc_t clearAdjBackup(int backup_fd, void *vp);

rc_t recoverBackup(int data_fi, int what);

//--------------------------- Settings Processing ----------------------------//

hoff_t outputWidth(int part, int formode, hoff_t linewh);

int ascertainShared(char const *str, int *subtype, char const **post);

rc_t processShared(int cmd, int subtype, char const *arg, int formode);

rc_t parseArgv(int argc, char **argv);

//------------------------------- Test Plugins -------------------------------//

#ifdef HEXPEEK_PLUGINS
    rc_t plugin_argv(int argc, char **argv, int *which);
    void* plugin(int type, void *vp);
#else
    #define plugin_argv(a, v, w) RC_NIL
    #define plugin(t, v)
#endif

#endif
