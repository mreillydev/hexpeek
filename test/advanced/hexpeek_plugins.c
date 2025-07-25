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

#define SRCNAME "hexpeek_plugins.c"

#include <hexpeek.h>

#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

// WARNING: THESE PLUGINS ARE FOR TESTING ONLY. DO NOT USE THEM ON A
// PRODUCTION BUILD.

// To use these plugins, copy this file to the top src/ directory and build
// hexpeek with HEXPEEK_PLUGINS defined.

// The following current invocations exist in the source:
//   plugin(0, ...) is called at program start
//   plugin(-1, ...) is called at program end
//   plugin(1, ...) is called after each writeable command
//   plugin(2, ...) is called in the middle of write loops
//   plugin(3, ...) is called before and after backup code writes
// The return code is not checked by any of the existing invocations.
// Plugin type domain (-10000, 10000) is considered reserved for future
// official hexpeek plugins.

#ifdef HEXPEEK_PLUGINS

//------------------------------- Plugin Data --------------------------------//

static char const *PluginData_Validator = NULL;

static uint64_t PluginData_ChangeCount = 0;

static int PluginData_DeathCount = -1;

static bool PluginData_AllowBackupDeath = false;

//--------------------------- Argument Interpreter ---------------------------//

rc_t plugin_argv(int argc, char **argv, int *which)
{
    // -Validator=<PATH_TO_VALIDATOR>
    // Use an external validation program to validate hexpeek operations.
    // dumpcmp is provided for this purpose; see the source for details on
    // arguments and operation.
    if(strncmp(argv[*which], "-Validator=", 11) == 0)
    {
        PluginData_Validator = argv[*which] + 11;
    }
    // -SimulateDeath=<COUNT>
    // Die after doing <COUNT> cycles of write loops, or at program end if
    // not hit previously.
    else if(strncmp(argv[*which], "-SimulateDeath=", 15) == 0)
    {
        PluginData_DeathCount = atoi(argv[*which] + 15);
        assert(PluginData_DeathCount > 0);
    }
    // -AllowBackupDeath
    // Include backup write loops in the above death count; otherwise death
    // will not deliberately be invoked while doing the backup itself.
    else if(strcmp(argv[*which], "-AllowBackupDeath") == 0)
    {
        PluginData_AllowBackupDeath = true;
    }
    // Process other arguments in primary settings processing.
    else
    {
        return RC_NIL;
    }

    (*which)++;
    return RC_OK;
}

//------------------------- Self-Validator Interface -------------------------//

static void validate(char const *command)
{
    if(PluginData_Validator)
    {
        assert(command && *command != '\0');
        char doclean = '1';
        if(strcmp(command, "-end") == 0)
            doclean = '2';
        else if(strcmp(command, "-start"))
            PluginData_ChangeCount++;
        size_t len_c = strlen(PluginData_Validator) + 3 + MAX_DEC + 1 +
                       1 + strlen(command) + 1 + 1;
        size_t len_t = len_c + 1;
        for(int fi = 0; fi < MAX_INFILES; fi++)
        {
            if(DT_PATH(fi))
                len_t += strlen(DT_PATH(fi)) + 1;
        }
        char *buf_mal = Malloc(len_t);
        memset(buf_mal, '\0', len_t);
        assert(snprintf(buf_mal, len_c, "%s %c %" PRIu64 " '%s'",
                        PluginData_Validator, doclean,
                        PluginData_ChangeCount, command) < len_c);
        for(int fi = 0; fi < MAX_INFILES; fi++)
        {
            if(DT_PATH(fi))
            {
                strcat(buf_mal, " ");
                strcat(buf_mal, DT_PATH(fi));
            }
        }
        buf_mal[len_t - 1] = '\0';
        int result = system(buf_mal);
        free(buf_mal);
        if(result)
        {
            fprintf(stderr, PGPRE "validator %s failed with %d!\n",
                    PluginData_Validator, result);
            die();
        }
    }
}

//---------------------------- Plugin Multiplexer ----------------------------//

void* plugin(int type, void *vp)
{
    static int saved_death_count = -1;
    switch(type)
    {
    case 0:
        validate("-start");
        break;
    case 1:
        validate(vp);
        break;
    case 2:
        if(PluginData_DeathCount > 0)
        {
            PluginData_DeathCount--;
            if(PluginData_DeathCount == 0)
                exit(60);
        }
        break;
    case 3:
        if( ! PluginData_AllowBackupDeath)
        {
            switch((int)vp)
            {
            case 0:
                saved_death_count = PluginData_DeathCount;
                PluginData_DeathCount = -1;
                break;
            case 1:
                PluginData_DeathCount = saved_death_count;
                saved_death_count = -1;
                break;
            }
        }
        break;
    case -1:
        validate("-end");
        if(PluginData_DeathCount > 0)
            exit(61);
        break;
    }
    return NULL;
}

#endif
