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

#define SRCNAME "hexpeek_types.c"

#include <hexpeek.h>

#include <string.h>

/**
 * @file hexpeek_types.c
 * @brief Initializer functions for header structs.
 */

/**
 * @brief Initializer for Settings struct.
 *
 * @param[out] st Pointer to Settings struct to initialize.
 */
void Settings_init(Settings *st)
{
    assert(st);
    memset(st, 0, sizeof(Settings));
    st->scalar_base                 = DEF_SCALAR_BASE;
    st->disp_mode                   = MODE_HEX;
    st->hexlower                    = 1;
    st->mode_print_defs[MODE_HEX]   = -1;
    st->mode_print_defs[MODE_BITS]  = -1;
    st->mode_search_defs[MODE_HEX]  = -1;
    st->mode_search_defs[MODE_BITS] = -1;
    st->mode_lines[MODE_HEX]        = -1;
    st->mode_lines[MODE_BITS]       = -1;
    st->mode_groups[MODE_HEX]       = 0x4;
    st->mode_groups[MODE_BITS]      = 0x1;
    st->endian_big                  = true;
    st->margin                      = -1;
    st->autoskip                    = -1;
    st->diffskip                    = true;
    st->line_term                   = "\n";
    st->group_pre[0]                = "";
    st->group_pre[1]                = " ";
    st->group_term                  = "";
    st->print_text                  = -1;
    st->text_encoding               = CODEPAGE_ASCII;
    st->ruler                       = false;
    st->print_prefix                = false;
    st->allow_ik                    = true;
    st->infer                       = true;
    st->tolerate_eof                = true;
    st->assume_unique_infiles       = false;
    st->assume_ttys                 = -1;
    st->recover_interactive         = false;
    st->recover_auto                = false;
    st->backup_depth                = -1;
    st->backup_sync                 = false;
    st->permissive                  = false;
    st->fail_strict                 = -1;
#ifdef HEXPEEK_EDITABLE_CONSOLE
    st->editable_console            = -1;
#else
    st->editable_console            = false;
#endif
    st->command                     = NULL;
    st->do_pack                     = false;
    for(int fi = 0; fi < MAX_INFILES; fi++)
    {
        st->infiles[fi].path                  = NULL;
        st->infiles[fi].name_mal              = NULL;
        st->infiles[fi].open_flags            = -1;
        st->infiles[fi].fd                    = -1;
        st->infiles[fi].at                    = HOFF_NIL;
        st->infiles[fi].last_at               = HOFF_NIL;
        st->infiles[fi].track                 = 0;
        st->infiles[fi].created               = false;
        st->infiles[fi].opcnt                 = 0;
        for(int bidx = 0; bidx < BACKUP_FILE_COUNT; bidx++)
        {
            st->infiles[fi].bk_path_mal[bidx] = NULL;
            st->infiles[fi].bk_name_mal[bidx] = NULL;
            st->infiles[fi].bk_fds[bidx]      = -1;
        }
    }
    st->trace_fp                    = NULL;
}

/**
 * @brief Initializer for FileZone struct.
 *
 * @param[out] zone Pointer to FileZone struct to initialize.
 */
void FileZone_init(FileZone *zone)
{
    assert(zone);
    memset(zone, 0, sizeof(FileZone));
    zone->fi           = FILE_INDEX_NIL;
    zone->start        = HOFF_NIL;
    zone->len          = HOFF_NIL;
    zone->tolerate_eof = Params.tolerate_eof;
}

/**
 * @brief Initializer for ConvertedText struct.
 *
 * @param[out] converted_text Pointer to ConvertedText struct to initialize.
 */
void ConvertedText_init(ConvertedText *converted_text)
{
    assert(converted_text);
    memset(converted_text, 0, sizeof(ConvertedText));
    FileZone_init(&converted_text->fz);
}

/**
 * @brief Initializer for ParsedCommand struct.
 *
 * @param[out] parsed_command Pointer to ParsedCommand struct to initialize.
 */
void ParsedCommand_init(ParsedCommand *parsed_command)
{
    assert(parsed_command);
    memset(parsed_command, 0, sizeof(ParsedCommand));
    FileZone_init(&parsed_command->fz);
    ConvertedText_init(&parsed_command->arg_cv);
}
