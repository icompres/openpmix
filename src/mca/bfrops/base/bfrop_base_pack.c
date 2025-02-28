/*
 * Copyright (c) 2004-2005 The Trustees of Indiana University and Indiana
 *                         University Research and Technology
 *                         Corporation.  All rights reserved.
 * Copyright (c) 2004-2006 The University of Tennessee and The University
 *                         of Tennessee Research Foundation.  All rights
 *                         reserved.
 * Copyright (c) 2004-2005 High Performance Computing Center Stuttgart,
 *                         University of Stuttgart.  All rights reserved.
 * Copyright (c) 2004-2005 The Regents of the University of California.
 *                         All rights reserved.
 * Copyright (c) 2015-2020 Intel, Inc.  All rights reserved.
 * Copyright (c) 2019      Mellanox Technologies, Inc.
 *                         All rights reserved.
 * Copyright (c) 2021-2022 Nanook Consulting.  All rights reserved.
 * $COPYRIGHT$
 *
 * Additional copyrights may follow
 *
 * $HEADER$
 */

#include "src/include/pmix_config.h"

#include <stdio.h>
#ifdef HAVE_UNISTD_H
#    include <unistd.h>
#endif

#include "src/class/pmix_pointer_array.h"
#include "src/hwloc/pmix_hwloc.h"
#include "src/include/pmix_globals.h"
#include "src/mca/preg/preg.h"
#include "src/util/pmix_argv.h"
#include "src/util/pmix_error.h"
#include "src/util/pmix_output.h"

#include "src/mca/bfrops/base/base.h"

pmix_status_t pmix_bfrops_base_pack(pmix_pointer_array_t *regtypes, pmix_buffer_t *buffer,
                                    const void *src, int num_vals, pmix_data_type_t type)
{
    pmix_status_t rc;

    /* check for error */
    if (NULL == buffer || NULL == src) {
        PMIX_ERROR_LOG(PMIX_ERR_BAD_PARAM);
        return PMIX_ERR_BAD_PARAM;
    }

    /* Pack the number of values */
    if (PMIX_BFROP_BUFFER_FULLY_DESC == buffer->type) {
        if (PMIX_SUCCESS != (rc = pmix_bfrop_store_data_type(regtypes, buffer, PMIX_INT32))) {
            return rc;
        }
    }
    PMIX_BFROPS_PACK_TYPE(rc, buffer, &num_vals, 1, PMIX_INT32, regtypes);
    if (PMIX_SUCCESS != rc) {
        return rc;
    }
    /* Pack the value(s) */
    return pmix_bfrops_base_pack_buffer(regtypes, buffer, src, num_vals, type);
}

pmix_status_t pmix_bfrops_base_pack_buffer(pmix_pointer_array_t *regtypes, pmix_buffer_t *buffer,
                                           const void *src, int32_t num_vals, pmix_data_type_t type)
{
    pmix_status_t rc;

    pmix_output_verbose(20, pmix_bfrops_base_framework.framework_output,
                        "pmix_bfrops_base_pack_buffer( %p, %p, %lu, %d )\n", (void *) buffer, src,
                        (long unsigned int) num_vals, (int) type);

    /* Pack the declared data type */
    if (PMIX_BFROP_BUFFER_FULLY_DESC == buffer->type) {
        if (PMIX_SUCCESS != (rc = pmix_bfrop_store_data_type(regtypes, buffer, type))) {
            return rc;
        }
    }
    PMIX_BFROPS_PACK_TYPE(rc, buffer, src, num_vals, type, regtypes);
    return rc;
}

/* PACK FUNCTIONS FOR GENERIC SYSTEM TYPES */

/*
 * BOOL
 */
pmix_status_t pmix_bfrops_base_pack_bool(pmix_pointer_array_t *regtypes, pmix_buffer_t *buffer,
                                         const void *src, int32_t num_vals, pmix_data_type_t type)
{
    uint8_t *dst;
    int32_t i;
    bool *s = (bool *) src;

    pmix_output_verbose(20, pmix_bfrops_base_framework.framework_output,
                        "pmix_bfrops_base_pack_bool * %d\n", num_vals);

    PMIX_HIDE_UNUSED_PARAMS(regtypes, type);

    /* check to see if buffer needs extending */
    if (NULL == (dst = (uint8_t *) pmix_bfrop_buffer_extend(buffer, num_vals))) {
        return PMIX_ERR_OUT_OF_RESOURCE;
    }

    /* store the data */
    for (i = 0; i < num_vals; i++) {
        if (s[i]) {
            dst[i] = 1;
        } else {
            dst[i] = 0;
        }
    }

    /* update buffer pointers */
    buffer->pack_ptr += num_vals;
    buffer->bytes_used += num_vals;

    return PMIX_SUCCESS;
}

/*
 * INT
 */
pmix_status_t pmix_bfrops_base_pack_int(pmix_pointer_array_t *regtypes, pmix_buffer_t *buffer,
                                        const void *src, int32_t num_vals, pmix_data_type_t type)
{
    pmix_status_t ret;

    PMIX_HIDE_UNUSED_PARAMS(type);

    /* System types need to always be described so we can properly
       unpack them */
    if (PMIX_SUCCESS != (ret = pmix_bfrop_store_data_type(regtypes, buffer, BFROP_TYPE_INT))) {
        return ret;
    }

    /* Turn around and pack the real type */
    PMIX_BFROPS_PACK_TYPE(ret, buffer, src, num_vals, BFROP_TYPE_INT, regtypes);
    return ret;
}

/*
 * SIZE_T
 */
pmix_status_t pmix_bfrops_base_pack_sizet(pmix_pointer_array_t *regtypes, pmix_buffer_t *buffer,
                                          const void *src, int32_t num_vals, pmix_data_type_t type)
{
    int ret;

    PMIX_HIDE_UNUSED_PARAMS(type);

    /* System types need to always be described so we can properly
       unpack them. */
    if (PMIX_SUCCESS != (ret = pmix_bfrop_store_data_type(regtypes, buffer, BFROP_TYPE_SIZE_T))) {
        return ret;
    }

    PMIX_BFROPS_PACK_TYPE(ret, buffer, src, num_vals, BFROP_TYPE_SIZE_T, regtypes);
    return ret;
}

/*
 * PID_T
 */
pmix_status_t pmix_bfrops_base_pack_pid(pmix_pointer_array_t *regtypes, pmix_buffer_t *buffer,
                                        const void *src, int32_t num_vals, pmix_data_type_t type)
{
    int ret;

    PMIX_HIDE_UNUSED_PARAMS(type);

    /* System types need to always be described so we can properly
       unpack them. */
    if (PMIX_SUCCESS != (ret = pmix_bfrop_store_data_type(regtypes, buffer, BFROP_TYPE_PID_T))) {
        return ret;
    }

    /* Turn around and pack the real type */
    PMIX_BFROPS_PACK_TYPE(ret, buffer, src, num_vals, BFROP_TYPE_PID_T, regtypes);
    return ret;
}

/*
 * BYTE, CHAR, INT8
 */
pmix_status_t pmix_bfrops_base_pack_byte(pmix_pointer_array_t *regtypes, pmix_buffer_t *buffer,
                                         const void *src, int32_t num_vals, pmix_data_type_t type)
{
    char *dst;

    pmix_output_verbose(20, pmix_bfrops_base_framework.framework_output,
                        "pmix_bfrops_base_pack_byte * %d\n", num_vals);

    PMIX_HIDE_UNUSED_PARAMS(regtypes, type);

    /* check to see if buffer needs extending */
    if (NULL == (dst = pmix_bfrop_buffer_extend(buffer, num_vals))) {
        return PMIX_ERR_OUT_OF_RESOURCE;
    }

    /* store the data */
    memcpy(dst, src, num_vals);

    /* update buffer pointers */
    buffer->pack_ptr += num_vals;
    buffer->bytes_used += num_vals;

    return PMIX_SUCCESS;
}

/*
 * INT16
 */
pmix_status_t pmix_bfrops_base_pack_int16(pmix_pointer_array_t *regtypes, pmix_buffer_t *buffer,
                                          const void *src, int32_t num_vals, pmix_data_type_t type)
{
    int32_t i;
    uint16_t tmp, *srctmp = (uint16_t *) src;
    char *dst;

    pmix_output_verbose(20, pmix_bfrops_base_framework.framework_output,
                        "pmix_bfrops_base_pack_int16 * %d\n", num_vals);

    PMIX_HIDE_UNUSED_PARAMS(regtypes, type);

    /* check to see if buffer needs extending */
    if (NULL == (dst = pmix_bfrop_buffer_extend(buffer, num_vals * sizeof(tmp)))) {
        return PMIX_ERR_OUT_OF_RESOURCE;
    }

    for (i = 0; i < num_vals; ++i) {
        tmp = pmix_htons(srctmp[i]);
        memcpy(dst, &tmp, sizeof(tmp));
        dst += sizeof(tmp);
    }
    buffer->pack_ptr += num_vals * sizeof(tmp);
    buffer->bytes_used += num_vals * sizeof(tmp);

    return PMIX_SUCCESS;
}

