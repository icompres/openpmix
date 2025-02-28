/*
 * Copyright (c) 2013-2020 Intel, Inc.  All rights reserved.
 * Copyright (c) 2015      Artem Y. Polyakov <artpol84@gmail.com>.
 *                         All rights reserved.
 * Copyright (c) 2015      Research Organization for Information Science
 *                         and Technology (RIST). All rights reserved.
 * Copyright (c) 2015-2018 Mellanox Technologies, Inc.
 *                         All rights reserved.
 * Copyright (c) 2021-2022 Nanook Consulting  All rights reserved.
 * $COPYRIGHT$
 *
 * Additional copyrights may follow
 *
 * $HEADER$
 *
 */

#ifndef TEST_COMMON_H
#define TEST_COMMON_H

#include "src/include/pmix_config.h"
#include "pmix_common.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>

#include "src/class/pmix_list.h"
#include "src/include/pmix_globals.h"
#include "src/util/pmix_argv.h"

#define TEST_NAMESPACE  "smoky_nspace"
#define TEST_CREDENTIAL "dummy"

#define PMIX_WAIT_FOR_COMPLETION(m) \
    do {                            \
        while ((m)) {               \
            usleep(10);             \
        }                           \
    } while (0)

/* WARNING: pmix_test_output_prepare is currently not threadsafe!
 * fix it once needed!
 */
char *pmix_test_output_prepare(const char *fmt, ...);
extern int pmix_test_verbose;
extern FILE *file;

#define STRIPPED_FILE_NAME (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)

#define TEST_OUTPUT(x)                                                                         \
    {                                                                                          \
        struct timeval _tv;                                                                    \
        gettimeofday(&_tv, NULL);                                                              \
        double _ts = _tv.tv_sec + 1E-6 * (_tv).tv_usec;                                        \
        fprintf(file, "==%d== [%lf] %s:%s: %s\n", getpid(), _ts, STRIPPED_FILE_NAME, __func__, \
                pmix_test_output_prepare x);                                                   \
        fflush(file);                                                                          \
    }

// Write output without adding anything to it.
// Need for automate tests to receive "OK" string
#define TEST_OUTPUT_CLEAR(x)                                              \
    {                                                                     \
        fprintf(file, "==%d== %s", getpid(), pmix_test_output_prepare x); \
        fflush(file);                                                     \
    }

// Always write errors to the stderr
#define TEST_ERROR(x)                                                                            \
    {                                                                                            \
        struct timeval tv;                                                                       \
        gettimeofday(&tv, NULL);                                                                 \
        double ts = tv.tv_sec + 1E-6 * tv.tv_usec;                                               \
        fprintf(stderr, "==%d== [%lf] ERROR [%s:%d:%s]: %s\n", getpid(), ts, STRIPPED_FILE_NAME, \
                __LINE__, __func__, pmix_test_output_prepare x);                                 \
        fflush(stderr);                                                                          \
    }

#define TEST_VERBOSE_ON()  (pmix_test_verbose = 1)
#define TEST_VERBOSE_GET() (pmix_test_verbose)

#define TEST_VERBOSE(x)          \
    {                            \
        if (pmix_test_verbose) { \
            TEST_OUTPUT(x);      \
        }                        \
    }

#define TEST_DEFAULT_TIMEOUT 10
#define MAX_DIGIT_LEN        10
#define TEST_REPLACE_DEFAULT "3:1"

#define TEST_SET_FILE(prefix, ns_id, rank)                              \
    {                                                                   \
        char *fname = malloc(strlen(prefix) + MAX_DIGIT_LEN + 2);       \
        sprintf(fname, "%s.%d.%d", prefix, ns_id, rank);                \
        file = fopen(fname, "w");                                       \
        free(fname);                                                    \
        if (NULL == file) {                                             \
            fprintf(stderr, "Cannot open file %s for writing!", fname); \
            exit(1);                                                    \
        }                                                               \
    }

#define TEST_CLOSE_FILE()     \
    {                         \
        if (stderr != file) { \
            fclose(file);     \
        }                     \
    }

typedef struct {
    char *binary;
    char *np;
    char *prefix;
    char *nspace;
    uint32_t nprocs;
    int timeout;
    int verbose;
    pmix_rank_t rank;
    int early_fail;
    int test_job_fence;
    int collect_bad;
    int use_same_keys;
    int collect;
    int nonblocking;
    char *fences;
    char *noise;
    char *ns_dist;
    int ns_size;
    int ns_id;
    pmix_rank_t base_rank;
    int test_publish;
    int test_spawn;
    int test_connect;
    int test_resolve_peers;
    int test_error;
    char *key_replace;
    int test_internal;
    char *gds_mode;
    int nservers;
    uint32_t lsize;
} test_params;

