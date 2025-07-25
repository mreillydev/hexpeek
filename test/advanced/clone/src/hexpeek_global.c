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

#define SRCNAME "hexpeek_global.c"

#include <hexpeek.h>

#include <stdlib.h>
#include <string.h>
#include <locale.h>
#include <errno.h>

/**
 * @file hexpeek_global.c
 * @brief Global values including lookup tables, malloc-d strings, and runtime
 *        settings.
 */

// Run-time constants set once by initialize()

/**
 * @brief Maximum value of an hoff_t set once by initialize().
 */
hoff_t HOFF_MAX = 0;

/**
 * @brief Mask lookup table created once by initialize().
 */
uintmax_t Masks[MASK_COUNT];

/**
 * @brief Character lookup table created once by initialize().
 */
uint8_t CharLookup[OCTET_COUNT];

// Global variables

/**
 * @brief Pointer to malloc()'d data containing result from generateCommand().
 */
char *GeneratedCommand_mal = NULL;

/**
 * @brief Pointer to malloc()'d data containing result from cleanstring().
 */
char *CleanString_mal = NULL;

/**
 * @brief Pointer to malloc()'d data containing result from consoleIn() or
 *        consoleAsk().
 */
char *LnInput_mal = NULL;

/**
 * @brief Size of LnInput_mal buffer.
 */
size_t LnInputSz = 0;

/**
 * @brief Global variable if backup files can be unlinked in closeFiles().
 */
bool BackupUnlinkAllowed = true;

/**
 * @brief Run parameters.
 */
Settings Params;

// Functions

/**
 * @brief Perform one time initialization of global variables.
 */
void initialize()
{
    assert(PRIXMAX[strlen(PRIXMAX) - 1] == 'X');           // @!!X_IN_PRIXMAX

    for(hoff_t flag = 1; flag > 0; flag <<= 1)
    {
        HOFF_MAX |= flag;
    }

    memset(Masks, 0, sizeof Masks);
    for(int ix = 0; ix < sizeof Masks / sizeof Masks[0]; ix++)
        for(int sub = 0; sub < ix; sub++)
            Masks[ix] |= ( ((uintmax_t)0xF) << (sub * 4) );

    memset(CharLookup, 0xFF, sizeof CharLookup);
    CharLookup['0']                   = 0x0;
    CharLookup['1']                   = 0x1;
    CharLookup['2']                   = 0x2;
    CharLookup['3']                   = 0x3;
    CharLookup['4']                   = 0x4;
    CharLookup['5']                   = 0x5;
    CharLookup['6']                   = 0x6;
    CharLookup['7']                   = 0x7;
    CharLookup['8']                   = 0x8;
    CharLookup['9']                   = 0x9;
    CharLookup['a'] = CharLookup['A'] = 0xA;
    CharLookup['b'] = CharLookup['B'] = 0xB;
    CharLookup['c'] = CharLookup['C'] = 0xC;
    CharLookup['d'] = CharLookup['D'] = 0xD;
    CharLookup['e'] = CharLookup['E'] = 0xE;
    CharLookup['f'] = CharLookup['F'] = 0xF;

    Settings_init(&Params);

    assert(atexit(cleanup) == 0);
    LnInputSz = 128;
    LnInput_mal = Malloc(LnInputSz);

    if( ! setlocale(LC_ALL, "C.UTF-8"))
        if( ! setlocale(LC_ALL, "C.utf8"))
            assert(setlocale(LC_ALL, "C"));
}

/**
 * @brief free() global variables
 */
void cleanup()
{
    for(int fi = 0; fi < MAX_INFILES; fi++)
    {
        if(Params.infiles[fi].name_mal)
        {
            free(Params.infiles[fi].name_mal);
            Params.infiles[fi].name_mal = NULL;
        }
        for(int bidx = 0; bidx < BACKUP_FILE_COUNT; bidx++)
        {
            if(Params.infiles[fi].bk_path_mal[bidx])
            {
                free(Params.infiles[fi].bk_path_mal[bidx]);
                Params.infiles[fi].bk_path_mal[bidx] = NULL;
            }
            if(Params.infiles[fi].bk_name_mal[bidx])
            {
                free(Params.infiles[fi].bk_name_mal[bidx]);
                Params.infiles[fi].bk_name_mal[bidx] = NULL;
            }
        }
    }
    if(GeneratedCommand_mal)
    {
        free(GeneratedCommand_mal);
        GeneratedCommand_mal = NULL;
    }
    if(CleanString_mal)
    {
        free(CleanString_mal);
        CleanString_mal = NULL;
    }
    if(LnInput_mal)
    {
        free(LnInput_mal);
        LnInput_mal = NULL;
    }
    closeTrace();
}
