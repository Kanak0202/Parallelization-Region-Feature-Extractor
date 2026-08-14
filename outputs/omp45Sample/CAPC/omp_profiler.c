#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <time.h>
#include <dlfcn.h>
#include <unistd.h>
#include <string.h>

typedef uint64_t ompt_id_t;

typedef struct ompt_data_s {
    uint64_t value;
    void *ptr;
} ompt_data_t;

typedef enum ompt_target_e {
    ompt_target = 1,
    ompt_target_enter_data = 2,
    ompt_target_exit_data = 3,
    ompt_target_update = 4
} ompt_target_t;

typedef enum ompt_scope_endpoint_e {
    ompt_scope_begin = 1,
    ompt_scope_end = 2,
    ompt_scope_beginend = 3
} ompt_scope_endpoint_t;

typedef enum ompt_target_data_op_e {
    ompt_target_data_alloc = 1,
    ompt_target_data_transfer_to_device = 2,
    ompt_target_data_transfer_from_device = 3,
    ompt_target_data_delete = 4
} ompt_target_data_op_t;

typedef enum ompt_callbacks_e {
    ompt_callback_target = 50,
    ompt_callback_target_data_op = 51,
    ompt_callback_target_submit = 52
} ompt_callbacks_t;

typedef void (*ompt_callback_t)(void);
typedef int (*ompt_set_callback_t)(ompt_callbacks_t event, ompt_callback_t callback);
typedef void *(*ompt_function_lookup_t)(const char *entrypoint);

typedef struct ompt_start_tool_result_s {
    int (*initialize)(ompt_function_lookup_t lookup, int initial_device_num, ompt_data_t *tool_data);
    void (*finalize)(ompt_data_t *tool_data);
    ompt_data_t tool_data;
} ompt_start_tool_result_t;

static double get_time_sec(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec + (double)ts.tv_nsec * 1e-9;
}

#define MAX_TARGETS 1024
typedef struct {
    ompt_id_t target_id;
    double start_time;
    int line_no;
    bool active;
} target_record_t;

static target_record_t target_records[MAX_TARGETS];

static int get_line_from_address(const void *codeptr_ra) {
    if (!codeptr_ra) return 0;

    Dl_info info;
    if (dladdr(codeptr_ra, &info) && info.dli_fname) {
        char cmd[512];
        uintptr_t addr = (uintptr_t)codeptr_ra;
        snprintf(cmd, sizeof(cmd), "addr2line -e %s %p 2>/dev/null", info.dli_fname, (void*)addr);
        FILE *fp = popen(cmd, "r");
        if (fp) {
            char buf[256];
            if (fgets(buf, sizeof(buf), fp)) {
                char *colon = strrchr(buf, ':');
                if (colon) {
                    int line = atoi(colon + 1);
                    pclose(fp);
                    if (line > 0) return line;
                }
            }
            pclose(fp);
        }
    }
    return 0;
}

static void on_ompt_callback_target(
    ompt_target_t kind,
    ompt_scope_endpoint_t endpoint,
    int device_num,
    ompt_data_t *task_data,
    ompt_id_t target_id,
    const void *codeptr_ra
) {
    double now = get_time_sec();
    if (endpoint == ompt_scope_begin) {
        int line = get_line_from_address(codeptr_ra);
        for (int i = 0; i < MAX_TARGETS; i++) {
            if (!target_records[i].active) {
                target_records[i].target_id = target_id;
                target_records[i].start_time = now;
                target_records[i].line_no = line;
                target_records[i].active = true;
                break;
            }
        }
    } else if (endpoint == ompt_scope_end) {
        for (int i = 0; i < MAX_TARGETS; i++) {
            if (target_records[i].active && target_records[i].target_id == target_id) {
                double duration = now - target_records[i].start_time;
                int line = target_records[i].line_no;
                target_records[i].active = false;
                fprintf(stderr, "[PROFILER] line:%d | Target Execution Time = %.6f s\n", line, duration);
                fflush(stderr);
                break;
            }
        }
    }
}

static void on_ompt_callback_target_data_op(
    ompt_id_t target_id,
    ompt_id_t host_op_id,
    ompt_target_data_op_t optype,
    void *src_addr,
    int src_device_num,
    void *dest_addr,
    int dest_device_num,
    size_t bytes,
    const void *codeptr_ra
) {
    /* Optional: Data Transfer Logging */
}

int ompt_initialize(ompt_function_lookup_t lookup, int initial_device_num, ompt_data_t *tool_data) {
    ompt_set_callback_t ompt_set_callback = (ompt_set_callback_t) lookup("ompt_set_callback");

    if (ompt_set_callback) {
        ompt_set_callback(ompt_callback_target, (ompt_callback_t)on_ompt_callback_target);
        ompt_set_callback(ompt_callback_target_data_op, (ompt_callback_t)on_ompt_callback_target_data_op);
    }
    return 1;
}

void ompt_finalize(ompt_data_t *tool_data) {
}

#ifdef __cplusplus
extern "C" {
#endif
ompt_start_tool_result_t *ompt_start_tool(unsigned int omp_version, const char *runtime_version) {
    static ompt_start_tool_result_t result = { &ompt_initialize, &ompt_finalize, {0} };
    return &result;
}
#ifdef __cplusplus
}
#endif
