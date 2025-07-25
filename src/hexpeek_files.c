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

#define SRCNAME "hexpeek_files.c"

#include <hexpeek.h>

#include <stdlib.h>
#include <libgen.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>


/**
 * @file hexpeek_files.c
 * @brief Filesystem API, mix of simple wrappers of OS APIs and more complex
 *        functions.
 */

/**
 * @brief Return hexpeek's file index given a file descriptor.
 *
 * @param[in] fd File descriptor
 * @return Corresponding file index, or a negative value if none found.
 */
static int whichfile(int fd)
{
    if(fd < 0)
        return -1;
    for(int fi = 0; fi < MAX_INFILES; fi++)
    {
        if(fd == Params.infiles[fi].fd)
            return fi;
    }
    return -1;
}

/**
 * @brief Return display name given a file descriptor.
 *
 * @param[in] fd File descriptor
 * @return String with name text to display.
 */
char const *fdname(int fd)
{
    if(fd >= 0)
    {
        for(int fi = 0; fi < MAX_INFILES; fi++)
        {
            if(Params.infiles[fi].fd >= 0)
            {
                if(fd == Params.infiles[fi].fd)
                    return DT_NAME(fi);
                for(int bidx = 0; bidx < BACKUP_FILE_COUNT; bidx++)
                {
                    if(fd == Params.infiles[fi].bk_fds[bidx])
                        return BK_NAME(fi, bidx);
                }
            }
        }
    }
    return "";
}

/**
 * @brief Return if this file descriptor is a hexpeek backup file descriptor.
 *
 * @param[in] fd File descriptor
 * @return Non-zero if true, zero if false
 */
static int isBackupFile(int fd)
{
    for(int fi = 0; fi < MAX_INFILES; fi++)
    {
        if(Params.infiles[fi].fd >= 0)
        {
            for(int bidx = 0; bidx < BACKUP_FILE_COUNT; bidx++)
            {
                if(fd == Params.infiles[fi].bk_fds[bidx])
                {
                    return 1;
                }
            }
        }
    }
    return 0;
}

/**
 * @brief Wrapper for open() with some error handling code.
 *
 * @param[in] path Filesystem path to open
 * @param[in] flags Flags to pass to open(), e.g. O_RDWR
 * @param[in] mode Mode to pass to open(), e.g. S_IRWXU
 * @param[out] fd Pointer in which to return opened file descriptor
 * @return Returns RC_OK on success, RC_CRIT otherwise
 */
rc_t hexpeek_open(char const *path, int flags, mode_t mode, int *fd)
{
    int tmpfd = open(path, flags, mode);
    if(tmpfd < 0)
    {
        prerr("error opening path \"%s\": %s\n", cleanstring(path),
              strerror(errno));
        return RC_CRIT;
    }
    *fd = tmpfd;
    return RC_OK;
}

#define _hexpeek_seek lseek

/**
 * @brief lseek() wrapper with error handling and non-seekable fallback support.
 *        If seek fails due to ESPIPE, hexpeek attempts an implicit seek using
 *        read().
 *
 * @param[in] fd File descriptor on which to perform a seek
 * @param[in] offset Numeric offset to pass to lseek()
 * @param[in] whence Whence to pass to lseek(), e.g. SEEK_SET
 * @return Returns lseek() type result
 */
hoff_t hexpeek_seek(int fd, hoff_t offset, int whence)
{
    errno = 0;
    hoff_t result = _hexpeek_seek(fd, offset, whence);
    if(result < 0) switch(errno)
    {
    case EINVAL:
    {
        prerr("invalid file offset\n");
        break;
    }
    case ESPIPE:
    {
        // Try a forward-only seek for non-seekable files
        int wf = whichfile(fd);
        if(wf >= 0 && whence == SEEK_SET && Params.infiles[wf].track <= offset)
        {
            uint8_t discard[PAGESZ];
            while(Params.infiles[wf].track < offset)
            {
                ssize_t lcl_rd = read(fd,
                                      discard,
                                      MIN(sizeof discard,
                                          offset - Params.infiles[wf].track));
                if(lcl_rd <= 0)
                    goto fallthrough;
                // Internal tracking for non-seekable files
                Params.infiles[wf].track += (hoff_t)lcl_rd;
            }
            result = Params.infiles[wf].track;
            break;
        }
    }
    default:
    {
fallthrough:
        prerr("error seeking in %s: %s\n", fdname(fd), strerror(errno));
        break;
    }
    }
    return result;
}

