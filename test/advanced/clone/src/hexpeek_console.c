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

#define SRCNAME "hexpeek_console.c"

#include <hexpeek.h>

#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <errno.h>

#ifdef HEXPEEK_EDITABLE_CONSOLE

#include <histedit.h>

/**
 * @file hexpeek_console.c
 * @param Console support encapsulating optional libedit.
 */

static EditLine *ConsoleInfo = NULL;

static History *ConsoleHistory = NULL;

static char *consolePrompt(void *UNUSED)
{
    return PromptString;
}

#endif

/**
 * @brief Initialize the console (necessary for editable console support)
 */
void consoleInit()
{
#ifdef HEXPEEK_EDITABLE_CONSOLE
    if(Params.editable_console)
    {
        HistEvent UNUSED;

        ConsoleHistory = history_init();
        assert(ConsoleHistory);
        assert(history(ConsoleHistory, &UNUSED, H_SETSIZE, 64) >= 0);
        assert(history(ConsoleHistory, &UNUSED, H_SETUNIQUE, 1) >= 0);

        ConsoleInfo = el_init(PRGNM, stdin, stdout, stderr);
        assert(ConsoleInfo);
        assert(el_set(ConsoleInfo, EL_EDITOR, "emacs") == 0);
        assert(el_set(ConsoleInfo, EL_EDITMODE, 1) == 0);
        assert(el_set(ConsoleInfo, EL_PROMPT, consolePrompt) == 0);
        assert(el_set(ConsoleInfo, EL_SIGNAL, 1) == 0);
        assert(el_set(ConsoleInfo, EL_HIST, history, ConsoleHistory) == 0);

        el_source(ConsoleInfo, NULL);                      // @!!IGNORE_RESULT
        int editable = 0;
        if(el_get(ConsoleInfo, EL_EDITMODE, &editable) == 0 && editable == 0)
            consoleClose();
        else
            assert(atexit(consoleClose) == 0);
    }
#endif
}

/**
 * @brief Close the console (necessary for editable console support)
 */
void consoleClose()
{
#ifdef HEXPEEK_EDITABLE_CONSOLE
    if(ConsoleInfo)
    {
        el_end(ConsoleInfo);
        ConsoleInfo = NULL;
    }
    if(ConsoleHistory)
    {
        history_end(ConsoleHistory);
        ConsoleHistory = NULL;
    }
#endif
}

/**
 * @brief Flush the console
 */
void consoleFlush()
{
    assert(fflush(stdout) == 0);
}

/**
 * @brief Get console text input
 *
 * @return A malloc-d string with text input from the console
 */
char *consoleIn()
{
    if(interactive())
    {
#ifdef HEXPEEK_EDITABLE_CONSOLE
        if(ConsoleInfo)
        {
            HistEvent UNUSED;
            int count = 0;
            char const *result = el_gets(ConsoleInfo, &count);
            if(result == NULL || count <= 0)
                return NULL;
            assert(history(ConsoleHistory, &UNUSED, H_ENTER, result) >= 0);
            // Copy result because el_gets() returns a const string
            count++; // for '\0'
            if(LnInputSz < count)
            {
                if(LnInput_mal)
                    free(LnInput_mal);
                assert(LnInputSz > 0);
                for( ; LnInputSz < count; LnInputSz *= 2);
                LnInput_mal = Malloc(LnInputSz * sizeof *LnInput_mal);
            }
            assert(LnInput_mal);
            strncpy(LnInput_mal, result, LnInputSz);
            LnInput_mal[LnInputSz - 1] = '\0';
            return LnInput_mal;
        }
#endif
        console("%s", PromptString);
        consoleFlush();
    }

    if(getline(&LnInput_mal, &LnInputSz, stdin) <= 0)
        return NULL;
    return LnInput_mal;
}

#define VARIADIC_PRINTF(s, f) \
    va_list vl; \
    va_start(vl, (s)); \
    if(vprintf((f), vl) < 0) \
    { \
        prerr("error writing output, aborting\n"); \
        terminate(RC_CRIT); \
    } \
    va_end(vl);

#define VARIADIC_EXTRA_PRINTF(f) \
    char changed[strlen((f)) + 1]; \
    hexpeek_genf(changed, (f)); \
    VARIADIC_PRINTF((f), changed);

/**
 * @brief Print an ordinary format string to the console. This function does
 *        not handle the PRI_hxpkint format identifier.
 *
 * @param[in] format Format string
 * @param[in] ... Variadic arguments
 */
void console(char const *format, ...)
{
    VARIADIC_PRINTF(format, format);
}

/**
 * @brief Generate a format string involving the PRI_hxpk* format specifiers.
 *
 * @param[out] Output buffer for format string, must be at least as long as in,
 *             including NUL terminator.
 * @param[in] Input format string
 */
void hexpeek_genf(char *out, char const *in)
{
    char *cur = out;
    char const *last = in;
    for(char const *ptr = in;
        (ptr = strstr(ptr, PRI_hxpkint)) != NULL;
        last = ptr)
    {
        strncpy(cur, last, ptr - last);
        cur += (ptr - last);
        *(cur++) = (Params.hexlower ? 'x' : 'X');
        ptr += strlen(PRI_hxpkint);
    }
    strcpy(cur, last);
}

/**
 * @brief Print a format string possibly including PRI_hxpk* format specifiers.
 * This function can handle any format string, but it is only used for strings
 * requiring processing of PRI_hxpkint due to the performance degradation
 * resulting from the copying of the format string.
 *
 * @param[in] format Format string
 * @param[in] ... Variadic arguments
 */
void consoleOutf(char const *format, ...)
{
    VARIADIC_EXTRA_PRINTF(format);
}

/**
 * @brief Prompt the user for a response to a question.
 *
 * @param[in] format Format string
 * @param[in] ... Variadic arguments
 * @return Returns 0 if user answers 'y', else 1.
 */
int consoleAsk(char const *format, ...)
{
    VARIADIC_EXTRA_PRINTF(format);
    console(" ('y' or 'n')? ");
    consoleFlush();
    if(interactive())
    {
        if(getline(&LnInput_mal, &LnInputSz, stdin) <= 0)
            return 1;
        stripTrailingSpaces(LnInput_mal);
        return strcmp(LnInput_mal, "y");
    }
    return 1;
}