/*
 * INT32
 */
pmix_status_t pmix_bfrops_base_pack_int32(pmix_pointer_array_t *regtypes, pmix_buffer_t *buffer,
                                          const void *src, int32_t num_vals, pmix_data_type_t type)
{
    int32_t i;
    uint32_t tmp, *srctmp = (uint32_t *) src;
    char *dst;

    pmix_output_verbose(20, pmix_bfrops_base_framework.framework_output,
                        "pmix_bfrops_base_pack_int32 * %d\n", num_vals);

    PMIX_HIDE_UNUSED_PARAMS(regtypes, type);

    /* check to see if buffer needs extending */
    if (NULL == (dst = pmix_bfrop_buffer_extend(buffer, num_vals * sizeof(tmp)))) {
        return PMIX_ERR_OUT_OF_RESOURCE;
    }
    for (i = 0; i < num_vals; ++i) {
        tmp = htonl(srctmp[i]);
        memcpy(dst, &tmp, sizeof(tmp));
        dst += sizeof(tmp);
    }
    buffer->pack_ptr += num_vals * sizeof(tmp);
    buffer->bytes_used += num_vals * sizeof(tmp);

    return PMIX_SUCCESS;
}

/*
 * INT64
 */
pmix_status_t pmix_bfrops_base_pack_int64(pmix_pointer_array_t *regtypes, pmix_buffer_t *buffer,
                                          const void *src, int32_t num_vals, pmix_data_type_t type)
{
    int32_t i;
    uint64_t tmp, tmp2;
    char *dst;
    size_t bytes_packed = num_vals * sizeof(tmp);

    pmix_output_verbose(20, pmix_bfrops_base_framework.framework_output,
                        "pmix_bfrops_base_pack_int64 * %d\n", num_vals);

    PMIX_HIDE_UNUSED_PARAMS(regtypes, type);

    /* check to see if buffer needs extending */
    if (NULL == (dst = pmix_bfrop_buffer_extend(buffer, bytes_packed))) {
        return PMIX_ERR_OUT_OF_RESOURCE;
    }

    for (i = 0; i < num_vals; ++i) {
        memcpy(&tmp2, (char *) src + i * sizeof(uint64_t), sizeof(uint64_t));
        tmp = pmix_hton64(tmp2);
        memcpy(dst, &tmp, sizeof(tmp));
        dst += sizeof(tmp);
    }
    buffer->pack_ptr += bytes_packed;
    buffer->bytes_used += bytes_packed;

    return PMIX_SUCCESS;
}

/*
 * STRING
 */
pmix_status_t pmix_bfrops_base_pack_string(pmix_pointer_array_t *regtypes, pmix_buffer_t *buffer,
                                           const void *src, int32_t num_vals, pmix_data_type_t type)
{
    int ret = PMIX_SUCCESS;
    int32_t i, len;
    char **ssrc = (char **) src;

    PMIX_HIDE_UNUSED_PARAMS(type);

    for (i = 0; i < num_vals; ++i) {
        if (NULL == ssrc[i]) { /* got zero-length string/NULL pointer - store NULL */
            len = 0;
            PMIX_BFROPS_PACK_TYPE(ret, buffer, &len, 1, PMIX_INT32, regtypes);
            if (PMIX_SUCCESS != ret) {
                return ret;
            }
        } else {
            len = (int32_t) strlen(ssrc[i]) + 1; // retain the NULL terminator
            PMIX_BFROPS_PACK_TYPE(ret, buffer, &len, 1, PMIX_INT32, regtypes);
            if (PMIX_SUCCESS != ret) {
                return ret;
            }
            PMIX_BFROPS_PACK_TYPE(ret, buffer, ssrc[i], len, PMIX_BYTE, regtypes);
            if (PMIX_SUCCESS != ret) {
                return ret;
            }
        }
    }
    return ret;
}

/* FLOAT */
pmix_status_t pmix_bfrops_base_pack_float(pmix_pointer_array_t *regtypes, pmix_buffer_t *buffer,
                                          const void *src, int32_t num_vals, pmix_data_type_t type)
{
    int ret = PMIX_SUCCESS;
    int32_t i;
    float *ssrc = (float *) src;
    char *convert;

    PMIX_HIDE_UNUSED_PARAMS(type);

    for (i = 0; i < num_vals; ++i) {
        ret = asprintf(&convert, "%f", ssrc[i]);
        if (0 > ret) {
            return PMIX_ERR_OUT_OF_RESOURCE;
        }
        PMIX_BFROPS_PACK_TYPE(ret, buffer, &convert, 1, PMIX_STRING, regtypes);
        if (PMIX_SUCCESS != ret) {
            free(convert);
            return ret;
        }
        free(convert);
    }
    return PMIX_SUCCESS;
}

/* DOUBLE */
pmix_status_t pmix_bfrops_base_pack_double(pmix_pointer_array_t *regtypes, pmix_buffer_t *buffer,
                                           const void *src, int32_t num_vals, pmix_data_type_t type)
{
    int ret = PMIX_SUCCESS;
    int32_t i;
    double *ssrc = (double *) src;
    char *convert;

    PMIX_HIDE_UNUSED_PARAMS(type);

    for (i = 0; i < num_vals; ++i) {
        ret = asprintf(&convert, "%f", ssrc[i]);
        if (0 > ret) {
            return PMIX_ERR_OUT_OF_RESOURCE;
        }
        PMIX_BFROPS_PACK_TYPE(ret, buffer, &convert, 1, PMIX_STRING, regtypes);
        if (PMIX_SUCCESS != ret) {
            free(convert);
            return ret;
        }
        free(convert);
    }
    return PMIX_SUCCESS;
}

/* TIMEVAL */
pmix_status_t pmix_bfrops_base_pack_timeval(pmix_pointer_array_t *regtypes, pmix_buffer_t *buffer,
                                            const void *src, int32_t num_vals,
                                            pmix_data_type_t type)
{
    int64_t tmp[2];
    int ret = PMIX_SUCCESS;
    int32_t i;
    struct timeval *ssrc = (struct timeval *) src;

    PMIX_HIDE_UNUSED_PARAMS(type);

    for (i = 0; i < num_vals; ++i) {
        tmp[0] = (int64_t) ssrc[i].tv_sec;
        tmp[1] = (int64_t) ssrc[i].tv_usec;
        PMIX_BFROPS_PACK_TYPE(ret, buffer, tmp, 2, PMIX_INT64, regtypes);
        if (PMIX_SUCCESS != ret) {
            return ret;
        }
    }
    return PMIX_SUCCESS;
}

/* TIME */
pmix_status_t pmix_bfrops_base_pack_time(pmix_pointer_array_t *regtypes, pmix_buffer_t *buffer,
                                         const void *src, int32_t num_vals, pmix_data_type_t type)
{
    int ret = PMIX_SUCCESS;
    int32_t i;
    time_t *ssrc = (time_t *) src;
    uint64_t ui64;

    PMIX_HIDE_UNUSED_PARAMS(type);

    /* time_t is a system-dependent size, so cast it
     * to uint64_t as a generic safe size
     */
    for (i = 0; i < num_vals; ++i) {
        ui64 = (uint64_t) ssrc[i];
        PMIX_BFROPS_PACK_TYPE(ret, buffer, &ui64, 1, PMIX_UINT64, regtypes);
        if (PMIX_SUCCESS != ret) {
            return ret;
        }
    }
    return PMIX_SUCCESS;
}

/* STATUS */
pmix_status_t pmix_bfrops_base_pack_status(pmix_pointer_array_t *regtypes, pmix_buffer_t *buffer,
                                           const void *src, int32_t num_vals, pmix_data_type_t type)
{
    int ret = PMIX_SUCCESS;
    int32_t i;
    pmix_status_t *ssrc = (pmix_status_t *) src;
    int32_t status;

    PMIX_HIDE_UNUSED_PARAMS(type);

    for (i = 0; i < num_vals; ++i) {
        status = (int32_t) ssrc[i];
        PMIX_BFROPS_PACK_TYPE(ret, buffer, &status, 1, PMIX_INT32, regtypes);
        if (PMIX_SUCCESS != ret) {
            PMIX_ERROR_LOG(ret);
            return ret;
        }
    }
    return PMIX_SUCCESS;
}

