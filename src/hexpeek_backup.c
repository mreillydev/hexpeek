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

#define SRCNAME "hexpeek_backup.c"

#include <hexpeek.h>

#include <limits.h>
#include <string.h>
#include <errno.h>

/**
 * @file hexpeek_backup.c
 * @brief Encapsulated backup and recovery implementation.
 */

//-------------------------------- Constants ---------------------------------//

#define HDR_MAGIC_SZ   0x10
#define HDR_MAGIC_DATA PRGNM " bk v0\0\0\0"

#define OPINFO_MAGIC_SZ   0xF
#define OPINFO_MAGIC_DATA "\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\0\0\0"

#define OP_STATUS_BACKUP_START  0xB0
#define OP_STATUS_BACKUP_DONE   0xBD
#define OP_STATUS_RECOVERY_DONE 0xDD

#define OP_SZ  0x100
#define OP_MID (OPINFO_MAGIC_SZ + sizeof(uint8_t) + 6 * sizeof(hoff_t))

#define OP_CMD_TRUNCATED '~'

/**
 * @struct BackupOp
 *
 * @brief Packed struct stored in a backup file representing a backup operation.
 *
 * @var BackupOp::magic
 * Magic string which should be equal to OPINFO_MAGIC_DATA
 * @var BackupOp::status
 * Status field (should be one of OP_STATUS_*)
 * @var BackupOp::size_orig
 * Original infile file size before operation for which this backup was done.
 * @var BackupOp::size_adj
 * File adjustment size for operation for which this backup was done.
 * @var BackupOp::last_at
 * Previous infile file offset.
 * @var BackupOp::saved_from
 * Infile file offset start from which data was copied. Should be set even if
 * saved_len is zero.
 * @var BackupOp::saved_at
 * Backup file offset at which data was saved. Should be set even if saved_len
 * is zero.
 * @var BackupOp::saved_len
 * Length of saved data region.
 * @var BackupOp::origcmd
 * Locally encoded human readable original command string.
 */
typedef struct
{
    // Basic information
    uint8_t magic[OPINFO_MAGIC_SZ];
    uint8_t status;
    // Offsets in original data file
    hoff_t size_orig;
    hoff_t size_adj;
    hoff_t last_at;
    hoff_t saved_from;
    // Offsets in backup file
    hoff_t saved_at;
    hoff_t saved_len;
    // Nul-terminated string in local encoding
    char origcmd[OP_SZ - OP_MID];
} __attribute__((packed)) BackupOp;

#define LAST_ADJ_OPIDX MAX_BACKUP_DEPTH

/**
 * @struct BackupHeader
 *
 * @brief Packed struct stored in a backup file containing header information.
 *
 * @var BackupHeader::magic
 * Magic string which should be equal to HDR_MAGIC_DATA.
 * @var BackupHeader::firstop
 * First operation represented in this backup file (whenever we roll over to a
 * new backup file out of the pair, this value is set to current opcnt).
 * @var BackupHeader::reserved
 * Reserved space which should be zero. 
 * @var BackupHeader::ops
 * Array of BackupOp structs.
 * @var BackupHeader::reserved2
 * Reserved space which should be zero. 
 */
typedef struct
{
    uint8_t magic[HDR_MAGIC_SZ];
    uint64_t firstop;
    uint8_t reserved[sizeof(BackupOp) - HDR_MAGIC_SZ - sizeof(uint64_t)];
    BackupOp ops[MAX_BACKUP_DEPTH + 1];
    uint8_t reserved2[0x4000 - (MAX_BACKUP_DEPTH + 2) * sizeof(BackupOp)];
} __attribute__((packed)) BackupHeader;

//---------------------------------- Macros ----------------------------------//

// Offset in backup file of OpHeader for operation index oi
#define BKFL_OPINFO_OFF(oi) ( (1 + (oi)) * sizeof(BackupOp) )

// Backup completion octet
#define BKFL_BFIN_OFF(oi)   (BKFL_OPINFO_OFF((oi)) + OPINFO_MAGIC_SZ)
#define BKFL_BFIN_PTR(poh)  (&((poh)->status))
#define BKFL_BFIN_LEN       (sizeof(uint8_t))

