/*
 * Copyright (C) by Argonne National Laboratory
 *     See COPYRIGHT in top-level directory
 */

#include "mpiimpl.h"

/*
*/

int MPIR_Allreduce_allcomm_auto(const void *sendbuf, void *recvbuf, MPI_Aint count,
                                MPI_Datatype datatype, MPI_Op op, MPIR_Comm * comm_ptr,
                                MPIR_Errflag_t * errflag)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_Csel_coll_sig_s coll_sig = {
        .coll_type = MPIR_CSEL_COLL_TYPE__ALLREDUCE,
        .comm_ptr = comm_ptr,

        .u.allreduce.sendbuf = sendbuf,
        .u.allreduce.recvbuf = recvbuf,
        .u.allreduce.count = count,
        .u.allreduce.datatype = datatype,
        .u.allreduce.op = op,
    };

    MPII_Csel_container_s *cnt = MPIR_Csel_search(comm_ptr->csel_comm, coll_sig);
    MPIR_Assert(cnt);

    switch (cnt->id) {
        case MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Allreduce_intra_recursive_doubling:
            mpi_errno =
                MPIR_Allreduce_intra_recursive_doubling(sendbuf, recvbuf, count, datatype, op,
                                                        comm_ptr, errflag);
            break;

        case MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Allreduce_intra_reduce_scatter_allgather:
            mpi_errno =
                MPIR_Allreduce_intra_reduce_scatter_allgather(sendbuf, recvbuf, count, datatype, op,
                                                              comm_ptr, errflag);
            break;

        case MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Allreduce_intra_smp:
            mpi_errno =
                MPIR_Allreduce_intra_smp(sendbuf, recvbuf, count, datatype, op, comm_ptr, errflag);
            break;

        case MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Allreduce_inter_reduce_exchange_bcast:
            mpi_errno =
                MPIR_Allreduce_inter_reduce_exchange_bcast(sendbuf, recvbuf, count, datatype, op,
                                                           comm_ptr, errflag);
            break;

        case MPII_CSEL_CONTAINER_TYPE__ALGORITHM__MPIR_Allreduce_allcomm_nb:
            mpi_errno =
                MPIR_Allreduce_allcomm_nb(sendbuf, recvbuf, count, datatype, op, comm_ptr, errflag);
            break;

        default:
            MPIR_Assert(0);
    }
    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

int MPIR_Allreduce_impl(const void *sendbuf, void *recvbuf, MPI_Aint count, MPI_Datatype datatype,
                        MPI_Op op, MPIR_Comm * comm_ptr, MPIR_Errflag_t * errflag)
{
    int mpi_errno = MPI_SUCCESS;

    if (comm_ptr->comm_kind == MPIR_COMM_KIND__INTRACOMM) {
        /* intracommunicator */
        switch (MPIR_CVAR_ALLREDUCE_INTRA_ALGORITHM) {
            case MPIR_CVAR_ALLREDUCE_INTRA_ALGORITHM_recursive_doubling:
                mpi_errno = MPIR_Allreduce_intra_recursive_doubling(sendbuf, recvbuf, count,
                                                                    datatype, op, comm_ptr,
                                                                    errflag);
                break;
            case MPIR_CVAR_ALLREDUCE_INTRA_ALGORITHM_reduce_scatter_allgather:
                mpi_errno = MPIR_Allreduce_intra_reduce_scatter_allgather(sendbuf, recvbuf, count,
                                                                          datatype, op, comm_ptr,
                                                                          errflag);
                break;
            case MPIR_CVAR_ALLREDUCE_INTRA_ALGORITHM_nb:
                mpi_errno = MPIR_Allreduce_allcomm_nb(sendbuf, recvbuf, count,
                                                      datatype, op, comm_ptr, errflag);
                break;
            case MPIR_CVAR_ALLREDUCE_INTRA_ALGORITHM_smp:
                mpi_errno =
                    MPIR_Allreduce_intra_smp(sendbuf, recvbuf, count, datatype, op, comm_ptr,
                                             errflag);
                break;
            case MPIR_CVAR_ALLREDUCE_INTRA_ALGORITHM_auto:
                mpi_errno = MPIR_Allreduce_allcomm_auto(sendbuf, recvbuf, count, datatype, op,
                                                        comm_ptr, errflag);
                break;
            default:
                MPIR_Assert(0);
        }
    } else {
        /* intercommunicator */
        switch (MPIR_CVAR_ALLREDUCE_INTER_ALGORITHM) {
            case MPIR_CVAR_ALLREDUCE_INTER_ALGORITHM_reduce_exchange_bcast:
                mpi_errno =
                    MPIR_Allreduce_inter_reduce_exchange_bcast(sendbuf, recvbuf, count, datatype,
                                                               op, comm_ptr, errflag);
                break;
            case MPIR_CVAR_ALLREDUCE_INTER_ALGORITHM_nb:
                mpi_errno = MPIR_Allreduce_allcomm_nb(sendbuf, recvbuf, count,
                                                      datatype, op, comm_ptr, errflag);
                break;
            case MPIR_CVAR_ALLREDUCE_INTER_ALGORITHM_auto:
                mpi_errno = MPIR_Allreduce_allcomm_auto(sendbuf, recvbuf, count, datatype, op,
                                                        comm_ptr, errflag);
                break;
            default:
                MPIR_Assert(0);
        }
    }
    MPIR_ERR_CHECK(mpi_errno);

  fn_exit:
    return mpi_errno;
  fn_fail:

    goto fn_exit;
}

int MPIR_Allreduce(const void *sendbuf, void *recvbuf, MPI_Aint count, MPI_Datatype datatype,
                   MPI_Op op, MPIR_Comm * comm_ptr, MPIR_Errflag_t * errflag)
{
    int mpi_errno = MPI_SUCCESS;
    void *in_recvbuf = recvbuf;
    void *host_sendbuf;
    void *host_recvbuf;

    MPIR_Coll_host_buffer_alloc(sendbuf, recvbuf, count, datatype, &host_sendbuf, &host_recvbuf);
    if (host_sendbuf)
        sendbuf = host_sendbuf;
    if (host_recvbuf)
        recvbuf = host_recvbuf;

    if ((MPIR_CVAR_DEVICE_COLLECTIVES == MPIR_CVAR_DEVICE_COLLECTIVES_all) ||
        ((MPIR_CVAR_DEVICE_COLLECTIVES == MPIR_CVAR_DEVICE_COLLECTIVES_percoll) &&
         MPIR_CVAR_ALLREDUCE_DEVICE_COLLECTIVE)) {
        mpi_errno = MPID_Allreduce(sendbuf, recvbuf, count, datatype, op, comm_ptr, errflag);
    } else {
        mpi_errno = MPIR_Allreduce_impl(sendbuf, recvbuf, count, datatype, op, comm_ptr, errflag);
    }

    /* Copy out data from host recv buffer to GPU buffer */
    if (host_recvbuf) {
        recvbuf = in_recvbuf;
        MPIR_Localcopy(host_recvbuf, count, datatype, recvbuf, count, datatype);
    }

    MPIR_Coll_host_buffer_free(host_sendbuf, host_recvbuf);

    return mpi_errno;
}