#define INIT_TEST_PARAMS(params)               \
    do {                                       \
        params.nprocs = 1;                     \
        params.verbose = 0;                    \
        params.rank = PMIX_RANK_UNDEF;         \
        params.base_rank = 0;                  \
        params.early_fail = 0;                 \
        params.ns_size = -1;                   \
        params.ns_id = -1;                     \
        params.timeout = TEST_DEFAULT_TIMEOUT; \
        params.test_job_fence = 0;             \
        params.use_same_keys = 0;              \
        params.collect = 0;                    \
        params.collect_bad = 0;                \
        params.nonblocking = 0;                \
        params.test_publish = 0;               \
        params.test_spawn = 0;                 \
        params.test_connect = 0;               \
        params.test_resolve_peers = 0;         \
        params.binary = NULL;                  \
        params.np = NULL;                      \
        params.prefix = NULL;                  \
        params.nspace = NULL;                  \
        params.fences = NULL;                  \
        params.noise = NULL;                   \
        params.ns_dist = NULL;                 \
        params.test_error = 0;                 \
        params.key_replace = NULL;             \
        params.test_internal = 0;              \
        params.gds_mode = NULL;                \
        params.nservers = 1;                   \
        params.lsize = 0;                      \
    } while (0)

#define FREE_TEST_PARAMS(params)      \
    do {                              \
        if (NULL != params.binary) {  \
            free(params.binary);      \
        }                             \
        if (NULL != params.np) {      \
            free(params.np);          \
        }                             \
        if (NULL != params.prefix) {  \
            free(params.prefix);      \
        }                             \
        if (NULL != params.nspace) {  \
            free(params.nspace);      \
        }                             \
        if (NULL != params.fences) {  \
            free(params.fences);      \
        }                             \
        if (NULL != params.noise) {   \
            free(params.noise);       \
        }                             \
        if (NULL != params.ns_dist) { \
            free(params.ns_dist);     \
        }                             \
    } while (0)

void parse_cmd(int argc, char **argv, test_params *params);
int parse_fence(char *fence_param, int store);
int parse_noise(char *noise_param, int store);
int parse_replace(char *replace_param, int store, int *key_num);

typedef struct {
    pmix_list_item_t super;
    int blocking;
    int data_exchange;
    pmix_list_t *participants; // list of participants
} fence_desc_t;
PMIX_CLASS_DECLARATION(fence_desc_t);

typedef struct {
    pmix_list_item_t super;
    pmix_proc_t proc;
} participant_t;
PMIX_CLASS_DECLARATION(participant_t);

typedef struct {
    pmix_list_item_t super;
    int key_idx;
} key_replace_t;
PMIX_CLASS_DECLARATION(key_replace_t);

extern pmix_list_t test_fences;
extern pmix_list_t *noise_range;
extern pmix_list_t key_replace;

int get_total_ns_number(test_params params);
int get_all_ranks_from_namespace(test_params params, char *nspace, pmix_proc_t **ranks,
                                 size_t *nranks);

typedef struct {
    int in_progress;
    pmix_value_t *kv;
    int status;
} get_cbdata;

#define SET_KEY(key, fence_num, ind, use_same_keys)                              \
    do {                                                                         \
        if (use_same_keys) {                                                     \
            (void) snprintf(key, sizeof(key) - 1, "key-%d", ind);                \
        } else {                                                                 \
            (void) snprintf(key, sizeof(key) - 1, "key-f%d:%d", fence_num, ind); \
        }                                                                        \
    } while (0)

#define PUT(dtype, data, flag, fence_num, ind, use_same_keys)                          \
    do {                                                                               \
        char _key[50];                                                                 \
        pmix_value_t _value;                                                           \
        SET_KEY(_key, fence_num, ind, use_same_keys);                                  \
        PMIX_VAL_SET(&_value, dtype, data);                                            \
        TEST_VERBOSE(("%s:%d put key %s", my_nspace, my_rank, _key));                  \
        if (PMIX_SUCCESS != (rc = PMIx_Put(flag, _key, &_value))) {                    \
            TEST_ERROR(("%s:%d: PMIx_Put key %s failed: %s", my_nspace, my_rank, _key, \
                        PMIx_Error_string(rc)));                                       \
        }                                                                              \
        PMIX_VALUE_DESTRUCT(&_value);                                                  \
    } while (0)

