/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil -*- */
/*
 * Copyright (c) 2014-2020 Intel, Inc.  All rights reserved.
 * Copyright (c) 2014-2019 Research Organization for Information Science
 *                         and Technology (RIST).  All rights reserved.
 * Copyright (c) 2014      Artem Y. Polyakov <artpol84@gmail.com>.
 *                         All rights reserved.
 * Copyright (c) 2016      Mellanox Technologies, Inc.
 *                         All rights reserved.
 * Copyright (c) 2016      IBM Corporation.  All rights reserved.
 * Copyright (c) 2021-2022 Nanook Consulting.  All rights reserved.
 * $COPYRIGHT$
 *
 * Additional copyrights may follow
 *
 * $HEADER$
 */

#include "src/include/pmix_config.h"

#include "src/include/pmix_stdint.h"

#include "include/pmix.h"

#include "src/include/pmix_globals.h"

#ifdef HAVE_STRING_H
#    include <string.h>
#endif
#include <fcntl.h>
#ifdef HAVE_UNISTD_H
#    include <unistd.h>
#endif
#ifdef HAVE_SYS_SOCKET_H
#    include <sys/socket.h>
#endif
#ifdef HAVE_SYS_UN_H
#    include <sys/un.h>
#endif
#ifdef HAVE_SYS_UIO_H
#    include <sys/uio.h>
#endif
#ifdef HAVE_SYS_TYPES_H
#    include <sys/types.h>
#endif
#include <event.h>

#include "src/class/pmix_list.h"
#include "src/mca/bfrops/bfrops.h"
#include "src/mca/gds/gds.h"
#include "src/mca/pfexec/pfexec.h"
#include "src/mca/pmdl/pmdl.h"
#include "src/mca/pnet/base/base.h"
#include "src/mca/ptl/ptl.h"
#include "src/threads/pmix_threads.h"
#include "src/util/pmix_argv.h"
#include "src/util/pmix_basename.h"
#include "src/util/pmix_error.h"
#include "src/util/pmix_name_fns.h"
#include "src/util/pmix_output.h"
#include "src/util/pmix_environ.h"
#include "src/util/pmix_getcwd.h"

#include "src/server/pmix_server_ops.h"
#include "pmix_client_ops.h"

static void wait_cbfunc(struct pmix_peer_t *pr, pmix_ptl_hdr_t *hdr, pmix_buffer_t *buf,
                        void *cbdata);
static void spawn_cbfunc(pmix_status_t status, char nspace[], void *cbdata);

PMIX_EXPORT pmix_status_t PMIx_Spawn(const pmix_info_t job_info[], size_t ninfo,
                                     const pmix_app_t apps[], size_t napps, pmix_nspace_t nspace)
{
    pmix_status_t rc;
    pmix_cb_t *cb;

    PMIX_ACQUIRE_THREAD(&pmix_global_lock);

    pmix_output_verbose(2, pmix_client_globals.spawn_output, "%s pmix: spawn called",
                        PMIX_NAME_PRINT(&pmix_globals.myid));

    if (pmix_globals.init_cntr <= 0) {
        PMIX_RELEASE_THREAD(&pmix_global_lock);
        return PMIX_ERR_INIT;
    }

    /* if we aren't connected, don't attempt to send */
    if (!PMIX_PEER_IS_SERVER(pmix_globals.mypeer) &&
        !pmix_globals.connected &&
        !PMIX_PEER_IS_LAUNCHER(pmix_globals.mypeer)) {
        PMIX_RELEASE_THREAD(&pmix_global_lock);
        return PMIX_ERR_UNREACH;
    }
    PMIX_RELEASE_THREAD(&pmix_global_lock);

    /* ensure the nspace (if provided) is initialized */
    if (NULL != nspace) {
        memset(nspace, 0, PMIX_MAX_NSLEN + 1);
    }

    /* create a callback object */
    cb = PMIX_NEW(pmix_cb_t);

    if (PMIX_SUCCESS != (rc = PMIx_Spawn_nb(job_info, ninfo, apps, napps, spawn_cbfunc, cb))) {
        /* note: the call may have returned PMIX_OPERATION_SUCCEEDED thus indicating
         * that the spawn was atomically completed */
        if (PMIX_OPERATION_SUCCEEDED == rc) {
            PMIX_LOAD_NSPACE(nspace, cb->pname.nspace);
            rc = PMIX_SUCCESS;
        }
        PMIX_RELEASE(cb);
        return rc;
    }

    /* wait for the result */
    PMIX_WAIT_THREAD(&cb->lock);
    rc = cb->status;
    if (NULL != nspace) {
        pmix_strncpy(nspace, cb->pname.nspace, PMIX_MAX_NSLEN);
    }
    PMIX_RELEASE(cb);

    return rc;
}