pmix_status_t pmix_bfrops_base_pack_buf(pmix_pointer_array_t *regtypes, pmix_buffer_t *buffer,
                                        const void *src, int32_t num_vals, pmix_data_type_t type)
{
    pmix_buffer_t *ptr;
    int32_t i;
    int ret;

    PMIX_HIDE_UNUSED_PARAMS(type);

    ptr = (pmix_buffer_t *) src;

    for (i = 0; i < num_vals; ++i) {
        /* pack the type of buffer */
        PMIX_BFROPS_PACK_TYPE(ret, buffer, &ptr[i].type, 1, PMIX_BYTE, regtypes);
        if (PMIX_SUCCESS != ret) {
            return ret;
        }
        /* pack the number of bytes */
        PMIX_BFROPS_PACK_TYPE(ret, buffer, &ptr[i].bytes_used, 1, PMIX_SIZE, regtypes);
        if (PMIX_SUCCESS != ret) {
            return ret;
        }
        /* pack the bytes */
        if (0 < ptr[i].bytes_used) {
            PMIX_BFROPS_PACK_TYPE(ret, buffer, ptr[i].base_ptr, ptr[i].bytes_used, PMIX_BYTE,
                                  regtypes);
            if (PMIX_SUCCESS != ret) {
                return ret;
            }
        }
    }
    return PMIX_SUCCESS;
}

pmix_status_t pmix_bfrops_base_pack_bo(pmix_pointer_array_t *regtypes, pmix_buffer_t *buffer,
                                       const void *src, int32_t num_vals, pmix_data_type_t type)
{
    int ret;
    int i;
    pmix_byte_object_t *bo;

    PMIX_HIDE_UNUSED_PARAMS(type);

    bo = (pmix_byte_object_t *) src;
    for (i = 0; i < num_vals; i++) {
        PMIX_BFROPS_PACK_TYPE(ret, buffer, &bo[i].size, 1, PMIX_SIZE, regtypes);
        if (PMIX_SUCCESS != ret) {
            return ret;
        }
        if (0 < bo[i].size) {
            PMIX_BFROPS_PACK_TYPE(ret, buffer, bo[i].bytes, bo[i].size, PMIX_BYTE, regtypes);
            if (PMIX_SUCCESS != ret) {
                return ret;
            }
        }
    }
    return PMIX_SUCCESS;
}

pmix_status_t pmix_bfrops_base_pack_proc(pmix_pointer_array_t *regtypes, pmix_buffer_t *buffer,
                                         const void *src, int32_t num_vals, pmix_data_type_t type)
{
    pmix_proc_t *proc;
    int32_t i;
    int ret;

    PMIX_HIDE_UNUSED_PARAMS(type);

    proc = (pmix_proc_t *) src;

    for (i = 0; i < num_vals; ++i) {
        char *ptr = proc[i].nspace;
        PMIX_BFROPS_PACK_TYPE(ret, buffer, &ptr, 1, PMIX_STRING, regtypes);
        if (PMIX_SUCCESS != ret) {
            return ret;
        }
        PMIX_BFROPS_PACK_TYPE(ret, buffer, &proc[i].rank, 1, PMIX_PROC_RANK, regtypes);
        if (PMIX_SUCCESS != ret) {
            return ret;
        }
    }
    return PMIX_SUCCESS;
}

/* PMIX_VALUE */
pmix_status_t pmix_bfrops_base_pack_value(pmix_pointer_array_t *regtypes, pmix_buffer_t *buffer,
                                          const void *src, int32_t num_vals, pmix_data_type_t type)
{
    pmix_value_t *ptr;
    int32_t i;
    int ret;

    PMIX_HIDE_UNUSED_PARAMS(type);

    ptr = (pmix_value_t *) src;

    for (i = 0; i < num_vals; ++i) {
        /* pack the type */
        if (PMIX_SUCCESS != (ret = pmix_bfrop_store_data_type(regtypes, buffer, ptr[i].type))) {
            return ret;
        }
        /* now pack the right field */
        if (PMIX_SUCCESS != (ret = pmix_bfrops_base_pack_val(regtypes, buffer, &ptr[i]))) {
            return ret;
        }
    }
    return PMIX_SUCCESS;
}

pmix_status_t pmix_bfrops_base_pack_info(pmix_pointer_array_t *regtypes, pmix_buffer_t *buffer,
                                         const void *src, int32_t num_vals, pmix_data_type_t type)
{
    pmix_info_t *info;
    int32_t i;
    int ret;
    char *foo;

    PMIX_HIDE_UNUSED_PARAMS(type);

    info = (pmix_info_t *) src;

    for (i = 0; i < num_vals; ++i) {
        /* pack key */
        foo = info[i].key;
        PMIX_BFROPS_PACK_TYPE(ret, buffer, &foo, 1, PMIX_STRING, regtypes);
        if (PMIX_SUCCESS != ret) {
            return ret;
        }
        /* pack info directives */
        PMIX_BFROPS_PACK_TYPE(ret, buffer, &info[i].flags, 1, PMIX_INFO_DIRECTIVES, regtypes);
        if (PMIX_SUCCESS != ret) {
            return ret;
        }
        /* pack the type */
        if (PMIX_SUCCESS
            != (ret = pmix_bfrop_store_data_type(regtypes, buffer, info[i].value.type))) {
            return ret;
        }
        /* pack value */
        if (PMIX_SUCCESS != (ret = pmix_bfrops_base_pack_val(regtypes, buffer, &info[i].value))) {
            return ret;
        }
    }
    return PMIX_SUCCESS;
}

pmix_status_t pmix_bfrops_base_pack_pdata(pmix_pointer_array_t *regtypes, pmix_buffer_t *buffer,
                                          const void *src, int32_t num_vals, pmix_data_type_t type)
{
    pmix_pdata_t *pdata;
    int32_t i;
    int ret;
    char *foo;

    PMIX_HIDE_UNUSED_PARAMS(type);

    pdata = (pmix_pdata_t *) src;

    for (i = 0; i < num_vals; ++i) {
        /* pack the proc */
        PMIX_BFROPS_PACK_TYPE(ret, buffer, &pdata[i].proc, 1, PMIX_PROC, regtypes);
        if (PMIX_SUCCESS != ret) {
            return ret;
        }
        /* pack key */
        foo = pdata[i].key;
        PMIX_BFROPS_PACK_TYPE(ret, buffer, &foo, 1, PMIX_STRING, regtypes);
        if (PMIX_SUCCESS != ret) {
            PMIX_ERROR_LOG(ret);
            return ret;
        }
        /* pack the type */
        if (PMIX_SUCCESS
            != (ret = pmix_bfrop_store_data_type(regtypes, buffer, pdata[i].value.type))) {
            PMIX_ERROR_LOG(ret);
            return ret;
        }
        /* pack value */
        if (PMIX_SUCCESS != (ret = pmix_bfrops_base_pack_val(regtypes, buffer, &pdata[i].value))) {
            PMIX_ERROR_LOG(ret);
            return ret;
        }
    }
    return PMIX_SUCCESS;
}

pmix_status_t pmix_bfrops_base_pack_app(pmix_pointer_array_t *regtypes, pmix_buffer_t *buffer,
                                        const void *src, int32_t num_vals, pmix_data_type_t type)
{
    pmix_app_t *app;
    int32_t i, j, nvals;
    int ret;

    PMIX_HIDE_UNUSED_PARAMS(type);

    app = (pmix_app_t *) src;

    for (i = 0; i < num_vals; ++i) {
        PMIX_BFROPS_PACK_TYPE(ret, buffer, &app[i].cmd, 1, PMIX_STRING, regtypes);
        if (PMIX_SUCCESS != ret) {
            return ret;
        }
        /* argv */
        nvals = pmix_argv_count(app[i].argv);
        /* although nvals is technically an int32, we have to pack it
         * as a generic int due to a typo in earlier release series. This
         * preserves the ordering of bytes in the packed buffer as it
         * includes a tag indicating the actual size of the value. No
         * harm is done as generic int is equivalent to int32 on all
         * current systems - just something to watch out for in the
         * future should someone someday change the size of "int" */
        PMIX_BFROPS_PACK_TYPE(ret, buffer, &nvals, 1, PMIX_INT, regtypes);
        if (PMIX_SUCCESS != ret) {
            return ret;
        }
        for (j = 0; j < nvals; j++) {
            PMIX_BFROPS_PACK_TYPE(ret, buffer, &app[i].argv[j], 1, PMIX_STRING, regtypes);
            if (PMIX_SUCCESS != ret) {
                return ret;
            }
        }
        /* env */
        nvals = pmix_argv_count(app[i].env);
        PMIX_BFROPS_PACK_TYPE(ret, buffer, &nvals, 1, PMIX_INT32, regtypes);
        if (PMIX_SUCCESS != ret) {
            return ret;
        }
        for (j = 0; j < nvals; j++) {
            PMIX_BFROPS_PACK_TYPE(ret, buffer, &app[i].env[j], 1, PMIX_STRING, regtypes);
            if (PMIX_SUCCESS != ret) {
                return ret;
            }
        }
        /* cwd */
        PMIX_BFROPS_PACK_TYPE(ret, buffer, &app[i].cwd, 1, PMIX_STRING, regtypes);
        if (PMIX_SUCCESS != ret) {
            return ret;
        }
        /* maxprocs */
        PMIX_BFROPS_PACK_TYPE(ret, buffer, &app[i].maxprocs, 1, PMIX_INT, regtypes);
        if (PMIX_SUCCESS != ret) {
            return ret;
        }
        /* info array */
        PMIX_BFROPS_PACK_TYPE(ret, buffer, &app[i].ninfo, 1, PMIX_SIZE, regtypes);
        if (PMIX_SUCCESS != ret) {
            return ret;
        }
        if (0 < app[i].ninfo) {
            PMIX_BFROPS_PACK_TYPE(ret, buffer, app[i].info, app[i].ninfo, PMIX_INFO, regtypes);
            if (PMIX_SUCCESS != ret) {
                return ret;
            }
        }
    }
    return PMIX_SUCCESS;
}