// Recovery completion octet
#define BKFL_RFIN_OFF(oi)   BKFL_BFIN_OFF(oi)
#define BKFL_RFIN_PTR(poh)  BKFL_BFIN_PTR(poh)
#define BKFL_RFIN_LEN       BKFL_BFIN_LEN

#define sync(d) \
    if(Params.backup_sync && (rc = hexpeek_sync(d)) != RC_OK) \
        goto end;

//-------------------------------- Functions ---------------------------------//

/**
 * @brief File descriptor for backup file corresponding to infile file index.
 *
 * @param[in] data_fi Infile file index
 * @return A file descriptor for a backup file, or a negative value if none
 */
int backupFd(int data_fi)
{
    if(BackupDepth <= 0)
        return -1;
    else
        return BK_FD(data_fi,
                     (DT_OPCNT(data_fi) / BackupDepth) % BACKUP_FILE_COUNT);
}

/**
 * @brief Return index of most recent backup operation in the BackupHeader.
 *
 * @param[in] ph Pointer to a BackupHeader
 * @return Index of most recent backup operation, or a negative value if none
 */
static int mostRecentOp(BackupHeader const *ph)
{
    int max_op = MAX_BACKUP_DEPTH - 1;
    for( ; max_op >= 0; max_op--)
    {
        if(ph->ops[max_op].status)
            break;
    }
    return max_op;
}

/**
 * @brief Check validity of specified backup operation.
 *
 * @param[in] ph Pointer to a BackupHeader
 * @param[in] cur Op index of current operation
 * @param[in] prv Op index of previous operation
 * @return Zero if all validity checks passed; otherwise non-zero
 */
static int checkOp(BackupHeader const *ph, int cur, int prv)
{
    if(memcmp(ph->ops[cur].magic, OPINFO_MAGIC_DATA, OPINFO_MAGIC_SZ))
        return 2;
    else if(ph->ops[cur].size_orig < 0)
        return 3;
    else if(ph->ops[cur].saved_from < 0)
        return 4;
    else if(ph->ops[cur].saved_at < (hoff_t)sizeof *ph)
        return 5;
    else if(ph->ops[cur].saved_len < 0)
        return 6;
    else if(ph->ops[cur].saved_len > 0 && cur > 0 && prv >= 0 &&
            ph->ops[cur].saved_at <
            ph->ops[prv].saved_at + ph->ops[prv].saved_len)
        return 7;
    else if(ph->ops[cur].origcmd[sizeof ph->ops[cur].origcmd - 1] != '\0')
        return 8;
    return 0;
}

/**
 * @brief Check validity of backup header and its extant operations.
 *
 * @param[in] ph Pointer to a BackupHeader
 * @return RC_OK if all validity checks passed, else RC_CRIT
 */
static rc_t checkHeader(BackupHeader const *ph)
{
    rc_t rc = RC_UNSPEC;
    int mark = 0, ix = -1, most_recent = -1;

    traceEntry();

    if(memcmp(ph->magic, HDR_MAGIC_DATA, HDR_MAGIC_SZ))
    {
        mark = 1;
        rc = RC_CRIT;
        goto end;
    }

    most_recent = mostRecentOp(ph);

    ix = LAST_ADJ_OPIDX;
    if(ph->ops[ix].status)
    {
        mark = checkOp(ph, ix, most_recent);
        if(mark)
        {
            rc = RC_CRIT;
            goto end;
        }
    }

    for(ix = most_recent; ix >= 0; ix--)
    {
        mark = checkOp(ph, ix, ix - 1);
        if(mark)
        {
            rc = RC_CRIT;
            goto end;
        }
    }

    rc = RC_OK;

end:
    traceExit(TRC_rc " [mark=%d] [ix=%d]", rc, mark, ix);
    return rc;
}

/**
 * @brief Read data from the provided file descriptor into a BackupHeader and
 *        perform validity checking on it.
 *
 * @param[in] backup_fd File descriptor of backup file
 * @param[out] p_hdr BackupHeader pointer to which to output struct data
 * @return RC_OK if read and validation were successful
 */
