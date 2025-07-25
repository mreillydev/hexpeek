// Copyright 2020 Michael Reilly (mreilly@mreilly.dev).
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
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <assert.h>

#define flip_echo() \
    assert(read(sd, buf, datalen) == datalen); \
    for(int ix = 0; ix < datalen; ix++) \
        buf[ix] ^= 0xFF; \
    assert(write(sd, buf, datalen) == datalen);

// usage: <host> <port> <datalen>
int main(int argc, char **argv)
{
    assert(argc == 4);

    pid_t result = fork();
    if(result == 0)
    {
        int listen_sd = -1, sd = -1;
        struct sockaddr_in6 addr;
        socklen_t addrlen = sizeof addr;
        struct addrinfo hints, *res = NULL;
        char *host = argv[1], *port = argv[2];
        int datalen = atoi(argv[3]);

        assert(datalen > 0);
    
        memset(&addr, 0, sizeof addr);
        memset(&hints, 0, sizeof hints);
    
        hints.ai_family = AF_UNSPEC;
        hints.ai_socktype = SOCK_STREAM;
        assert(getaddrinfo(host, port, &hints, &res) == 0);
    
        listen_sd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
        assert(listen_sd >= 0);

        int flag = 1;
        assert(setsockopt(listen_sd, SOL_SOCKET, SO_REUSEADDR,
                          &flag, sizeof flag) == 0);

        assert(bind(listen_sd, res->ai_addr, res->ai_addrlen) == 0);
    
        assert(listen(listen_sd, 1) == 0);
    
        sd = accept(listen_sd, (struct sockaddr*)&addr, &addrlen);
        assert(sd >= 0);

        uint8_t buf[datalen];
        flip_echo();
        flip_echo();

        assert(close(sd) == 0);
        assert(close(listen_sd) == 0);
    }
    else if(result < 0)
    {
        fprintf(stderr, " ** fork() failed because %s", strerror(errno));
        exit(2);
    }
    exit(0);
}