pmix_status_t pmix_bfrops_base_pack_kval(pmix_pointer_array_t *regtypes, pmix_buffer_t *buffer,
                                         const void *src, int32_t num_vals, pmix_data_type_t type)
{
    pmix_kval_t *ptr;
    int32_t i;
    int ret;

    PMIX_HIDE_UNUSED_PARAMS(type);

    ptr = (pmix_kval_t *) src;

    for (i = 0; i < num_vals; ++i) {
        /* pack the key */
        PMIX_BFROPS_PACK_TYPE(ret, buffer, &ptr[i].key, 1, PMIX_STRING, regtypes);
        if (PMIX_SUCCESS != ret) {
            return ret;
        }
        /* pack the value */
        PMIX_BFROPS_PACK_TYPE(ret, buffer, ptr[i].value, 1, PMIX_VALUE, regtypes);
        if (PMIX_SUCCESS != ret) {
            return ret;
        }
    }
    return PMIX_SUCCESS;
}

pmix_status_t pmix_bfrops_base_pack_persist(pmix_pointer_array_t *regtypes, pmix_buffer_t *buffer,
                                            const void *src, int32_t num_vals,
                                            pmix_data_type_t type)
{
    pmix_status_t ret;
    PMIX_HIDE_UNUSED_PARAMS(type);

    PMIX_BFROPS_PACK_TYPE(ret, buffer, src, num_vals, PMIX_BYTE, regtypes);
    return ret;
}

pmix_status_t pmix_bfrops_base_pack_datatype(pmix_pointer_array_t *regtypes, pmix_buffer_t *buffer,
                                             const void *src, int32_t num_vals,
                                             pmix_data_type_t type)
{
    pmix_status_t ret;
    PMIX_HIDE_UNUSED_PARAMS(type);

    PMIX_BFROPS_PACK_TYPE(ret, buffer, src, num_vals, PMIX_UINT16, regtypes);
    return ret;
}

pmix_status_t pmix_bfrops_base_pack_ptr(pmix_pointer_array_t *regtypes, pmix_buffer_t *buffer,
                                        const void *src, int32_t num_vals, pmix_data_type_t type)
{
    pmix_status_t ret;
    uint8_t foo = 1;

    PMIX_HIDE_UNUSED_PARAMS(src, num_vals, type);

    /* it obviously makes no sense to pack a pointer and
     * send it somewhere else, so we just pack a sentinel */
    PMIX_BFROPS_PACK_TYPE(ret, buffer, &foo, 1, PMIX_UINT8, regtypes);
    return ret;
}

pmix_status_t pmix_bfrops_base_pack_scope(pmix_pointer_array_t *regtypes, pmix_buffer_t *buffer,
                                          const void *src, int32_t num_vals, pmix_data_type_t type)
{
    pmix_status_t ret;

    PMIX_HIDE_UNUSED_PARAMS(type);

    PMIX_BFROPS_PACK_TYPE(ret, buffer, src, num_vals, PMIX_UINT8, regtypes);
    return ret;
}

pmix_status_t pmix_bfrops_base_pack_range(pmix_pointer_array_t *regtypes, pmix_buffer_t *buffer,
                                          const void *src, int32_t num_vals, pmix_data_type_t type)
{
    pmix_status_t ret;

    PMIX_HIDE_UNUSED_PARAMS(type);

    PMIX_BFROPS_PACK_TYPE(ret, buffer, src, num_vals, PMIX_UINT8, regtypes);
    return ret;
}

pmix_status_t pmix_bfrops_base_pack_cmd(pmix_pointer_array_t *regtypes, pmix_buffer_t *buffer,
                                        const void *src, int32_t num_vals, pmix_data_type_t type)
{
    pmix_status_t ret;

    PMIX_HIDE_UNUSED_PARAMS(type);

    PMIX_BFROPS_PACK_TYPE(ret, buffer, src, num_vals, PMIX_UINT8, regtypes);
    return ret;
}

pmix_status_t pmix_bfrops_base_pack_info_directives(pmix_pointer_array_t *regtypes,
                                                    pmix_buffer_t *buffer, const void *src,
                                                    int32_t num_vals, pmix_data_type_t type)
{
    pmix_status_t ret;

    PMIX_HIDE_UNUSED_PARAMS(type);

    PMIX_BFROPS_PACK_TYPE(ret, buffer, src, num_vals, PMIX_UINT32, regtypes);
    return ret;
}

pmix_status_t pmix_bfrops_base_pack_pstate(pmix_pointer_array_t *regtypes, pmix_buffer_t *buffer,
                                           const void *src, int32_t num_vals, pmix_data_type_t type)
{
    pmix_status_t ret;

    PMIX_HIDE_UNUSED_PARAMS(type);

    PMIX_BFROPS_PACK_TYPE(ret, buffer, src, num_vals, PMIX_UINT8, regtypes);
    return ret;
}

pmix_status_t pmix_bfrops_base_pack_pinfo(pmix_pointer_array_t *regtypes, pmix_buffer_t *buffer,
                                          const void *src, int32_t num_vals, pmix_data_type_t type)
{
    pmix_proc_info_t *pinfo = (pmix_proc_info_t *) src;
    pmix_status_t ret;
    int32_t i;

    PMIX_HIDE_UNUSED_PARAMS(type);

    for (i = 0; i < num_vals; i++) {
        /* pack the proc identifier */
        PMIX_BFROPS_PACK_TYPE(ret, buffer, &pinfo[i].proc, 1, PMIX_PROC, regtypes);
        if (PMIX_SUCCESS != ret) {
            return ret;
        }
        /* pack the hostname and exec */
        PMIX_BFROPS_PACK_TYPE(ret, buffer, &pinfo[i].hostname, 1, PMIX_STRING, regtypes);
        if (PMIX_SUCCESS != ret) {
            return ret;
        }
        PMIX_BFROPS_PACK_TYPE(ret, buffer, &pinfo[i].executable_name, 1, PMIX_STRING, regtypes);
        if (PMIX_SUCCESS != ret) {
            return ret;
        }
        /* pack the pid and state */
        PMIX_BFROPS_PACK_TYPE(ret, buffer, &pinfo[i].pid, 1, PMIX_PID, regtypes);
        if (PMIX_SUCCESS != ret) {
            return ret;
        }
        PMIX_BFROPS_PACK_TYPE(ret, buffer, &pinfo[i].state, 1, PMIX_PROC_STATE, regtypes);
        if (PMIX_SUCCESS != ret) {
            return ret;
        }
    }
    return PMIX_SUCCESS;
}

pmix_status_t pmix_bfrops_base_pack_darray(pmix_pointer_array_t *regtypes, pmix_buffer_t *buffer,
                                           const void *src, int32_t num_vals, pmix_data_type_t type)
{
    pmix_data_array_t *p = (pmix_data_array_t *) src;
    pmix_status_t ret;
    int32_t i;

    PMIX_HIDE_UNUSED_PARAMS(type);

    for (i = 0; i < num_vals; i++) {
        /* pack the actual type in the array */
        if (PMIX_SUCCESS != (ret = pmix_bfrop_store_data_type(regtypes, buffer, p[i].type))) {
            return ret;
        }
        /* pack the number of array elements */
        PMIX_BFROPS_PACK_TYPE(ret, buffer, &p[i].size, 1, PMIX_SIZE, regtypes);
        if (PMIX_SUCCESS != ret) {
            return ret;
        }
        if (0 == p[i].size || PMIX_UNDEF == p[i].type) {
            /* nothing left to do */
            continue;
        }
        /* pack the actual elements */
        PMIX_BFROPS_PACK_TYPE(ret, buffer, p[i].array, p[i].size, p[i].type, regtypes);
        if (PMIX_ERR_UNKNOWN_DATA_TYPE == ret) {
            pmix_output(0, "PACK-PMIX-VALUE[%s:%d]: UNSUPPORTED TYPE %d", __FILE__, __LINE__,
                        (int) p[i].type);
        }
        if (PMIX_SUCCESS != ret) {
            return ret;
        }
    }
    return PMIX_SUCCESS;
}