/**
 * @brief Return if seek works on this file index by testing a seek to current
 *        offset.
 *
 * @param[in] file_index Hexpeek file index to evaluate for seekability
 * @return Boolean value corresponding to seekability
 */
bool isseekable(int file_index)
{
    assert(file_index >= 0 && file_index < MAX_INFILES);
    return (_hexpeek_seek(DT_FD(file_index), 0, SEEK_CUR) >= 0);
}

/**
 * @brief Minimalistic seek wrapper fixed to whence = SEEK_SET.
 *
 * @param[in] fd File descriptor on which to perform the seek
 * @param[in] offset File offset to which to seek
 * @return RC_OK on success, RC_USER for an invalid offset, RC_CRIT otherwise
 */
rc_t seekto(int fd, hoff_t offset)
{
    // No current code path has offset < 0 here, but if it happens, fix it
    // rather than failing.
    int whence = (offset < 0 ? SEEK_END : SEEK_SET);
    if(hexpeek_seek(fd, offset, whence) >= 0)
        return RC_OK;
    else if(errno == EINVAL)
        return RC_USER;
    else
        return RC_CRIT;
}

/**
 * @brief See documentation for hexpeek_read(). This function is necessary
 *        because read() may read less than the desired size in certain cases,
 *        e.g. on streams.
 */
ssize_t readfull(int fd, void *buf, size_t count)
{
    int wf = whichfile(fd);
    ssize_t result = -1;
    size_t octets_read = 0;

    while(octets_read < count)
    {
        ssize_t lcl_rd = read(fd,
                              buf + octets_read,
                              count - octets_read);
        if(lcl_rd < 0)
            goto end;
        if(lcl_rd == 0)
            break;
        octets_read += lcl_rd;
    }

    result = (ssize_t)octets_read;

end:
    if(wf >= 0)
    {
        // Internal tracking for non-seekable files
        if(Params.infiles[wf].track > HOFF_MAX - octets_read)
            Params.infiles[wf].track = HOFF_MAX;
        else
            Params.infiles[wf].track += (hoff_t)octets_read;
    }
    return result;
}

/**
 * @brief Read from the provided file descriptor until count bytes have been
 *        read or EOF or error is returned.
 *
 * @param[in] fd File descriptor on which to perform the read
 * @param[out] buf Buffer to which to write data
 * @param[in] count Total cumulative size of data to read
 * @return Number of bytes read (can differ from count on EOF) or a negative
 *         value on error
 */
hoff_t hexpeek_read(int fd, void *buf, hoff_t count)
{
    if(count < 0 || (uintmax_t)count > (uintmax_t)SIZE_MAX)
        return (hoff_t)-1;
    if(count == 0)
        return 0;
    ssize_t result = readfull(fd, buf, (size_t)count);
    if(result < 0)
        prerr("error reading from %s: %s\n", fdname(fd), strerror(errno));
    return (hoff_t)result;
}

/**
 * @brief Call hexpeek_read() and additionally fail if the return code is
 *        anything other than exactly count bytes.
 */
static hoff_t readstrict(int fd, void *buf, hoff_t count)
{
    hoff_t result = hexpeek_read(fd, buf, count);
    if(result >= 0 && result != count)
    {
        prerr(EofErrString, fdname(fd));
        result = -1;
    }
    return result;
}

/**
 * @brief Wrapper for write() with additional error handling code.
 *
 * @param[in] fd File descriptor on which to perform the write
 * @param[in] buf Buffer from which to read data
 * @param[in] count Size of data to write
 * @return write() style result
 */