static rc_t readHeader(int backup_fd, BackupHeader *p_hdr)
{
    rc_t rc = RC_UNSPEC;

    rc = readat(backup_fd, 0, p_hdr, sizeof *p_hdr);
    checkrc(rc);

    rc = checkHeader(p_hdr);
    if(rc)
    {
        prerr("%s header is malformed!\n", fdname(backup_fd));
        goto end;
    }

    rc = RC_OK;

end:
    return rc;
}

/**
 * @brief Read and validate header data from a given file descriptor and then
 *        determine file offset of next location to write backup operation data.
 *
 * @param[in] backup_fd File descriptor of backup file
 * @param[out] p_hdr BackupHeader pointer to which to output struct data
 * @param[out] next_at File offset of next location for backup op data
 * @return RC_OK on success, else a hexpeek error code
 */
static rc_t getHeader(int backup_fd, BackupHeader *p_hdr, hoff_t *next_at)
{
    rc_t rc = RC_UNSPEC;
    int max_op = -1;

    rc = readHeader(backup_fd, p_hdr);
    checkrc(rc);

    if(p_hdr->ops[LAST_ADJ_OPIDX].status)
    {
        rc = RC_CRIT;
        prerr("cannot make backup with incomplete file adjustment\n");
        goto end;
    }

    max_op = mostRecentOp(p_hdr);
    if(max_op < 0)
        *next_at = ceilbound(sizeof *p_hdr, PAGESZ);
    else
        *next_at = ceilbound(p_hdr->ops[max_op].saved_at +
                             p_hdr->ops[max_op].saved_len, PAGESZ);

    rc = RC_OK;

end:
    return rc;
}

/**
 * @brief Write backup data for a backup operation.
 *
 * @param[in] data_fi Infile file index
 * @param[in] backup_fd File descriptor of a backup file
 * @param[in] opix Operation index
 * @param[in,out] p_op Struct containing backup operation data to write to
 *                backup file. This function sets the status field.
 * @return RC_OK on success, else a hexpeek error code
 */
static rc_t writeOp(int data_fi, int backup_fd, int opix, BackupOp *p_op)
{
    rc_t rc = RC_UNSPEC;

    plugin(3, (void*)0);

    rc = writeat(backup_fd, BKFL_OPINFO_OFF(opix), p_op, sizeof *p_op);
    checkrc(rc);

    rc = filecpy(DT_FD(data_fi), p_op->saved_from, p_op->saved_len,
                 backup_fd,      p_op->saved_at,   p_op->saved_len);
    checkrc(rc);

    sync(backup_fd);
    p_op->status = OP_STATUS_BACKUP_DONE;
    rc = writeat(backup_fd, BKFL_BFIN_OFF(opix),
                 BKFL_BFIN_PTR(p_op), BKFL_BFIN_LEN);
    checkrc(rc);
    sync(backup_fd);

    rc = RC_OK;

end:
    plugin(3, (void*)1);
    return rc;
}

/**
 * @brief Perform backup operations corresponding to the command represented
 *        by the provided ParsedCommand structure.
 *
 * @param[in] ppc Pointer to a ParsedCommand structure.
 * @param Returns RC_OK on success, else a hexpeek error code
 */