pmix_status_t pmix_bfrops_base_pack_rank(pmix_pointer_array_t *regtypes, pmix_buffer_t *buffer,
                                         const void *src, int32_t num_vals, pmix_data_type_t type)
{
    pmix_status_t ret;

    PMIX_HIDE_UNUSED_PARAMS(type);

    PMIX_BFROPS_PACK_TYPE(ret, buffer, src, num_vals, PMIX_UINT32, regtypes);
    return ret;
}

pmix_status_t pmix_bfrops_base_pack_query(pmix_pointer_array_t *regtypes, pmix_buffer_t *buffer,
                                          const void *src, int32_t num_vals, pmix_data_type_t type)
{
    pmix_query_t *pq = (pmix_query_t *) src;
    pmix_status_t ret;
    int32_t i;
    int32_t nkeys;

    PMIX_HIDE_UNUSED_PARAMS(type);

    for (i = 0; i < num_vals; i++) {
        /* pack the number of keys */
        nkeys = pmix_argv_count(pq[i].keys);
        PMIX_BFROPS_PACK_TYPE(ret, buffer, &nkeys, 1, PMIX_INT32, regtypes);
        if (PMIX_SUCCESS != ret) {
            return ret;
        }
        if (0 < nkeys) {
            /* pack the keys */
            PMIX_BFROPS_PACK_TYPE(ret, buffer, pq[i].keys, nkeys, PMIX_STRING, regtypes);
            if (PMIX_SUCCESS != ret) {
                return ret;
            }
        }
        /* pack the number of qualifiers */
        PMIX_BFROPS_PACK_TYPE(ret, buffer, &pq[i].nqual, 1, PMIX_SIZE, regtypes);
        if (PMIX_SUCCESS != ret) {
            return ret;
        }
        if (0 < pq[i].nqual) {
            /* pack any provided qualifiers */
            PMIX_BFROPS_PACK_TYPE(ret, buffer, pq[i].qualifiers, pq[i].nqual, PMIX_INFO, regtypes);
            if (PMIX_SUCCESS != ret) {
                return ret;
            }
        }
    }
    return PMIX_SUCCESS;
}

/********************/
/* PACK FUNCTIONS FOR VALUE TYPES */
pmix_status_t pmix_bfrops_base_pack_val(pmix_pointer_array_t *regtypes, pmix_buffer_t *buffer,
                                        pmix_value_t *p)
{
    pmix_status_t ret;

    switch (p->type) {
    case PMIX_UNDEF:
        break;
    /* need to callout all the fields that are pointers to
     * a data object as opposed to a simple value */
    case PMIX_PROC:
    case PMIX_PROC_NSPACE:
    case PMIX_PROC_INFO:
    case PMIX_DATA_ARRAY:
    case PMIX_COORD:
    case PMIX_TOPO:
    case PMIX_PROC_CPUSET:
    case PMIX_GEOMETRY:
    case PMIX_DEVICE_DIST:
    case PMIX_ENDPOINT:
    case PMIX_REGATTR:
    case PMIX_PROC_STATS:
    case PMIX_DISK_STATS:
    case PMIX_NET_STATS:
    case PMIX_NODE_STATS:
        PMIX_BFROPS_PACK_TYPE(ret, buffer, p->data.ptr, 1, p->type, regtypes);
        if (PMIX_SUCCESS != ret) {
            return ret;
        }
        break;
    default:
        /* pass the address of the value instead of the value itself */
        PMIX_BFROPS_PACK_TYPE(ret, buffer, &p->data, 1, p->type, regtypes);
        if (PMIX_ERR_UNKNOWN_DATA_TYPE == ret) {
            pmix_output(0, "PACK-PMIX-VALUE[%s:%d]: UNSUPPORTED TYPE %d", __FILE__, __LINE__,
                        (int) p->type);
            return PMIX_ERROR;
        } else if (PMIX_SUCCESS != ret) {
            return ret;
        }
    }
    return PMIX_SUCCESS;
}

pmix_status_t pmix_bfrops_base_pack_alloc_directive(pmix_pointer_array_t *regtypes,
                                                    pmix_buffer_t *buffer, const void *src,
                                                    int32_t num_vals, pmix_data_type_t type)
{
    pmix_status_t ret;

    PMIX_HIDE_UNUSED_PARAMS(type);

    PMIX_BFROPS_PACK_TYPE(ret, buffer, src, num_vals, PMIX_UINT8, regtypes);
    return ret;
}

pmix_status_t pmix_bfrops_base_pack_iof_channel(pmix_pointer_array_t *regtypes,
                                                pmix_buffer_t *buffer, const void *src,
                                                int32_t num_vals, pmix_data_type_t type)
{
    pmix_status_t ret;

    PMIX_HIDE_UNUSED_PARAMS(type);

    PMIX_BFROPS_PACK_TYPE(ret, buffer, src, num_vals, PMIX_UINT16, regtypes);
    return ret;
}

pmix_status_t pmix_bfrops_base_pack_envar(pmix_pointer_array_t *regtypes, pmix_buffer_t *buffer,
                                          const void *src, int32_t num_vals, pmix_data_type_t type)
{
    pmix_envar_t *ptr = (pmix_envar_t *) src;
    int32_t i;
    pmix_status_t ret;

    PMIX_HIDE_UNUSED_PARAMS(type);

    for (i = 0; i < num_vals; ++i) {
        /* pack the name */
        PMIX_BFROPS_PACK_TYPE(ret, buffer, &ptr[i].envar, 1, PMIX_STRING, regtypes);
        if (PMIX_SUCCESS != ret) {
            return ret;
        }
        /* pack the value */
        PMIX_BFROPS_PACK_TYPE(ret, buffer, &ptr[i].value, 1, PMIX_STRING, regtypes);
        if (PMIX_SUCCESS != ret) {
            return ret;
        }
        /* pack the separator */
        PMIX_BFROPS_PACK_TYPE(ret, buffer, &ptr[i].separator, 1, PMIX_BYTE, regtypes);
        if (PMIX_SUCCESS != ret) {
            return ret;
        }
    }
    return PMIX_SUCCESS;
}

pmix_status_t pmix_bfrops_base_pack_coord(pmix_pointer_array_t *regtypes, pmix_buffer_t *buffer,
                                          const void *src, int32_t num_vals, pmix_data_type_t type)
{
    pmix_coord_t *ptr = (pmix_coord_t *) src;
    int32_t i;
    pmix_status_t ret;

    PMIX_HIDE_UNUSED_PARAMS(type);

    for (i = 0; i < num_vals; ++i) {
        /* pack the view */
        PMIX_BFROPS_PACK_TYPE(ret, buffer, &ptr[i].view, 1, PMIX_UINT8, regtypes);
        if (PMIX_SUCCESS != ret) {
            return ret;
        }
        /* pack the number of dimensions */
        PMIX_BFROPS_PACK_TYPE(ret, buffer, &ptr[i].dims, 1, PMIX_SIZE, regtypes);
        if (PMIX_SUCCESS != ret) {
            return ret;
        }
        /* pack the array of coordinates */
        PMIX_BFROPS_PACK_TYPE(ret, buffer, ptr[i].coord, ptr[i].dims, PMIX_UINT32, regtypes);
        if (PMIX_SUCCESS != ret) {
            return ret;
        }
    }
    return PMIX_SUCCESS;
}

pmix_status_t pmix_bfrops_base_pack_regattr(pmix_pointer_array_t *regtypes, pmix_buffer_t *buffer,
                                            const void *src, int32_t num_vals,
                                            pmix_data_type_t type)
{
    pmix_regattr_t *ptr = (pmix_regattr_t *) src;
    int32_t i, nd;
    pmix_status_t ret;
    char *foo;

    PMIX_HIDE_UNUSED_PARAMS(type);

    for (i = 0; i < num_vals; ++i) {
        /* pack the name */
        PMIX_BFROPS_PACK_TYPE(ret, buffer, &ptr[i].name, 1, PMIX_STRING, regtypes);
        if (PMIX_SUCCESS != ret) {
            return ret;
        }
        /* pack the string */
        foo = ptr[i].string;
        PMIX_BFROPS_PACK_TYPE(ret, buffer, &foo, 1, PMIX_STRING, regtypes);
        if (PMIX_SUCCESS != ret) {
            return ret;
        }
        /* pack the type */
        PMIX_BFROPS_PACK_TYPE(ret, buffer, &ptr[i].type, 1, PMIX_DATA_TYPE, regtypes);
        if (PMIX_SUCCESS != ret) {
            PMIX_ERROR_LOG(ret);
            return ret;
        }
        /* pack the description */
        nd = pmix_argv_count(ptr[i].description);
        PMIX_BFROPS_PACK_TYPE(ret, buffer, &nd, 1, PMIX_INT32, regtypes);
        if (PMIX_SUCCESS != ret) {
            PMIX_ERROR_LOG(ret);
            return ret;
        }
        PMIX_BFROPS_PACK_TYPE(ret, buffer, ptr[i].description, nd, PMIX_STRING, regtypes);
        if (PMIX_SUCCESS != ret) {
            PMIX_ERROR_LOG(ret);
            return ret;
        }
    }
    return PMIX_SUCCESS;
}