hoff_t hexpeek_write(int fd, const void *buf, hoff_t count)
{
    if(count < 0 || (uintmax_t)count > (uintmax_t)SIZE_MAX)
        return (hoff_t)-1;
    if(count == 0)
        return 0;
    ssize_t result = write(fd, buf, (size_t)count);
    if(result != count)
        prerr("error writing to %s: %s\n", fdname(fd), strerror(errno));
    return (hoff_t)result;
}

/**
 * @brief Wrapper for fstat() with additional error handling code.
 *
 * @param[in] fd File descriptor to stat
 * @param[out] fileinfo Pointer to struct stat to output file information
 * @return RC_OK on success, RC_CRIT if fstat() fails
 */
rc_t hexpeek_stat(int fd, struct stat *fileinfo)
{
    if(fstat(fd, fileinfo))
    {
        prerr("error retrieving file info for %s: %s\n", fdname(fd),
              strerror(errno));
        return RC_CRIT;
    }
    return RC_OK;
}

/**
 * @brief Wrapper for fsync() with additional error handling code.
 *
 * @param[in] fd File descriptor to sync
 * @return RC_OK on success, RC_CRIT if fsync() fails
 */
rc_t hexpeek_sync(int fd)
{
    if(fsync(fd))
    {
        prerr("error syncing %s: %s\n", fdname(fd), strerror(errno));
        return RC_CRIT;
    }
    return RC_OK;
}

/**
 * @brief Perform a sync on the directory containing the file specified by
 *        path.
 *
 * @param[in] path Path to a file in the directory to be synced (dirname() is
 *            called on path to determine the directory's path)
 * @return RC_OK on success, RC_CRIT if fsync() or open() fail
 */
rc_t hexpeek_syncdir(char const *path)
{
    rc_t rc  = RC_UNSPEC;
    const size_t len = strlen(path) + 1;
    char buf[len];

    strncpy(buf, path, len);
    buf[len - 1] = '\0';

    char *directory = dirname(buf);
    int fd = open(directory, O_RDONLY);
    if(fd < 0)
    {
        rc = RC_CRIT;
        prerr("error opening path \"%s\": %s\n", cleanstring(directory),
              strerror(errno));
        goto end;
    }

    rc = hexpeek_sync(fd);

end:
    if(fd >= 0)
        close(fd);
    return rc;
}

/**
 * @brief Wrapper for ftruncate() with additional error handling code.
 *
 * @param[in] fd File descriptor on which to call ftruncate()
 * @param[in] len Length to which to truncate
 * @return RC_OK on success, RC_CRIT if ftruncate() fails
 */
rc_t hexpeek_truncate(int fd, hoff_t len)
{
    if(ftruncate(fd, len))
    {
        prerr("error truncating %s: %s\n", fdname(fd), strerror(errno));
        return RC_CRIT;
    }
    return RC_OK;
}

/**
 * @brief Try to determine if two file descriptors point to the same underlying
 *        file.
 *
 * @param[in] fd0 First file descriptor
 * @param[in] fd1 Second file descriptor
 * @return Returns 0 if determined unique, 2 if determined same, 2 otherwise
 */
int sameness(int fd0, int fd1)
{
    int result = 2;

    if(fd0 == fd1)
    {
        result = 1;
    }
    else
    {
        struct stat i0, i1;
        if(hexpeek_stat(fd0, &i0))
            goto end;
        if(hexpeek_stat(fd1, &i1))
            goto end;
        result = (i0.st_dev == i1.st_dev && i0.st_ino == i1.st_ino) ? 1 : 0;
    }

end:
    return result;
}

/**
 * @brief Wrapper for hexpeek_stat().st_size
 *
 * @param[in] Hexpeek file index of file whose size is to be evaluated
 * @return File size (never returns a negative number - dies instead)
 */
hoff_t filesize(int file_index)
{
    struct stat info;
    assert(hexpeek_stat(DT_FD(file_index), &info) == RC_OK);
    assert(info.st_size >= 0);
    return info.st_size;
}

/**
 * @brief Wrapper for stat().st_size with additional error handling code.
 *
 * @param[in] path Path name of file whose size is to be evaluated
 * @return File size, -1 if stat() returns ENOENT, else die
 */