rc_t makeBackup(ParsedCommand const *ppc)
{
    rc_t rc = RC_UNSPEC;
    int opix = 0, backup_fd = 0;
    hoff_t sv_at = HOFF_NIL;
    BackupHeader header;
    BackupOp *p_op = NULL;

    if(BackupDepth <= 0)
    {
        rc = RC_OK;
        goto end;
    }

    traceEntry("%" PRIu64, DT_OPCNT(ppc->fz.fi));

    assert(ppc);
    assert(ppc->fz.start >= 0);
    assert(ppc->fz.len >= 0);
    memset(&header, 0, sizeof header);

    if(DT_OPCNT(ppc->fz.fi) == UINT64_MAX)
    {
        rc = RC_CRIT;
        prerr("64 bit operation counter would overflow, aborting.\n");
        goto end;
    }

    opix = DT_OPCNT(ppc->fz.fi) % BackupDepth;
    p_op = &header.ops[opix];
    backup_fd = backupFd(ppc->fz.fi);
    assert(backup_fd >= 0);

    // If start of round, need to truncate backup file and write a new Header
    if(opix == 0)
    {
        memcpy(header.magic, HDR_MAGIC_DATA, HDR_MAGIC_SZ);
        header.firstop = DT_OPCNT(ppc->fz.fi);
        rc = hexpeek_truncate(backup_fd, 0);
        checkrc(rc);
        rc = writeat(backup_fd, 0, &header, sizeof header);
        checkrc(rc);
        sv_at = ceilbound(sizeof header, PAGESZ);
    }
    else
    {
        rc = getHeader(backup_fd, &header, &sv_at);
        checkrc(rc);
        if(p_op->status && p_op->status != OP_STATUS_RECOVERY_DONE)
        {
            prerr("%s header is malformed: unexpected operation present!\n",
                  fdname(backup_fd));
            goto end;
        }
    }

    memset(p_op, 0, sizeof *p_op);
    memcpy(p_op->magic, OPINFO_MAGIC_DATA, OPINFO_MAGIC_SZ);
    p_op->status        = OP_STATUS_BACKUP_START;
    p_op->size_orig     = filesize(ppc->fz.fi);
    p_op->last_at       = Params.infiles[ppc->fz.fi].last_at;
    p_op->saved_from    = ppc->fz.start;
    p_op->saved_at      = sv_at;
    switch(ppc->cmd)
    {
    case CMD_REPLACE:
        p_op->size_adj  = 0;
        p_op->saved_len = ppc->fz.len;
        break;
    case CMD_INSERT:
        p_op->size_adj  = ppc->fz.len;
        p_op->saved_len = 0;
        break;
    case CMD_KILL:
        p_op->size_adj  = -ppc->fz.len;
        p_op->saved_len = ppc->fz.len;
        break;
    default:
        die();
    }
    if(p_op->saved_from + p_op->saved_len > p_op->size_orig)
        p_op->saved_len = MAX(0, p_op->size_orig - p_op->saved_from);

    if(ppc->origcmd)
    {
        strncpy(p_op->origcmd, ppc->origcmd, sizeof p_op->origcmd - 1);
        if(strlen(ppc->origcmd) > sizeof p_op->origcmd - 1)
        {
            p_op->origcmd[sizeof p_op->origcmd - 3] = '\0';
            p_op->origcmd[sizeof p_op->origcmd - 2] = OP_CMD_TRUNCATED;
        }
    }

    rc = writeOp(ppc->fz.fi, backup_fd, opix, p_op);
    checkrc(rc);

    rc = RC_OK;

end:
    if(rc)
        prerr("backup failed\n");
    traceExit(TRC_rc, rc);
    return rc;
}

/**
 * @brief Make backup for file adjustment operation (insert / kill).
 *
 * @param[in] data_fi Infile file index.
 * @param[in] backup_fd Backup file descriptor.
 * @param[in] sv_from File offset from which to save data.
 * @return RC_OK on success, else a hexpeek error code
 */
rc_t makeAdjBackup(int data_fi, int backup_fd, hoff_t sv_from)
{
    rc_t rc = RC_UNSPEC;
    hoff_t sv_at = -1;
    BackupHeader header;
    BackupOp *p_op = &header.ops[LAST_ADJ_OPIDX];

    if(backup_fd < 0)
    {
        rc = RC_OK;
        goto end;
    }

    traceEntry("%d, %d, " TRC_hoff, data_fi, backup_fd, trchoff(sv_from));

    rc = getHeader(backup_fd, &header, &sv_at);
    checkrc(rc);

    memset(p_op, 0, sizeof *p_op);
    memcpy(p_op->magic, OPINFO_MAGIC_DATA, OPINFO_MAGIC_SZ);
    p_op->status     = OP_STATUS_BACKUP_START;
    p_op->saved_from = sv_from;
    p_op->saved_at   = sv_at;
    p_op->saved_len  = MAX(0, filesize(data_fi) - sv_from);

    rc = writeOp(data_fi, backup_fd, LAST_ADJ_OPIDX, p_op);
    checkrc(rc);

    rc = RC_OK;

end:
    if(rc)
        prerr("backup failed\n");
    traceExit(TRC_rc, rc);
    return rc;
}