pmix_status_t pmix_bfrops_base_pack_regex(pmix_pointer_array_t *regtypes, pmix_buffer_t *buffer,
                                          const void *src, int32_t num_vals, pmix_data_type_t type)
{
    char **ptr = (char **) src;
    int32_t i;
    pmix_status_t ret;

    PMIX_HIDE_UNUSED_PARAMS(regtypes, type);

    for (i = 0; i < num_vals; ++i) {
        ret = pmix_preg.pack(buffer, ptr[i]);
        if (PMIX_SUCCESS != ret) {
            return ret;
        }
    }
    return PMIX_SUCCESS;
}

pmix_status_t pmix_bfrops_base_pack_jobstate(pmix_pointer_array_t *regtypes, pmix_buffer_t *buffer,
                                             const void *src, int32_t num_vals,
                                             pmix_data_type_t type)
{
    pmix_status_t ret;

    PMIX_HIDE_UNUSED_PARAMS(type);

    PMIX_BFROPS_PACK_TYPE(ret, buffer, src, num_vals, PMIX_UINT8, regtypes);
    return ret;
}

pmix_status_t pmix_bfrops_base_pack_linkstate(pmix_pointer_array_t *regtypes, pmix_buffer_t *buffer,
                                              const void *src, int32_t num_vals,
                                              pmix_data_type_t type)
{
    pmix_status_t ret;

    PMIX_HIDE_UNUSED_PARAMS(type);

    PMIX_BFROPS_PACK_TYPE(ret, buffer, src, num_vals, PMIX_UINT8, regtypes);
    return ret;
}

pmix_status_t pmix_bfrops_base_pack_cpuset(pmix_pointer_array_t *regtypes, pmix_buffer_t *buffer,
                                           const void *src, int32_t num_vals, pmix_data_type_t type)
{
    pmix_cpuset_t *ptr = (pmix_cpuset_t *) src;
    int32_t i;
    pmix_status_t ret;

    PMIX_HIDE_UNUSED_PARAMS(type);

    for (i = 0; i < num_vals; ++i) {
        ret = pmix_hwloc_pack_cpuset(buffer, &ptr[i], regtypes);
        if (PMIX_SUCCESS != ret) {
            return ret;
        }
    }
    return PMIX_SUCCESS;
}

pmix_status_t pmix_bfrops_base_pack_geometry(pmix_pointer_array_t *regtypes, pmix_buffer_t *buffer,
                                             const void *src, int32_t num_vals,
                                             pmix_data_type_t type)
{
    pmix_geometry_t *ptr = (pmix_geometry_t *) src;
    int32_t i;
    pmix_status_t ret;

    PMIX_HIDE_UNUSED_PARAMS(type);

    for (i = 0; i < num_vals; ++i) {
        PMIX_BFROPS_PACK_TYPE(ret, buffer, &ptr[i].fabric, 1, PMIX_SIZE, regtypes);
        if (PMIX_SUCCESS != ret) {
            return ret;
        }
        PMIX_BFROPS_PACK_TYPE(ret, buffer, &ptr[i].uuid, 1, PMIX_STRING, regtypes);
        if (PMIX_SUCCESS != ret) {
            return ret;
        }
        PMIX_BFROPS_PACK_TYPE(ret, buffer, &ptr[i].osname, 1, PMIX_STRING, regtypes);
        if (PMIX_SUCCESS != ret) {
            return ret;
        }
        PMIX_BFROPS_PACK_TYPE(ret, buffer, &ptr[i].ncoords, 1, PMIX_SIZE, regtypes);
        if (PMIX_SUCCESS != ret) {
            return ret;
        }
        PMIX_BFROPS_PACK_TYPE(ret, buffer, ptr[i].coordinates, ptr[i].ncoords, PMIX_COORD,
                              regtypes);
        if (PMIX_SUCCESS != ret) {
            return ret;
        }
    }
    return PMIX_SUCCESS;
}

pmix_status_t pmix_bfrops_base_pack_devdist(pmix_pointer_array_t *regtypes, pmix_buffer_t *buffer,
                                            const void *src, int32_t num_vals,
                                            pmix_data_type_t type)
{
    pmix_device_distance_t *ptr = (pmix_device_distance_t *) src;
    int32_t i;
    pmix_status_t ret;

    PMIX_HIDE_UNUSED_PARAMS(type);

    for (i = 0; i < num_vals; ++i) {
        PMIX_BFROPS_PACK_TYPE(ret, buffer, &ptr[i].uuid, 1, PMIX_STRING, regtypes);
        if (PMIX_SUCCESS != ret) {
            return ret;
        }
        PMIX_BFROPS_PACK_TYPE(ret, buffer, &ptr[i].osname, 1, PMIX_STRING, regtypes);
        if (PMIX_SUCCESS != ret) {
            return ret;
        }
        PMIX_BFROPS_PACK_TYPE(ret, buffer, &ptr[i].type, 1, PMIX_DEVTYPE, regtypes);
        if (PMIX_SUCCESS != ret) {
            return ret;
        }
        PMIX_BFROPS_PACK_TYPE(ret, buffer, &ptr[i].mindist, 1, PMIX_UINT16, regtypes);
        if (PMIX_SUCCESS != ret) {
            return ret;
        }
        PMIX_BFROPS_PACK_TYPE(ret, buffer, &ptr[i].maxdist, 1, PMIX_UINT16, regtypes);
        if (PMIX_SUCCESS != ret) {
            return ret;
        }
    }
    return PMIX_SUCCESS;
}

pmix_status_t pmix_bfrops_base_pack_endpoint(pmix_pointer_array_t *regtypes, pmix_buffer_t *buffer,
                                             const void *src, int32_t num_vals,
                                             pmix_data_type_t type)
{
    pmix_endpoint_t *ptr = (pmix_endpoint_t *) src;
    int32_t i;
    pmix_status_t ret;

    PMIX_HIDE_UNUSED_PARAMS(type);

    for (i = 0; i < num_vals; ++i) {
        PMIX_BFROPS_PACK_TYPE(ret, buffer, &ptr[i].uuid, 1, PMIX_STRING, regtypes);
        if (PMIX_SUCCESS != ret) {
            return ret;
        }
        PMIX_BFROPS_PACK_TYPE(ret, buffer, &ptr[i].osname, 1, PMIX_STRING, regtypes);
        if (PMIX_SUCCESS != ret) {
            return ret;
        }
        PMIX_BFROPS_PACK_TYPE(ret, buffer, &ptr[i].endpt.size, 1, PMIX_SIZE, regtypes);
        if (PMIX_SUCCESS != ret) {
            return ret;
        }
        if (0 < ptr[i].endpt.size) {
            PMIX_BFROPS_PACK_TYPE(ret, buffer, ptr[i].endpt.bytes, ptr[i].endpt.size, PMIX_BYTE,
                                  regtypes);
            if (PMIX_SUCCESS != ret) {
                return ret;
            }
        }
    }
    return PMIX_SUCCESS;
}

pmix_status_t pmix_bfrops_base_pack_topology(pmix_pointer_array_t *regtypes, pmix_buffer_t *buffer,
                                             const void *src, int32_t num_vals,
                                             pmix_data_type_t type)
{
    pmix_topology_t *ptr = (pmix_topology_t *) src;
    int32_t i;
    pmix_status_t ret;

    PMIX_HIDE_UNUSED_PARAMS(type);

    for (i = 0; i < num_vals; ++i) {
        /* call the framework to pack it */
        ret = pmix_hwloc_pack_topology(buffer, &ptr[i], regtypes);
        if (PMIX_SUCCESS != ret) {
            return ret;
        }
    }
    return PMIX_SUCCESS;
}

pmix_status_t pmix_bfrops_base_pack_devtype(pmix_pointer_array_t *regtypes, pmix_buffer_t *buffer,
                                            const void *src, int32_t num_vals,
                                            pmix_data_type_t type)
{
    pmix_status_t ret;

    PMIX_HIDE_UNUSED_PARAMS(type);

    PMIX_BFROPS_PACK_TYPE(ret, buffer, src, num_vals, PMIX_UINT64, regtypes);
    return ret;
}