hoff_t pathsize(char const *path)
{
    struct stat info;
    errno = 0;
    if(stat(path, &info))
    {
        if(errno == ENOENT)
            return -1;
        prerr("error retrieving information about path \"%s\": %s\n",
              cleanstring(path), strerror(errno));
        die();
    }
    return info.st_size;
}

/**
 * @brief Seek to a given offset and read a specified amount of data. Fail if
 *        the full amount of data cannot be read.
 *
 * @param[in] fd File descriptor on which to read
 * @param[in] at Offset at which to perform the read
 * @param[out] buf Buffer to write data into
 * @param[in] count Number of bytes to be read
 * @return RC_OK on success, RC_CRIT otherwise
 */
rc_t readat(int fd, hoff_t at, void *buf, hoff_t count)
{
    if(hexpeek_seek(fd, at, SEEK_SET) != at)
        return RC_CRIT;
    if(readstrict(fd, buf, count) != count)
        return RC_CRIT;
    return RC_OK;
}

/**
 * @brief Seek to a given offset and write a specified amount of data. Fail if
 *        the specified amount of data cannot be written.
 *
 * @param[in] fd File descriptor on which to write
 * @param[in] at Offset at which to perform the write
 * @param[in] buf Buffer providing data to be written
 * @param[in] count Number of bytes to be written
 * @return RC_OK on success, RC_CRIT otherwise
 */
rc_t writeat(int fd, hoff_t at, const void *buf, hoff_t count)
{
    if(hexpeek_seek(fd, at, SEEK_SET) != at)
        return RC_CRIT;
    if(hexpeek_write(fd, buf, count) != count)
        return RC_CRIT;
    return RC_OK;
}

/**
 * @brief Copy data backwards between two file descriptors. This function is
 *        safe for overlapping file regions when src_at <= dst_at.
 *
 * @param[in] src_fd File descriptor from which to read data
 * @param[in] src_at Offset at which to begin reading
 * @param[in] dst_fd File descriptor to which to write data
 * @param[in] dst_at Offset at which to begin writing
 * @param[in] length Size of data region to be copied
 * @return RC_OK on success, readat() or writeat() rc otherwise
 */
static rc_t cpybk(int src_fd, hoff_t src_at, int dst_fd, hoff_t dst_at,
                  hoff_t length)
{
    rc_t rc = RC_UNSPEC;
    hoff_t sz = (src_at + length) % PAGESZ;
    uint8_t cpybuf[MAX(BUFSZ, PAGESZ)];

    if(sz == 0)
        sz = BUFSZ;

    for(hoff_t rel = length; rel > 0; sz = BUFSZ)
    {
        sz = MIN(sz, rel);
        rel -= sz;
        progress(length - rel, length, 0);
        rc = readat(src_fd, src_at + rel, cpybuf, sz);
        checkrc(rc);
        rc = writeat(dst_fd, dst_at + rel, cpybuf, sz);
        checkrc(rc);
        plugin(2, NULL);
    }

    progress(-1, length, 0);

    rc = RC_OK;

end:
    return rc;
}

/**
 * @brief Copy data forwards between two file descriptors. This function is
 *        safe for overlapping file regions when src_at >= dst_at.
 *
 * @param[in] src_fd File descriptor from which to read data
 * @param[in] src_at Offset at which to begin reading
 * @param[in] dst_fd File descriptor at which to write data
 * @param[in] dst_at Offset at which to begin writing
 * @param[in] length Size of data region to be copied
 */
static rc_t cpyfw(int src_fd, hoff_t src_at, int dst_fd, hoff_t dst_at,
                  hoff_t length)
{
    rc_t rc = RC_UNSPEC;
    hoff_t sz = distbound(src_at, PAGESZ);
    uint8_t cpybuf[MAX(BUFSZ, PAGESZ)];

    for(hoff_t rel = 0; rel < length; sz = BUFSZ)
    {
        sz = MIN(sz, length - rel);
        progress(rel, length, 0);
        rc = readat(src_fd, src_at + rel, cpybuf, sz);
        checkrc(rc);
        rc = writeat(dst_fd, dst_at + rel, cpybuf, sz);
        checkrc(rc);
        rel += sz;
        plugin(2, NULL);
    }

    progress(-1, length, 0);

    rc = RC_OK;

end:
    return rc;
}