PMIX_EXPORT pmix_status_t PMIx_Spawn_nb(const pmix_info_t job_info[], size_t ninfo,
                                        const pmix_app_t apps[], size_t napps,
                                        pmix_spawn_cbfunc_t cbfunc, void *cbdata)
{
    pmix_buffer_t *msg;
    pmix_cmd_t cmd = PMIX_SPAWNNB_CMD;
    pmix_status_t rc;
    size_t n, m;
    pmix_app_t *aptr;
    bool jobenvars = false;
    bool forkexec = false;
    pmix_kval_t *kv;
    pmix_list_t ilist;
    char cwd[PMIX_PATH_MAX];
    char *tmp, *t2;
    pmix_setup_caddy_t *cd;
    bool proxy = false;
    pmix_proc_t parent;

    PMIX_ACQUIRE_THREAD(&pmix_global_lock);

    pmix_output_verbose(2, pmix_client_globals.spawn_output,
                        "%s pmix: spawn_nb called",
                        PMIX_NAME_PRINT(&pmix_globals.myid));

    if (pmix_globals.init_cntr <= 0) {
        PMIX_RELEASE_THREAD(&pmix_global_lock);
        return PMIX_ERR_INIT;
    }

    /* if we aren't connected, don't attempt to send */
    if (!pmix_globals.connected) {
        /* if I am a launcher, we default to local fork/exec */
        if (PMIX_PEER_IS_LAUNCHER(pmix_globals.mypeer)) {
            forkexec = true;
        } else if (!PMIX_PEER_IS_SERVER(pmix_globals.mypeer)) {
            PMIX_RELEASE_THREAD(&pmix_global_lock);
            return PMIX_ERR_UNREACH;
        }
    }
    PMIX_RELEASE_THREAD(&pmix_global_lock);

    /* check job info for directives */
    if (NULL != job_info) {
        for (n = 0; n < ninfo; n++) {
            if (PMIX_CHECK_KEY(&job_info[n], PMIX_SETUP_APP_ENVARS)) {
                PMIX_CONSTRUCT(&ilist, pmix_list_t);
                rc = pmix_pmdl.harvest_envars(NULL, job_info, ninfo, &ilist);
                if (PMIX_SUCCESS != rc) {
                    PMIX_LIST_DESTRUCT(&ilist);
                    return rc;
                }
                PMIX_LIST_FOREACH (kv, &ilist, pmix_kval_t) {
                    /* cycle across all the apps and set this envar */
                    for (m = 0; m < napps; m++) {
                        aptr = (pmix_app_t *) &apps[m];
                        pmix_setenv(kv->value->data.envar.envar, kv->value->data.envar.value, true,
                                    &aptr->env);
                    }
                }
                jobenvars = true;
                PMIX_LIST_DESTRUCT(&ilist);
            } else if (PMIX_CHECK_KEY(&job_info[n], PMIX_PARENT_ID)) {
                PMIX_XFER_PROCID(&parent, job_info[n].value.data.proc);
                proxy = true;
            }
        }
    }

    for (n = 0; n < napps; n++) {
        aptr = (pmix_app_t *) &apps[n];
        /* protect against idiot case (yes, they exist) */
        if (NULL == aptr->cmd && NULL == aptr->argv) {
            /* they gave us nothing to spawn! */
            return PMIX_ERR_BAD_PARAM;
        }
        /* if they didn't give us a desired working directory, then
         * take the one we are in */
        if (NULL == aptr->cwd) {
            rc = pmix_getcwd(cwd, sizeof(cwd));
            if (PMIX_SUCCESS != rc) {
                return rc;
            }
            aptr->cwd = strdup(cwd);
        }
        /* if they didn't give us the cmd as the first argv, fix it */
        if (NULL == aptr->argv) {
            tmp = pmix_basename(aptr->cmd);
            aptr->argv = (char **) malloc(2 * sizeof(char *));
            aptr->argv[0] = tmp;
            aptr->argv[1] = NULL;
        } else {
            tmp = pmix_basename(aptr->cmd);
            t2 = pmix_basename(aptr->argv[0]);
            if (0 != strcmp(tmp, t2)) {
                pmix_argv_prepend_nosize(&aptr->argv, tmp);
            }
            free(tmp);
            free(t2);
        }
        /* do a quick check of the apps directive array to ensure
         * the ninfo field has been set */
        if (NULL != aptr->info && 0 == aptr->ninfo) {
            /* look for the info marked as "end" */
            m = 0;
            while (!(PMIX_INFO_IS_END(&aptr->info[m])) && m < SIZE_MAX) {
                ++m;
            }
            if (SIZE_MAX == m) {
                /* nothing we can do */
                return PMIX_ERR_BAD_PARAM;
            }
            aptr->ninfo = m;
        }
        if (!jobenvars) {
            for (m = 0; m < aptr->ninfo; m++) {
                if (PMIX_CHECK_KEY(&aptr->info[m], PMIX_SETUP_APP_ENVARS)) {
                    PMIX_CONSTRUCT(&ilist, pmix_list_t);
                    rc = pmix_pmdl.harvest_envars(NULL, aptr->info, aptr->ninfo, &ilist);
                    if (PMIX_SUCCESS != rc) {
                        PMIX_LIST_DESTRUCT(&ilist);
                        return rc;
                    }
                    PMIX_LIST_FOREACH (kv, &ilist, pmix_kval_t) {
                        pmix_setenv(kv->value->data.envar.envar, kv->value->data.envar.value, true,
                                    &aptr->env);
                    }
                    jobenvars = true;
                    PMIX_LIST_DESTRUCT(&ilist);
                    break;
                }
            }
        }
    }

    /* if we are a server, then process this ourselves */
    if (PMIX_PEER_IS_SERVER(pmix_globals.mypeer) &&
        !PMIX_PEER_IS_LAUNCHER(pmix_globals.mypeer)) {
        cd = PMIX_NEW(pmix_setup_caddy_t);
        if (NULL == cd) {
            return PMIX_ERR_NOMEM;
        }
        /* if I am spawning on behalf of someone else, then
         * that peer is the "spawner" */
        if (proxy) {
            /* find the parent's peer object */
            cd->peer = pmix_get_peer_object(&parent);
            if (NULL == cd->peer) {
                PMIX_RELEASE(cd);
                return PMIX_ERR_NOT_FOUND;
            }
        } else {
            cd->peer = pmix_globals.mypeer;
        }
        PMIX_RETAIN(cd->peer);
        cd->info = (pmix_info_t*)job_info;
        cd->ninfo = ninfo;
        cd->apps = (pmix_app_t*)apps;
        cd->napps = napps;
        cd->spcbfunc = cbfunc;
        cd->cbdata = cbdata;
        // mark that we are using the input data
        cd->copied = false;
        /* run a quick check of the directives to see if any IOF
         * requests were included so we can set that up now - helps
         * to catch any early output - and a request for notification
         * of job termination so we can setup the event registration */
        pmix_server_spawn_parser(pmix_globals.mypeer, cd);
        /* call the local host */
        rc = pmix_host_server.spawn(&pmix_globals.myid, cd->info, cd->ninfo,
                                    cd->apps, cd->napps,
                                    pmix_server_spcbfunc, cd);
        if (PMIX_SUCCESS != rc) {
            PMIX_RELEASE(cd);
        }
        return rc;
    }

    /* if we are not connected, then just fork/exec
     * the specified application */
    if (forkexec) {
        rc = pmix_pfexec.spawn_job(job_info, ninfo, apps, napps, cbfunc, cbdata);
        return rc;
    }

    msg = PMIX_NEW(pmix_buffer_t);
    /* pack the cmd */
    PMIX_BFROPS_PACK(rc, pmix_client_globals.myserver, msg, &cmd, 1, PMIX_COMMAND);
    if (PMIX_SUCCESS != rc) {
        PMIX_ERROR_LOG(rc);
        PMIX_RELEASE(msg);
        return rc;
    }

    /* pack the job-level directives */
    PMIX_BFROPS_PACK(rc, pmix_client_globals.myserver, msg, &ninfo, 1, PMIX_SIZE);
    if (PMIX_SUCCESS != rc) {
        PMIX_ERROR_LOG(rc);
        PMIX_RELEASE(msg);
        return rc;
    }
    if (0 < ninfo) {
        PMIX_BFROPS_PACK(rc, pmix_client_globals.myserver, msg, job_info, ninfo, PMIX_INFO);
        if (PMIX_SUCCESS != rc) {
            PMIX_ERROR_LOG(rc);
            PMIX_RELEASE(msg);
            return rc;
        }
    }

    /* pack the apps */
    PMIX_BFROPS_PACK(rc, pmix_client_globals.myserver, msg, &napps, 1, PMIX_SIZE);
    if (PMIX_SUCCESS != rc) {
        PMIX_ERROR_LOG(rc);
        PMIX_RELEASE(msg);
        return rc;
    }
    if (0 < napps) {
        PMIX_BFROPS_PACK(rc, pmix_client_globals.myserver, msg, apps, napps, PMIX_APP);
        if (PMIX_SUCCESS != rc) {
            PMIX_ERROR_LOG(rc);
            PMIX_RELEASE(msg);
            return rc;
        }
    }

    /* create a callback object as we need to pass it to the
     * recv routine so we know which callback to use when
     * the return message is recvd */
    cd = PMIX_NEW(pmix_setup_caddy_t);
    if (NULL == cd) {
        return PMIX_ERR_NOMEM;
    }
    cd->spcbfunc = cbfunc;
    cd->cbdata = cbdata;
    cd->info = (pmix_info_t*)job_info;
    cd->ninfo = ninfo;
    // mark that we are using the input data
    cd->copied = false;
    /* check for IOF flags */
    pmix_server_spawn_parser(pmix_globals.mypeer, cd);

    /* push the message into our event base to send to the server */
    PMIX_PTL_SEND_RECV(rc, pmix_client_globals.myserver, msg, wait_cbfunc, (void *) cd);
    if (PMIX_SUCCESS != rc) {
        PMIX_RELEASE(msg);
        PMIX_RELEASE(cd);
    }

    return rc;
}

