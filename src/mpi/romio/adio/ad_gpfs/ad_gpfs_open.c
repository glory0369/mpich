/* ---------------------------------------------------------------- */
/* (C)Copyright IBM Corp.  2007, 2008                               */
/* ---------------------------------------------------------------- */
/**
 * \file ad_gpfs_open.c
 * \brief ???
 */

/* -*- Mode: C; c-basic-offset:4 ; -*- */
/* 
 *   Copyright (C) 1997 University of Chicago. 
 *   See COPYRIGHT notice in top-level directory.
 */

#include "ad_gpfs_tuning.h"

#include <sys/statfs.h>
#include <sys/vfs.h>
#include <sys/types.h>
#include <unistd.h>


void ADIOI_GPFS_Open(ADIO_File fd, int *error_code)
{
  int perm, old_mask, amode, rank, rc;
  static char myname[] = "ADIOI_GPFS_OPEN";

  /* set internal variables for tuning environment variables */
  ad_gpfs_get_env_vars();

  if (fd->perm == ADIO_PERM_NULL)  {
    old_mask = umask(022);
    umask(old_mask);
    perm = old_mask ^ 0666;
  }
  else perm = fd->perm;

    amode = 0;
    if (fd->access_mode & ADIO_CREATE)
	amode = amode | O_CREAT;
    if (fd->access_mode & ADIO_RDONLY)
	amode = amode | O_RDONLY;
    if (fd->access_mode & ADIO_WRONLY)
	amode = amode | O_WRONLY;
    if (fd->access_mode & ADIO_RDWR)
	amode = amode | O_RDWR;
    if (fd->access_mode & ADIO_EXCL)
	amode = amode | O_EXCL;
#ifdef ADIOI_MPE_LOGGING
    MPE_Log_event(ADIOI_MPE_open_a, 0, NULL);
#endif
    fd->fd_sys = open(fd->filename, amode, perm);
#ifdef ADIOI_MPE_LOGGING
    MPE_Log_event(ADIOI_MPE_open_b, 0, NULL);
#endif
  DBG_FPRINTF(stderr,"open('%s',%#X,%#X) rc=%d, errno=%d\n",fd->filename,amode,perm,fd->fd_sys,errno);
  fd->fd_direct = -1;

  if (gpfsmpio_devnullio == 1) {
      fd->null_fd = open("/dev/null", O_RDWR);
  } else {
      fd->null_fd = -1;
  }

  if ((fd->fd_sys != -1) && (fd->access_mode & ADIO_APPEND))
    fd->fp_ind = fd->fp_sys_posn = lseek(fd->fd_sys, 0, SEEK_END);

    if(fd->fd_sys != -1)
    {

        fd->blksize = 1048576; /* default to 1M */

#ifdef ADIOI_MPE_LOGGING
        MPE_Log_event(ADIOI_MPE_stat_a, 0, NULL);
#endif
	/* in this fs-specific routine, we might not be called over entire
	 * communicator (deferred open).  Collect statistics on one process.
	 * ADIOI_GEN_Opencoll (common-code caller) will take care of the
	 * broadcast */

	MPI_Comm_rank(fd->comm, &rank);
	if ((rank == fd->hints->ranklist[0]) || (fd->comm == MPI_COMM_SELF)) {
	    struct stat64 gpfs_stat;
	    /* Get the (real) underlying file system block size */
	    rc = stat64(fd->filename, &gpfs_stat);
	    if (rc >= 0)
	    {
		fd->blksize = gpfs_stat.st_blksize;
		DBGV_FPRINTF(stderr,"Successful stat '%s'.  Blocksize=%ld\n",
			fd->filename,gpfs_stat.st_blksize);
	    }
	    else
	    {
		DBGV_FPRINTF(stderr,"Stat '%s' failed with rc=%d, errno=%d\n",
			fd->filename,rc,errno);
	    }
	}
	/* all other ranks have incorrect fd->blocksize, but ADIOI_GEN_Opencoll
	 * will take care of that in both standard and deferred-open case */

#ifdef ADIOI_MPE_LOGGING
        MPE_Log_event(ADIOI_MPE_stat_b, 0, NULL);
#endif
    }

  if (fd->fd_sys == -1)  {
    if (errno == ENAMETOOLONG)
      *error_code = MPIO_Err_create_code(MPI_SUCCESS,
                                         MPIR_ERR_RECOVERABLE, myname,
                                         __LINE__, MPI_ERR_BAD_FILE,
                                         "**filenamelong",
                                         "**filenamelong %s %d",
                                         fd->filename,
                                         strlen(fd->filename));
    else if (errno == ENOENT)
      *error_code = MPIO_Err_create_code(MPI_SUCCESS,
                                         MPIR_ERR_RECOVERABLE, myname,
                                         __LINE__, MPI_ERR_NO_SUCH_FILE,
                                         "**filenoexist",
                                         "**filenoexist %s",
                                         fd->filename);
    else if (errno == ENOTDIR || errno == ELOOP)
      *error_code = MPIO_Err_create_code(MPI_SUCCESS,
                                         MPIR_ERR_RECOVERABLE,
                                         myname, __LINE__,
                                         MPI_ERR_BAD_FILE,
                                         "**filenamedir",
                                         "**filenamedir %s",
                                         fd->filename);
    else if (errno == EACCES)    {
      *error_code = MPIO_Err_create_code(MPI_SUCCESS,
                                         MPIR_ERR_RECOVERABLE, myname,
                                         __LINE__, MPI_ERR_ACCESS,
                                         "**fileaccess",
                                         "**fileaccess %s", 
                                         fd->filename );
    }
    else if (errno == EROFS)    {
      /* Read only file or file system and write access requested */
      *error_code = MPIO_Err_create_code(MPI_SUCCESS,
                                         MPIR_ERR_RECOVERABLE, myname,
                                         __LINE__, MPI_ERR_READ_ONLY,
                                         "**ioneedrd", 0 );
    }
    else    {
      *error_code = MPIO_Err_create_code(MPI_SUCCESS,
                                         MPIR_ERR_RECOVERABLE, myname,
                                         __LINE__, MPI_ERR_IO, "**io",
                                         "**io %s", strerror(errno));
    }
  }
  else *error_code = MPI_SUCCESS;
}
/* 
 *vim: ts=8 sts=4 sw=4 noexpandtab 
 */