/**
 * @brief Copy data between two file descriptors that point to different files.
 *        If the file descriptors point to the same file, data corruption may
 *        result. This function exists because the distinct file requirement
 *        allows less seeking than is done by the functions above.
 *
 * @param[in] src_fd File descriptor from which to read data
 * @param[in] src_at Offset at which to begin reading
 * @param[in] dst_fd File descriptor at which to write data
 * @param[in] dst_at Offset at which to begin writing
 * @param[in] length Size of data region to be copied
 * @param[in] isbk Boolean if exactly one of the file descriptors is a backup
 *            file (e.g. if this copy represents a backup operation). This value
 *            is only used for displaying informational output to the user and
 *            does not affect file operations.
 * @return Returns RC_OK if the copy operation completed fully, otherwise a
 *         hexpeek error code.
 */
static rc_t cpyext(int src_fd, hoff_t src_at, int dst_fd, hoff_t dst_at,
                   hoff_t length, int isbk)
{
    rc_t rc = RC_UNSPEC;
    hoff_t sz = distbound(src_at, PAGESZ);
    uint8_t cpybuf[MAX(BUFSZ, PAGESZ)];

    rc = seekto(src_fd, src_at);
    checkrc(rc);
    rc = seekto(dst_fd, dst_at);
    checkrc(rc);

    for(hoff_t rel = 0; rel < length; sz = BUFSZ)
    {
        sz = MIN(sz, length - rel);
        progress(rel, length, isbk);
        if(readstrict(src_fd, cpybuf, sz) != sz)
        {
            rc = RC_CRIT;
            goto end;
        }
        if(hexpeek_write(dst_fd, cpybuf, sz) != sz)
        {
            rc = RC_CRIT;
            goto end;
        }
        rel += sz;
        plugin(2, NULL);
    }

    progress(-1, length, isbk);

    rc = RC_OK;

end:
    return rc;
}

/**
 * @brief Copy data between file descriptors at specified file offsets. The
 *        file regions may overlap. This function allows for repeated copying
 *        of the same data if src_len is less than dst_len.
 *
 * @param[in] src_fd File descriptor from which to read data
 * @param[in] src_at File offset at which to begin reading
 * @param[in] src_len Length of data to read
 * @param[in] dst_fd File descriptor to which to write data
 * @param[in] dst_at File offset at which to begin writing
 * @param[in] dst_len Length of data to write
 * @return Returns RC_OK if the copy operation completed fully, otherwise a
 *         hexpeek error code.
 */