pmix_status_t pmix_bfrops_base_pack_locality(pmix_pointer_array_t *regtypes, pmix_buffer_t *buffer,
                                             const void *src, int32_t num_vals,
                                             pmix_data_type_t type)
{
    pmix_status_t ret;

    PMIX_HIDE_UNUSED_PARAMS(type);

    PMIX_BFROPS_PACK_TYPE(ret, buffer, src, num_vals, PMIX_UINT16, regtypes);
    return ret;
}

pmix_status_t pmix_bfrops_base_pack_nspace(pmix_pointer_array_t *regtypes, pmix_buffer_t *buffer,
                                           const void *src, int32_t num_vals, pmix_data_type_t type)
{
    pmix_nspace_t *ptr = (pmix_nspace_t *) src;
    char *p;
    int32_t i;
    pmix_status_t ret;

    PMIX_HIDE_UNUSED_PARAMS(type);

    for (i = 0; i < num_vals; ++i) {
        p = (char *) ptr[i];
        PMIX_BFROPS_PACK_TYPE(ret, buffer, &p, 1, PMIX_STRING, regtypes);
        if (PMIX_SUCCESS != ret) {
            return ret;
        }
    }
    return PMIX_SUCCESS;
}

pmix_status_t pmix_bfrops_base_pack_pstats(pmix_pointer_array_t *regtypes, pmix_buffer_t *buffer,
                                           const void *src, int32_t num_vals, pmix_data_type_t type)
{
    pmix_proc_stats_t *ptr = (pmix_proc_stats_t *) src;
    int32_t i;
    int ret;
    char *cptr;

    PMIX_HIDE_UNUSED_PARAMS(type);

    for (i = 0; i < num_vals; ++i) {
        cptr = ptr[i].node;
        PMIX_BFROPS_PACK_TYPE(ret, buffer, &cptr, 1, PMIX_STRING, regtypes);
        if (PMIX_SUCCESS != ret) {
            return ret;
        }
        PMIX_BFROPS_PACK_TYPE(ret, buffer, &ptr[i].proc, 1, PMIX_PROC, regtypes);
        if (PMIX_SUCCESS != ret) {
            return ret;
        }
        PMIX_BFROPS_PACK_TYPE(ret, buffer, &ptr[i].pid, 1, PMIX_PID, regtypes);
        if (PMIX_SUCCESS != ret) {
            return ret;
        }
        cptr = ptr[i].cmd;
        PMIX_BFROPS_PACK_TYPE(ret, buffer, &cptr, 1, PMIX_STRING, regtypes);
        if (PMIX_SUCCESS != ret) {
            return ret;
        }
        PMIX_BFROPS_PACK_TYPE(ret, buffer, &ptr[i].state, 1, PMIX_BYTE, regtypes);
        if (PMIX_SUCCESS != ret) {
            return ret;
        }
        PMIX_BFROPS_PACK_TYPE(ret, buffer, &ptr[i].time, 1, PMIX_TIMEVAL, regtypes);
        if (PMIX_SUCCESS != ret) {
            return ret;
        }
        PMIX_BFROPS_PACK_TYPE(ret, buffer, &ptr[i].priority, 1, PMIX_INT32, regtypes);
        if (PMIX_SUCCESS != ret) {
            return ret;
        }
        PMIX_BFROPS_PACK_TYPE(ret, buffer, &ptr[i].num_threads, 1, PMIX_INT16, regtypes);
        if (PMIX_SUCCESS != ret) {
            return ret;
        }
        PMIX_BFROPS_PACK_TYPE(ret, buffer, &ptr[i].pss, 1, PMIX_FLOAT, regtypes);
        if (PMIX_SUCCESS != ret) {
            return ret;
        }
        PMIX_BFROPS_PACK_TYPE(ret, buffer, &ptr[i].vsize, 1, PMIX_FLOAT, regtypes);
        if (PMIX_SUCCESS != ret) {
            return ret;
        }
        PMIX_BFROPS_PACK_TYPE(ret, buffer, &ptr[i].rss, 1, PMIX_FLOAT, regtypes);
        if (PMIX_SUCCESS != ret) {
            return ret;
        }
        PMIX_BFROPS_PACK_TYPE(ret, buffer, &ptr[i].processor, 1, PMIX_INT16, regtypes);
        if (PMIX_SUCCESS != ret) {
            return ret;
        }
        PMIX_BFROPS_PACK_TYPE(ret, buffer, &ptr[i].sample_time, 1, PMIX_TIMEVAL, regtypes);
        if (PMIX_SUCCESS != ret) {
            return ret;
        }
    }

    return PMIX_SUCCESS;
}

pmix_status_t pmix_bfrops_base_pack_dkstats(pmix_pointer_array_t *regtypes, pmix_buffer_t *buffer,
                                            const void *src, int32_t num_vals,
                                            pmix_data_type_t type)
{
    pmix_disk_stats_t *ptr = (pmix_disk_stats_t *) src;
    int32_t i;
    int ret;
    char *cptr;

    PMIX_HIDE_UNUSED_PARAMS(type);

    for (i = 0; i < num_vals; ++i) {
        cptr = ptr[i].disk;
        PMIX_BFROPS_PACK_TYPE(ret, buffer, &cptr, 1, PMIX_STRING, regtypes);
        if (PMIX_SUCCESS != ret) {
            return ret;
        }
        PMIX_BFROPS_PACK_TYPE(ret, buffer, &ptr[i].num_reads_completed, 1, PMIX_UINT64, regtypes);
        if (PMIX_SUCCESS != ret) {
            return ret;
        }
        PMIX_BFROPS_PACK_TYPE(ret, buffer, &ptr[i].num_reads_merged, 1, PMIX_UINT64, regtypes);
        if (PMIX_SUCCESS != ret) {
            return ret;
        }
        PMIX_BFROPS_PACK_TYPE(ret, buffer, &ptr[i].num_sectors_read, 1, PMIX_UINT64, regtypes);
        if (PMIX_SUCCESS != ret) {
            return ret;
        }
        PMIX_BFROPS_PACK_TYPE(ret, buffer, &ptr[i].milliseconds_reading, 1, PMIX_UINT64, regtypes);
        if (PMIX_SUCCESS != ret) {
            return ret;
        }
        PMIX_BFROPS_PACK_TYPE(ret, buffer, &ptr[i].num_writes_completed, 1, PMIX_UINT64, regtypes);
        if (PMIX_SUCCESS != ret) {
            return ret;
        }
        PMIX_BFROPS_PACK_TYPE(ret, buffer, &ptr[i].num_writes_merged, 1, PMIX_UINT64, regtypes);
        if (PMIX_SUCCESS != ret) {
            return ret;
        }
        PMIX_BFROPS_PACK_TYPE(ret, buffer, &ptr[i].num_sectors_written, 1, PMIX_UINT64, regtypes);
        if (PMIX_SUCCESS != ret) {
            return ret;
        }
        PMIX_BFROPS_PACK_TYPE(ret, buffer, &ptr[i].milliseconds_writing, 1, PMIX_UINT64, regtypes);
        if (PMIX_SUCCESS != ret) {
            return ret;
        }
        PMIX_BFROPS_PACK_TYPE(ret, buffer, &ptr[i].num_ios_in_progress, 1, PMIX_UINT64, regtypes);
        if (PMIX_SUCCESS != ret) {
            return ret;
        }
        PMIX_BFROPS_PACK_TYPE(ret, buffer, &ptr[i].milliseconds_io, 1, PMIX_UINT64, regtypes);
        if (PMIX_SUCCESS != ret) {
            return ret;
        }
        PMIX_BFROPS_PACK_TYPE(ret, buffer, &ptr[i].weighted_milliseconds_io, 1, PMIX_UINT64,
                              regtypes);
        if (PMIX_SUCCESS != ret) {
            return ret;
        }
    }
    return PMIX_SUCCESS;
}