/**
 * @brief Clear backup op data for a file adjustment operation (insert / kill).
 *
 * @param[in] backup_fd Backup file file descriptor
 * @param[in,out] vp Pointer to a BackupOp structure, ultimately zeroed
 * @return RC_OK on success, else a hexpeek error code
 */
rc_t clearAdjBackup(int backup_fd, void *vp)
{
    rc_t rc = RC_UNSPEC;
    hoff_t sv_at = HOFF_NIL;
    BackupHeader storage;
    BackupOp *p_adj = (BackupOp*)vp;

    if(backup_fd < 0)
    {
        rc = RC_OK;
        goto end;
    }

    traceEntry("%d, %p", backup_fd, p_adj);

    if( ! p_adj)
    {
        rc = readHeader(backup_fd, &storage);
        checkrc(rc);
        p_adj = &storage.ops[LAST_ADJ_OPIDX];
    }

    if(p_adj->status && p_adj->saved_len)
        sv_at = p_adj->saved_at;

    memset(p_adj, 0, sizeof *p_adj);
    rc = writeat(backup_fd, BKFL_OPINFO_OFF(LAST_ADJ_OPIDX),
                 p_adj, sizeof *p_adj);
    checkrc(rc);

    if(sv_at != HOFF_NIL)
    {
        rc = hexpeek_truncate(backup_fd, sv_at);
        checkrc(rc);
    }

    sync(backup_fd);

    rc = RC_OK;

end:
    traceExit(TRC_rc, rc);
    return rc;
}

/**
 * @struct RecoveryCounts
 *
 * @brief Store counters for recovery operations.
 *
 * @var RecoveryCounts::total
 * Count of recovery operations attempted in total.
 * @var RecoveryCounts::prev
 * Count of recovery operations that were already recovered on a previous run.
 * @var RecoveryCounts::reverted
 * Count of recovery operations that were successfully reverted.
 * @var RecoveryCounts::noncompl
 * Count of recovery operations skipped due to backup operation not completing.
 * @var RecoveryCounts::failed
 * Count of recovery recovery operations that were attempted but failed.
 */
typedef struct
{
    int total;
    int prev;
    int reverted;
    int noncompl;
    int failed;
} RecoveryCounts;

#define OPFMT      "#x%" PRIX64
#define OPNUM      ((p_hdr)->firstop + (opix))
#define OPTRUNC(s) ( ( (s)[sizeof (s) - 3] == '\0' && \
                       (s)[sizeof (s) - 2] == OP_CMD_TRUNCATED ) ? \
                     " (truncated)" : "")

/**
 * @brief Perform a recovery operation of a backup operation specified by
 *        p_hdr and opix.
 *
 * @param[in] data_fi Infile file index
 * @param[in] backup_fd Backup file file descriptor
 * @param[in] ask Boolean whether to prompt before reverting operations
 * @param[in] opix Backup operation index into p_hdr->ops[]
 * @param[in,out] p_hdr Pointer to a BackupHeader. This function sets the status
 *                field.
 * @param[out] p_cnt If non-NULL, write statistics for recovery operations
 * @return RC_OK on successful recovery, else a hexpeek error code
 */