rc_t filecpy(int src_fd, hoff_t src_at, hoff_t src_len,
             int dst_fd, hoff_t dst_at, hoff_t dst_len)
{
    rc_t rc = RC_UNSPEC;
    hoff_t src_before = -1, dst_before = -1, cpy_tot = 0;

    traceEntry("%s, " TRC_hoff ", " TRC_hoff ", "
               "%s, " TRC_hoff ", " TRC_hoff,
               fdname(src_fd), trchoff(src_at), trchoff(src_len),
               fdname(dst_fd), trchoff(dst_at), trchoff(dst_len));

    assert(src_fd >= 0);
    assert(dst_fd >= 0);
    assert(src_at >= 0);
    assert(src_len >= 0);
    assert(dst_at >= 0);
    assert(dst_len >= 0);
    assert(src_len <= dst_len);

    SAVE_OFFSET(src_fd, src_before);
    SAVE_OFFSET(dst_fd, dst_before);

    bool isbk = isBackupFile(src_fd) ^ isBackupFile(dst_fd);
    bool uniq = isbk || (sameness(src_fd, dst_fd) == 0);

    if(uniq)
    {
        // Copy src_len octets into start of dst_at
        rc = cpyext(src_fd, src_at, dst_fd, dst_at, src_len, isbk);
        checkrc(rc);
        cpy_tot += src_len;

        // Repeated write if needed
        while(cpy_tot < dst_len)
        {
            hoff_t cpy_len = MIN(dst_len - cpy_tot, src_len);
            rc = cpyext(src_fd, src_at, dst_fd, dst_at+cpy_tot, cpy_len, isbk);
            checkrc(rc);
            cpy_tot += cpy_len;
        }
    }
    else
    {
        // Copy src_len octets into start of dst_at
        if(src_at < dst_at && src_at + src_len > dst_at)
        {
            rc = cpybk(src_fd, src_at, dst_fd, dst_at, src_len);
            checkrc(rc);
        }
        else
        {
            rc = cpyfw(src_fd, src_at, dst_fd, dst_at, src_len);
            checkrc(rc);
        }
        cpy_tot += src_len;
    
        // Repeated write if needed (data at src_at may have been overwritten
        // depending on overlap, so read from dst_at).
        while(cpy_tot < dst_len)
        {
            hoff_t cpy_len = MIN(dst_len - cpy_tot, src_len);
            rc = cpyfw(dst_fd, dst_at, dst_fd, dst_at + cpy_tot, cpy_len);
            checkrc(rc);
            cpy_tot += cpy_len;
        }
    }

    rc = RC_OK;

end:
    if(src_before >= 0)
        RESTORE_OFFSET(src_fd, src_before);
    if(dst_before >= 0)
        RESTORE_OFFSET(dst_fd, dst_before);
    traceExit(TRC_rc, rc);
    return rc;
}

/**
 * @brief Copy data within one file descriptor at specified file offsets. The
 *        file regions may overlap. This function is a wrapper for filecpy()
 *        with src_fd == dst_fd and src_len == dst_len.
 *
 * @param[in] fd File descriptor in which to read and write data
 * @param[in] src_at File offset at which to begin reading
 * @param[in] dst_at File offset at which to begin writing
 * @param[in] length Length of data to read and then write
 * @return Returns RC_OK if the copy operation completed fully, otherwise a
 *         hexpeek error code.
 */
rc_t lclcpy(int fd, hoff_t src_at, hoff_t dst_at, hoff_t length)
{
    return filecpy(fd, src_at, length, fd, dst_at, length);
}

/**
 * @brief Adjust the size of file by inserting or killing (deleting) bytes
 *        at a certain file offset.
 *
 * @param[in] data_fi Hexpeek file index of data file on which to perform the
 *            adjustment
 * @param[in] pos Offset at which to make a file adjustment
 * @param[in] amt Amount of file adjustment (positive inserts, negative kills)
 * @param[in] backup_fd Allows backup file descriptor to be manually specified
 *            for recovery operations. If negative, the value of backupFd() for
 *            data_fi is used.
 * @return Returns RC_OK if the copy operation completed fully, otherwise a
 *         hexpeek error code.
 */
rc_t adjustSize(int data_fi, hoff_t pos, hoff_t amt, int backup_fd)
{
    rc_t rc = RC_UNSPEC;
    hoff_t f_sz = filesize(data_fi);

    traceEntry("%d, " TRC_hoff ", " TRC_hoff ", %d",
               DT_FD(data_fi), trchoff(pos), trchoff(amt), backup_fd);

    assert(pos >= 0);
    if(amt < 0)
        pos -= amt;
    if(backup_fd < 0)
        backup_fd = backupFd(data_fi);

    rc = makeAdjBackup(data_fi, backup_fd, pos);
    checkrc(rc);

    if(pos < f_sz)
    {
        rc = lclcpy(DT_FD(data_fi), pos, pos + amt, f_sz - pos);
        checkrc(rc);
    }

    if(amt < 0 && hexpeek_truncate(DT_FD(data_fi), f_sz + amt))
        goto end;

    rc = clearAdjBackup(backup_fd, NULL);
    checkrc(rc);

    rc = RC_OK;

end:
    traceExit(TRC_rc, rc);
    return rc;
}