pmix_status_t pmix_bfrops_base_pack_netstats(pmix_pointer_array_t *regtypes, pmix_buffer_t *buffer,
                                             const void *src, int32_t num_vals,
                                             pmix_data_type_t type)
{
    pmix_net_stats_t *ptr = (pmix_net_stats_t *) src;
    int32_t i;
    int ret;
    char *cptr;

    PMIX_HIDE_UNUSED_PARAMS(type);

    for (i = 0; i < num_vals; ++i) {
        cptr = ptr[i].net_interface;
        PMIX_BFROPS_PACK_TYPE(ret, buffer, &cptr, 1, PMIX_STRING, regtypes);
        if (PMIX_SUCCESS != ret) {
            return ret;
        }
        PMIX_BFROPS_PACK_TYPE(ret, buffer, &ptr[i].num_bytes_recvd, 1, PMIX_UINT64, regtypes);
        if (PMIX_SUCCESS != ret) {
            return ret;
        }
        PMIX_BFROPS_PACK_TYPE(ret, buffer, &ptr[i].num_packets_recvd, 1, PMIX_UINT64, regtypes);
        if (PMIX_SUCCESS != ret) {
            return ret;
        }
        PMIX_BFROPS_PACK_TYPE(ret, buffer, &ptr[i].num_recv_errs, 1, PMIX_UINT64, regtypes);
        if (PMIX_SUCCESS != ret) {
            return ret;
        }
        PMIX_BFROPS_PACK_TYPE(ret, buffer, &ptr[i].num_bytes_sent, 1, PMIX_UINT64, regtypes);
        if (PMIX_SUCCESS != ret) {
            return ret;
        }
        PMIX_BFROPS_PACK_TYPE(ret, buffer, &ptr[i].num_packets_sent, 1, PMIX_UINT64, regtypes);
        if (PMIX_SUCCESS != ret) {
            return ret;
        }
        PMIX_BFROPS_PACK_TYPE(ret, buffer, &ptr[i].num_send_errs, 1, PMIX_UINT64, regtypes);
        if (PMIX_SUCCESS != ret) {
            return ret;
        }
    }
    return PMIX_SUCCESS;
}

pmix_status_t pmix_bfrops_base_pack_ndstats(pmix_pointer_array_t *regtypes, pmix_buffer_t *buffer,
                                            const void *src, int32_t num_vals,
                                            pmix_data_type_t type)
{
    pmix_node_stats_t *ptr = (pmix_node_stats_t *) src;
    int32_t i;
    int ret;
    char *cptr;

    PMIX_HIDE_UNUSED_PARAMS(type);

    for (i = 0; i < num_vals; ++i) {
        cptr = ptr[i].node;
        PMIX_BFROPS_PACK_TYPE(ret, buffer, &cptr, 1, PMIX_STRING, regtypes);
        if (PMIX_SUCCESS != ret) {
            return ret;
        }
        PMIX_BFROPS_PACK_TYPE(ret, buffer, &ptr[i].la, 1, PMIX_FLOAT, regtypes);
        if (PMIX_SUCCESS != ret) {
            return ret;
        }
        PMIX_BFROPS_PACK_TYPE(ret, buffer, &ptr[i].la5, 1, PMIX_FLOAT, regtypes);
        if (PMIX_SUCCESS != ret) {
            return ret;
        }
        PMIX_BFROPS_PACK_TYPE(ret, buffer, &ptr[i].la15, 1, PMIX_FLOAT, regtypes);
        if (PMIX_SUCCESS != ret) {
            return ret;
        }
        PMIX_BFROPS_PACK_TYPE(ret, buffer, &ptr[i].total_mem, 1, PMIX_FLOAT, regtypes);
        if (PMIX_SUCCESS != ret) {
            return ret;
        }
        PMIX_BFROPS_PACK_TYPE(ret, buffer, &ptr[i].free_mem, 1, PMIX_FLOAT, regtypes);
        if (PMIX_SUCCESS != ret) {
            return ret;
        }
        PMIX_BFROPS_PACK_TYPE(ret, buffer, &ptr[i].buffers, 1, PMIX_FLOAT, regtypes);
        if (PMIX_SUCCESS != ret) {
            return ret;
        }
        PMIX_BFROPS_PACK_TYPE(ret, buffer, &ptr[i].cached, 1, PMIX_FLOAT, regtypes);
        if (PMIX_SUCCESS != ret) {
            return ret;
        }
        PMIX_BFROPS_PACK_TYPE(ret, buffer, &ptr[i].swap_cached, 1, PMIX_FLOAT, regtypes);
        if (PMIX_SUCCESS != ret) {
            return ret;
        }
        PMIX_BFROPS_PACK_TYPE(ret, buffer, &ptr[i].swap_total, 1, PMIX_FLOAT, regtypes);
        if (PMIX_SUCCESS != ret) {
            return ret;
        }
        PMIX_BFROPS_PACK_TYPE(ret, buffer, &ptr[i].swap_free, 1, PMIX_FLOAT, regtypes);
        if (PMIX_SUCCESS != ret) {
            return ret;
        }
        PMIX_BFROPS_PACK_TYPE(ret, buffer, &ptr[i].mapped, 1, PMIX_FLOAT, regtypes);
        if (PMIX_SUCCESS != ret) {
            return ret;
        }
        PMIX_BFROPS_PACK_TYPE(ret, buffer, &ptr[i].sample_time, 1, PMIX_TIMEVAL, regtypes);
        if (PMIX_SUCCESS != ret) {
            return ret;
        }
        PMIX_BFROPS_PACK_TYPE(ret, buffer, &ptr[i].ndiskstats, 1, PMIX_SIZE, regtypes);
        if (PMIX_SUCCESS != ret) {
            return ret;
        }
        if (0 < ptr[i].ndiskstats) {
            PMIX_BFROPS_PACK_TYPE(ret, buffer, &ptr[i].diskstats, ptr[i].ndiskstats,
                                  PMIX_DISK_STATS, regtypes);
            if (PMIX_SUCCESS != ret) {
                return ret;
            }
        }
        PMIX_BFROPS_PACK_TYPE(ret, buffer, &ptr[i].nnetstats, 1, PMIX_SIZE, regtypes);
        if (PMIX_SUCCESS != ret) {
            return ret;
        }
        if (0 < ptr[i].nnetstats) {
            PMIX_BFROPS_PACK_TYPE(ret, buffer, &ptr[i].netstats, ptr[i].nnetstats, PMIX_NET_STATS,
                                  regtypes);
            if (PMIX_SUCCESS != ret) {
                return ret;
            }
        }
    }
    return PMIX_SUCCESS;
}

pmix_status_t pmix_bfrops_base_pack_dbuf(pmix_pointer_array_t *regtypes, pmix_buffer_t *buffer,
                                         const void *src, int32_t num_vals, pmix_data_type_t type)
{
    pmix_data_buffer_t *ptr = (pmix_data_buffer_t *) src;
    int32_t i;
    pmix_status_t ret;

    PMIX_HIDE_UNUSED_PARAMS(type);

    for (i = 0; i < num_vals; ++i) {
        PMIX_BFROPS_PACK_TYPE(ret, buffer, &ptr[i].bytes_used, 1, PMIX_SIZE, regtypes);
        if (PMIX_SUCCESS != ret) {
            return ret;
        }
        if (0 < ptr[i].bytes_used) {
            PMIX_BFROPS_PACK_TYPE(ret, buffer, ptr[i].base_ptr, ptr[i].bytes_used, PMIX_BYTE,
                                  regtypes);
            if (PMIX_SUCCESS != ret) {
                return ret;
            }
        }
    }
    return PMIX_SUCCESS;
}

pmix_status_t pmix_bfrops_base_pack_smed(pmix_pointer_array_t *regtypes, pmix_buffer_t *buffer,
                                         const void *src, int32_t num_vals, pmix_data_type_t type)
{
    pmix_status_t ret;

    PMIX_HIDE_UNUSED_PARAMS(type);

    PMIX_BFROPS_PACK_TYPE(ret, buffer, src, num_vals, PMIX_UINT64, regtypes);
    return ret;
}

pmix_status_t pmix_bfrops_base_pack_sacc(pmix_pointer_array_t *regtypes, pmix_buffer_t *buffer,
                                         const void *src, int32_t num_vals, pmix_data_type_t type)
{
    pmix_status_t ret;

    PMIX_HIDE_UNUSED_PARAMS(type);

    PMIX_BFROPS_PACK_TYPE(ret, buffer, src, num_vals, PMIX_UINT64, regtypes);
    return ret;
}

pmix_status_t pmix_bfrops_base_pack_spers(pmix_pointer_array_t *regtypes, pmix_buffer_t *buffer,
                                          const void *src, int32_t num_vals, pmix_data_type_t type)
{
    pmix_status_t ret;

    PMIX_HIDE_UNUSED_PARAMS(type);

    PMIX_BFROPS_PACK_TYPE(ret, buffer, src, num_vals, PMIX_UINT64, regtypes);
    return ret;
}

pmix_status_t pmix_bfrops_base_pack_satyp(pmix_pointer_array_t *regtypes, pmix_buffer_t *buffer,
                                          const void *src, int32_t num_vals, pmix_data_type_t type)
{
    pmix_status_t ret;

    PMIX_HIDE_UNUSED_PARAMS(type);

    PMIX_BFROPS_PACK_TYPE(ret, buffer, src, num_vals, PMIX_UINT16, regtypes);
    return ret;
}