#define GET(dtype, data, ns, r, fence_num, ind, use_same_keys, blocking, ok_notfnd)               \
    do {                                                                                          \
        char _key[50];                                                                            \
        pmix_value_t *_val;                                                                       \
        get_cbdata _cbdata;                                                                       \
        _cbdata.status = PMIX_SUCCESS;                                                            \
        pmix_proc_t _foobar;                                                                      \
        SET_KEY(_key, fence_num, ind, use_same_keys);                                             \
        PMIX_LOAD_PROCID(&_foobar, ns, r);                                                        \
        TEST_VERBOSE(("%s:%d want to get from %s:%d key %s", my_nspace, my_rank, ns, r, _key));   \
        if (blocking) {                                                                           \
            if (PMIX_SUCCESS != (rc = PMIx_Get(&_foobar, _key, NULL, 0, &_val))) {                \
                if (!((rc == PMIX_ERR_NOT_FOUND || rc == PMIX_ERR_NOT_FOUND)           \
                      && ok_notfnd)) {                                                            \
                    TEST_ERROR(("%s:%d: PMIx_Get failed: %s from %s:%d, key %s", my_nspace,       \
                                my_rank, PMIx_Error_string(rc), ns, r, _key));                    \
                }                                                                                 \
            }                                                                                     \
        } else {                                                                                  \
            int _count;                                                                           \
            _cbdata.in_progress = 1;                                                              \
            PMIX_VALUE_CREATE(_val, 1);                                                           \
            _cbdata.kv = _val;                                                                    \
            if (PMIX_SUCCESS                                                                      \
                != (rc = PMIx_Get_nb(&_foobar, _key, NULL, 0, get_cb, (void *) &_cbdata))) {      \
                TEST_VERBOSE(("%s:%d: PMIx_Get_nb failed: %s from %s:%d, key=%s", my_nspace,      \
                              my_rank, PMIx_Error_string(rc), ns, r, _key));                      \
            } else {                                                                              \
                _count = 0;                                                                       \
                while (_cbdata.in_progress) {                                                     \
                    struct timespec ts;                                                           \
                    ts.tv_sec = 0;                                                                \
                    ts.tv_nsec = 100;                                                             \
                    nanosleep(&ts, NULL);                                                         \
                    _count++;                                                                     \
                }                                                                                 \
                rc = _cbdata.status;                                                              \
                PMIX_ACQUIRE_OBJECT(&_cbdata);                                                    \
            }                                                                                     \
        }                                                                                         \
        if (PMIX_SUCCESS == rc) {                                                                 \
            if (PMIX_SUCCESS != _cbdata.status) {                                                 \
                if (!((_cbdata.status == PMIX_ERR_NOT_FOUND)                                      \
                      && ok_notfnd)) {                                                            \
                    TEST_ERROR(("%s:%d: PMIx_Get_nb failed: %s from %s:%d, key=%s", my_nspace,    \
                                my_rank, PMIx_Error_string(rc), my_nspace, r, _key));             \
                }                                                                                 \
            } else if (NULL == _val) {                                                            \
                TEST_VERBOSE(("%s:%d: PMIx_Get returned NULL value", my_nspace, my_rank));        \
            } else if (_val->type != PMIX_VAL_TYPE_##dtype                                        \
                       || PMIX_VAL_CMP(dtype, PMIX_VAL_FIELD_##dtype((_val)), data)) {            \
                TEST_ERROR(("%s:%u: from %s:%d Key %s value or type mismatch,"                    \
                            " want type %d get type %d",                                          \
                            my_nspace, my_rank, ns, r, _key, PMIX_VAL_TYPE_##dtype, _val->type)); \
            }                                                                                     \
        }                                                                                         \
        if (PMIX_SUCCESS == rc) {                                                                 \
            TEST_VERBOSE(                                                                         \
                ("%s:%d: GET OF %s from %s:%d SUCCEEDED", my_nspace, my_rank, _key, ns, r));      \
            PMIX_VALUE_RELEASE(_val);                                                             \
        }                                                                                         \
    } while (0)

#define FENCE(blocking, data_ex, pcs, nprocs)                                                  \
    do {                                                                                       \
        if (blocking) {                                                                        \
            pmix_info_t *_info = NULL;                                                         \
            size_t _ninfo = 0;                                                                 \
            if (data_ex) {                                                                     \
                bool _value = 1;                                                               \
                PMIX_INFO_CREATE(_info, 1);                                                    \
                pmix_strncpy(_info->key, PMIX_COLLECT_DATA, PMIX_MAX_KEYLEN);                \
                PMIx_Value_load(&_info->value, &_value, PMIX_BOOL);                            \
                _ninfo = 1;                                                                    \
            }                                                                                  \
            rc = PMIx_Fence(pcs, nprocs, _info, _ninfo);                                       \
            PMIX_INFO_FREE(_info, _ninfo);                                                     \
        } else {                                                                               \
            int in_progress = 1, _count;                                                       \
            rc = PMIx_Fence_nb(pcs, nprocs, NULL, 0, release_cb, &in_progress);                \
            if (PMIX_SUCCESS == rc) {                                                          \
                _count = 0;                                                                    \
                while (in_progress) {                                                          \
                    struct timespec ts;                                                        \
                    ts.tv_sec = 0;                                                             \
                    ts.tv_nsec = 100;                                                          \
                    nanosleep(&ts, NULL);                                                      \
                    _count++;                                                                  \
                }                                                                              \
                TEST_VERBOSE(                                                                  \
                    ("PMIx_Fence_nb(barrier,collect): free time: %lfs", _count * 100 * 1E-9)); \
            }                                                                                  \
        }                                                                                      \
        if (PMIX_SUCCESS == rc) {                                                              \
            TEST_VERBOSE(("%s:%d: Fence successfully completed", my_nspace, my_rank));         \
        }                                                                                      \
    } while (0)

/* Key-Value pair management macros */
// TODO: add all possible types/fields here.

#define PMIX_VAL_FIELD_int(x)      ((x)->data.integer)
#define PMIX_VAL_FIELD_uint32_t(x) ((x)->data.uint32)
#define PMIX_VAL_FIELD_uint16_t(x) ((x)->data.uint16)
#define PMIX_VAL_FIELD_string(x)   ((x)->data.string)
#define PMIX_VAL_FIELD_float(x)    ((x)->data.fval)
#define PMIX_VAL_FIELD_byte(x)     ((x)->data.byte)
#define PMIX_VAL_FIELD_flag(x)     ((x)->data.flag)

#define PMIX_VAL_TYPE_int      PMIX_INT
#define PMIX_VAL_TYPE_uint32_t PMIX_UINT32
#define PMIX_VAL_TYPE_uint16_t PMIX_UINT16
#define PMIX_VAL_TYPE_string   PMIX_STRING
#define PMIX_VAL_TYPE_float    PMIX_FLOAT
#define PMIX_VAL_TYPE_byte     PMIX_BYTE
#define PMIX_VAL_TYPE_flag     PMIX_BOOL

#define PMIX_VAL_set_assign(_v, _field, _val) \
    do {                                      \
        (_v)->type = PMIX_VAL_TYPE_##_field;  \
        PMIX_VAL_FIELD_##_field((_v)) = _val; \
    } while (0)

#define PMIX_VAL_set_strdup(_v, _field, _val)         \
    do {                                              \
        (_v)->type = PMIX_VAL_TYPE_##_field;          \
        PMIX_VAL_FIELD_##_field((_v)) = strdup(_val); \
    } while (0)

#define PMIX_VAL_SET_int      PMIX_VAL_set_assign
#define PMIX_VAL_SET_uint32_t PMIX_VAL_set_assign
#define PMIX_VAL_SET_uint16_t PMIX_VAL_set_assign
#define PMIX_VAL_SET_string   PMIX_VAL_set_strdup
#define PMIX_VAL_SET_float    PMIX_VAL_set_assign
#define PMIX_VAL_SET_byte     PMIX_VAL_set_assign
#define PMIX_VAL_SET_flag     PMIX_VAL_set_assign

#define PMIX_VAL_SET(_v, _field, _val) PMIX_VAL_SET_##_field(_v, _field, _val)

#define PMIX_VAL_cmp_val(_val1, _val2) ((_val1) != (_val2))
#define PMIX_VAL_cmp_float(_val1, _val2) \
    (((_val1) > (_val2)) ? (((_val1) - (_val2)) > 0.000001) : (((_val2) - (_val1)) > 0.000001))
#define PMIX_VAL_cmp_ptr(_val1, _val2) strncmp(_val1, _val2, strlen(_val1) + 1)

#define PMIX_VAL_CMP_int      PMIX_VAL_cmp_val
#define PMIX_VAL_CMP_uint32_t PMIX_VAL_cmp_val
#define PMIX_VAL_CMP_uint16_t PMIX_VAL_cmp_val
#define PMIX_VAL_CMP_float    PMIX_VAL_cmp_float
#define PMIX_VAL_CMP_string   PMIX_VAL_cmp_ptr
#define PMIX_VAL_CMP_byte     PMIX_VAL_cmp_val
#define PMIX_VAL_CMP_flag     PMIX_VAL_cmp_val

#define PMIX_VAL_ASSIGN(_v, _field, _val) PMIX_VAL_set_assign(_v, _field, _val)

#define PMIX_VAL_CMP(_field, _val1, _val2) PMIX_VAL_CMP_##_field(_val1, _val2)

#define PMIX_VAL_FREE(_v) PMIx_free_value_data(_v)

#endif // TEST_COMMON_H