static rc_t recoverOp(int data_fi, int backup_fd, bool ask, int opix,
                      BackupHeader *p_hdr, RecoveryCounts *p_cnt)
{
    rc_t rc = RC_UNSPEC;
    hoff_t f_sz = HOFF_NIL, post_sz = HOFF_NIL;

    traceEntry("%d, %d, %d, %p, %p", data_fi, backup_fd, opix, p_hdr, p_cnt);

    BackupOp *p_op = &p_hdr->ops[opix];

    switch(p_op->status)
    {
    case OP_STATUS_BACKUP_START:
        console("  Backup record " OPFMT " incomplete, skipping.\n", OPNUM);
        if(p_cnt) p_cnt->noncompl++;
        break;
    case OP_STATUS_BACKUP_DONE:
        if(p_op->size_adj < 0 && p_op->saved_len == 0)
        {
            rc = RC_CRIT;
            prerr("Backup record " OPFMT " has no data!\n", OPNUM);
            goto end;
        }

        if(ask && consoleAsk("  Revert operation " OPFMT " '%s'%s",
                             OPNUM, p_op->origcmd, OPTRUNC(p_op->origcmd)))
        {
            rc = RC_DONE;
            goto end;
        }
    
        f_sz = filesize(data_fi);
        post_sz = p_op->size_orig + p_op->size_adj;
        if(f_sz == p_op->size_orig)
        {
            // Nothing to do, file size is same as original.
        }
        else if(f_sz == post_sz)
        {
            // Previous file size adjustment completed, so just reverse it.
            rc = adjustSize(data_fi, p_op->saved_from, -p_op->size_adj,
                            backup_fd);
            checkrc(rc);
        }
        else if(p_op->size_adj >= 0 &&
                p_op->saved_from + p_op->saved_len >= p_op->size_orig &&
                f_sz > p_op->size_orig)
        {
            // An append operation did not complete, truncate it.
            rc = hexpeek_truncate(DT_FD(data_fi), p_op->size_orig);
            checkrc(rc);
        }
        else
        {
            // The interruption of a file size adjustment should be handled by
            // recoverAdjOp() - it should not be possible to reach this case.
            rc = RC_CRIT;
            prerr("data file size is wrong!\n");
            goto end;
        }
    
        rc = filecpy(backup_fd,      p_op->saved_at,   p_op->saved_len,
                     DT_FD(data_fi), p_op->saved_from, p_op->saved_len);
        checkrc(rc);

        p_op->status = OP_STATUS_RECOVERY_DONE;
        rc = writeat(backup_fd, BKFL_RFIN_OFF(opix),
                     BKFL_RFIN_PTR(p_op), BKFL_RFIN_LEN);
        checkrc(rc);

        sync(backup_fd);

        Params.infiles[data_fi].at = p_op->last_at;
        if(DT_OPCNT(data_fi) > 0)
            DT_OPCNT(data_fi)--;

        if(p_cnt) p_cnt->reverted++;
        break;
    case OP_STATUS_RECOVERY_DONE:
        console("  Backup record " OPFMT " previously recovered, skipping.\n",
                OPNUM);
        if(p_cnt) p_cnt->prev++;
        break;
    default:
        rc = RC_CRIT;
        prerr("Backup record " OPFMT " has unknown status!\n", OPNUM);
        goto end;
    }

    rc = RC_OK;

end:
    traceExit(TRC_rc, rc);
    return rc;
}

/**
 * @brief Perform a recovery operation of a file adjustment backup operation
 *        specified in p_adj.
 *
 * @param[in] data_fi Infile file index
 * @param[in] backup_fd Backup file file descriptor
 * @param[in] ask Boolean whether to prompt before reverting operations
 * @param[in,out] p_adj Pointer to a BackupOp.
 * @param[out] p_cnt If non-NULL, write statistics for recovery operations
 * @return RC_OK on successful recovery, else a hexpeek error code
 */
static rc_t recoverAdjOp(int data_fi, int backup_fd, bool ask, BackupOp *p_adj,
                         RecoveryCounts *p_cnt)
{
    rc_t rc = RC_UNSPEC;

    traceEntry("%d, %d, %p, %p", data_fi, backup_fd, p_adj, p_cnt);

    if(p_adj->status == 0)
    {
        rc = RC_OK;
        goto end;
    }

    if(p_cnt) p_cnt->total++;

    switch(p_adj->status)
    {
    case OP_STATUS_BACKUP_START:
        console("  Backup record for file size adjustment incomplete, "
                "skipping.\n");
        if(p_cnt) p_cnt->noncompl++;
        break;
    case OP_STATUS_BACKUP_DONE:
        if(ask &&
           consoleAsk("  A file size adjustment was interrupted, revert"))
        {
            rc = RC_DONE;
            goto end;
        }
        rc = hexpeek_truncate(DT_FD(data_fi),
                              p_adj->saved_from + p_adj->saved_len);
        checkrc(rc);
        rc = filecpy(backup_fd,      p_adj->saved_at,   p_adj->saved_len,
                     DT_FD(data_fi), p_adj->saved_from, p_adj->saved_len);
        checkrc(rc);
        sync(backup_fd);
        rc = clearAdjBackup(backup_fd, p_adj);
        checkrc(rc);
        console("  File size adjustment successfully reverted.\n");
        if(p_cnt) p_cnt->reverted++;
        break;
    case OP_STATUS_RECOVERY_DONE:
        console("  Backup record for file size adjustment previously "
                "recovered, skipping.\n");
        if(p_cnt) p_cnt->prev++;
    default:
        rc = RC_CRIT;
        prerr("backup record for file size adjustment has unknown status!\n");
        goto end;
    }

    rc = RC_OK;

end:
    traceExit(TRC_rc, rc);
    return rc;
}