/* callback for wait completion */
static void wait_cbfunc(struct pmix_peer_t *pr, pmix_ptl_hdr_t *hdr, pmix_buffer_t *buf,
                        void *cbdata)
{
    pmix_setup_caddy_t *cd = (pmix_setup_caddy_t *) cbdata;
    char nspace[PMIX_MAX_NSLEN + 1];
    char *n2 = NULL;
    pmix_status_t rc, ret;
    int32_t cnt;
    pmix_namespace_t *nptr, *ns;

    PMIX_ACQUIRE_OBJECT(cd);

    pmix_output_verbose(2, pmix_globals.debug_output,
                        "pmix:client recv spawn callback activated with %d bytes",
                        (NULL == buf) ? -1 : (int) buf->bytes_used);
    PMIX_HIDE_UNUSED_PARAMS(pr, hdr);

    /* init */
    memset(nspace, 0, PMIX_MAX_NSLEN + 1);

    if (NULL == buf) {
        ret = PMIX_ERR_BAD_PARAM;
        goto report;
    }
    /* a zero-byte buffer indicates that this recv is being
     * completed due to a lost connection */
    if (PMIX_BUFFER_IS_EMPTY(buf)) {
        ret = PMIX_ERR_UNREACH;
        goto report;
    }

    /* unpack the returned status */
    cnt = 1;
    PMIX_BFROPS_UNPACK(rc, pmix_client_globals.myserver, buf, &ret, &cnt, PMIX_STATUS);
    if (PMIX_SUCCESS != rc) {
        PMIX_ERROR_LOG(rc);
        ret = rc;
    }
    /* unpack the namespace */
    cnt = 1;
    PMIX_BFROPS_UNPACK(rc, pmix_client_globals.myserver, buf, &n2, &cnt, PMIX_STRING);
    if (PMIX_SUCCESS != rc && PMIX_ERR_UNPACK_READ_PAST_END_OF_BUFFER != rc) {
        PMIX_ERROR_LOG(rc);
        ret = rc;
    }
    pmix_output_verbose(1, pmix_globals.debug_output,
                        "pmix:client recv '%s'", n2);

    if (NULL != n2) {
        /* protect length */
        pmix_strncpy(nspace, n2, PMIX_MAX_NSLEN);
        free(n2);
        PMIX_GDS_STORE_JOB_INFO(rc, pmix_globals.mypeer, nspace, buf);
        /* extract and process any job-related info for this nspace */
        if (PMIX_SUCCESS != rc) {
            PMIX_ERROR_LOG(rc);
            ret = rc;
        }
        /* process any IOF flags - we are only concerned if we are a TOOL
         * and need to know if/how we should output any IO */
        if (PMIX_PEER_IS_TOOL(pmix_globals.mypeer)) {
            nptr = NULL;
            PMIX_LIST_FOREACH (ns, &pmix_globals.nspaces, pmix_namespace_t)
            {
                if (PMIX_CHECK_NSPACE(ns->nspace, nspace)) {
                    nptr = ns;
                    break;
                }
            }
            if (NULL == nptr) {
                /* shouldn't happen, but protect us */
                nptr = PMIX_NEW(pmix_namespace_t);
                nptr->nspace = strdup(nspace);
                pmix_list_append(&pmix_globals.nspaces, &nptr->super);
            }
            /* as a client, we only handle a select set of the flags */
            memcpy(&nptr->iof_flags, &cd->flags, sizeof(pmix_iof_flags_t));
            nptr->iof_flags.file = NULL;
            nptr->iof_flags.directory = NULL;
            /* since we are not a server, nocopy equates to no_local_output */
            if (cd->flags.nocopy) {
                nptr->iof_flags.local_output = false;
            }
        }
    }

report:
    if (NULL != cd->spcbfunc) {
        cd->spcbfunc(ret, nspace, cd->cbdata);
    }
    PMIX_RELEASE(cd);
}

static void spawn_cbfunc(pmix_status_t status, char nspace[], void *cbdata)
{
    pmix_cb_t *cb = (pmix_cb_t *) cbdata;

    PMIX_ACQUIRE_OBJECT(cb);
    cb->status = status;
    if (NULL != nspace) {
        cb->pname.nspace = strdup(nspace);
    }
    PMIX_POST_OBJECT(cb);
    PMIX_WAKEUP_THREAD(&cb->lock);
}