/**
 * @brief Perform a recovery operation of a specified backup operation.
 *
 * @param[in] data_fi Infile file index
 * @param[in] backup_fd Backup file file descriptor
 * @param[in,out] p_hdr Pointer to a BackupHeader
 * @param[out] uncompleted If there are any recovery operations that could not
 *             complete, set this to true
 * @return RC_OK on successful recovery, else a hexpeek error code
 */
static rc_t recoverBackupFile(int data_fi,
                              int backup_fd,
                              BackupHeader *p_hdr,
                              bool *uncompleted)
{
    rc_t rc = RC_UNSPEC;
    int max_op = 0;
    RecoveryCounts counts;

    memset(&counts, 0, sizeof counts);

    console("\nRecovery from %s starting.\n", fdname(backup_fd));

    max_op = mostRecentOp(p_hdr);
    counts.total += max_op;

    rc = recoverAdjOp(data_fi, backup_fd, ! Params.recover_auto,
                      p_hdr->ops + LAST_ADJ_OPIDX, &counts);
    checkrc(rc);

    // Note that this code will process, without error, a backup file that
    // contains non-complete backup records between complete backup records
    // even though such a file can never be generated by operation of this
    // program. Consider adding a pre-check that such a gap does not exist.
    for(int cur_op = max_op; cur_op >= 0; cur_op--)
    {
        rc = recoverOp(data_fi, backup_fd, ! Params.recover_auto, cur_op, p_hdr,
                       &counts);
        checkrc(rc);
    }

    rc = RC_OK;

end:
    console("\n");
    if(rc == RC_DONE)
    {
        console("Recovery from %s was terminated by user:\n",fdname(backup_fd));
    }
    else if(rc)
    {
        counts.failed = 1;
        console("Recovery from %s failed:\n", fdname(backup_fd));
    }
    else if(counts.reverted)
    {
        console("Recovery from %s was successful:\n", fdname(backup_fd));
    }
    else
    {
        console("No recovery from %s was attempted:\n", fdname(backup_fd));
    }
    int nonproc = counts.total + 1 - counts.prev - counts.reverted
                                   - counts.noncompl - counts.failed;
    console("  x%X backup record%s previously recovered\n",
            plrztn(counts.prev));
    console("  x%X backup record%s successfully reverted\n",
            plrztn(counts.reverted));
    console("  x%X backup record%s skipped due to incompletion\n",
            plrztn(counts.noncompl));
    console("  x%X backup record%s failed recovery attempt\n",
            plrztn(counts.failed));
    console("  x%X backup record%s not processed due to early termination\n",
            plrztn(nonproc));
    if(counts.noncompl)
    {
        console(
"When a backup record is skipped during recovery due to incompletion, this\n"
"usually indicates the backup for that operation was interrupted, meaning the\n"
"operation in question never modified the data file. It is also possible the\n"
"backup file has been corrupted since it was written.\n");
    }
    if(rc || counts.noncompl || counts.failed || nonproc)
        *uncompleted = true;
    return rc;
}

/**
 * @brief Perform available recovery operations on an infile.
 *
 * @param[in] data_fi Infile file index
 * @param[in] what Specify what specifically this call should do
 *            - -1      : print recoverable operations
 *            - INT_MAX : prompt for recovery or auto recover
 *            - <DEPTH> : recover up to DEPTH operations
 */
rc_t recoverBackup(int data_fi, int what)
{
    rc_t rc = RC_UNSPEC;
    int *backup_fds = Params.infiles[data_fi].bk_fds;
    int files_count = 0, files_successful = 0;
    bool ops_uncompleted = false;
    BackupHeader hrs[BACKUP_FILE_COUNT];
    int sorted[BACKUP_FILE_COUNT];

    memset(&hrs, 0, sizeof hrs);
    for(int bidx = 0; bidx < BACKUP_FILE_COUNT; bidx++)
        sorted[bidx] = -1;

    if(what == INT_MAX)
        console("\nRecovery starting.\n");

    // Read and validate the Header of each backup file
    for(int bidx = 0; bidx < BACKUP_FILE_COUNT; bidx++)
    {
        ssize_t rdsz = -1;
        if(backup_fds[bidx] < 0)
            continue;
        files_count++;
        rc = seekto(backup_fds[bidx], 0);
        checkrc(rc);
        rdsz = readfull(backup_fds[bidx], &hrs[bidx], sizeof hrs[bidx]);
        if(rdsz == 0)
        {
            if(what == INT_MAX)
                console("\n%s is empty, skipping.\n", fdname(backup_fds[bidx]));
            files_count--;
            continue;
        }
        else if(rdsz != sizeof hrs[bidx])
        {
            rc = RC_CRIT;
            if(errno)
            {
                prerr("error reading from %s: %s\n", fdname(backup_fds[bidx]),
                      strerror(errno));
            }
            else
            {
                prerr(EofErrString , fdname(backup_fds[bidx]));
            }
            goto end;
        }
        if(checkHeader(&hrs[bidx]))
        {
            rc = RC_CRIT;
            prerr("%s header is malformed!\n", fdname(backup_fds[bidx]));
            goto end;
        }
        sorted[bidx] = bidx;
    }

    // Sort so backup files are processed newest to oldest
#if (BACKUP_FILE_COUNT == 2)
    if(files_count == 2 && hrs[1].firstop > hrs[0].firstop)
    {
        sorted[0] = 1;
        sorted[1] = 0;
    }
#else
    #error
#endif

    if(what == INT_MAX)
    {
        for(int st_idx = 0; st_idx < files_count; st_idx++)
        {
            rc = recoverBackupFile(data_fi, BK_FD(data_fi, sorted[st_idx]),
                                   &hrs[sorted[st_idx]], &ops_uncompleted);
            if(rc == RC_DONE)
            {
                rc = RC_OK;
                goto end;
            }
            else if(rc)
            {
                goto end;
            }
            files_successful++;
        }
        console("\nSyncing data file...\n");
        rc = hexpeek_sync(DT_FD(data_fi));
        checkrc(rc);
        console("Sync complete.\n");
    }
    else
    {
        for(int st_idx = 0, counter = 0; st_idx < files_count; st_idx++)
        {
            BackupHeader *p_hdr = &hrs[sorted[st_idx]];
            for(int opix = mostRecentOp(p_hdr); opix >= 0; opix--)
            {
                if(p_hdr->ops[opix].status != OP_STATUS_RECOVERY_DONE)
                {
                    counter++; // 1 based index
                    if(what == -1)
                    {
                        console("%X, operation " OPFMT ", command '%s'%s\n",
                                counter, OPNUM,
                                p_hdr->ops[opix].origcmd,
                                OPTRUNC(p_hdr->ops[opix].origcmd));
                    }
                    else if(counter <= what)
                    {
                        rc = recoverOp(data_fi, BK_FD(data_fi, sorted[st_idx]),
                                       false, opix, &hrs[sorted[st_idx]], NULL);
                        if(rc)
                            goto end;
                    }
                    else
                    {
                        goto done;
                    }
                }
            }
        }
/*
        if(what > 0)
        {
            // Getting here means the undo depth was greater than the number of
            // operations available to recover. Currently this is not treated as
            // an error or warning condition.
        }
*/
    }

done:
    rc = RC_OK;

end:
    if(what == INT_MAX)
    {
        if(rc)
            console("\nRecovery FAILED.\n");
        else if(files_count != files_successful)
            console("\nRecovery skipped.\n");
        else
            console("\nRecovery complete.\n");
        if(rc || ops_uncompleted)
            BackupUnlinkAllowed = false;
    }
    return rc;
}
