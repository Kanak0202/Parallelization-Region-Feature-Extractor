// acc_profiler.c
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <time.h>
#include <openacc.h>
#include <acc_prof.h>

static double launch_start_time = 0.0;
static double upload_start_time = 0.0;
static double download_start_time = 0.0;

static double get_time_sec(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec + (double)ts.tv_nsec * 1e-9;
}

// Callback for GPU Kernel Launches
void cb_enqueue_launch(acc_prof_info *prof_info, acc_event_info *event_info, acc_api_info *api_info) {
    (void)event_info;
    (void)api_info;

    if (prof_info->event_type == acc_ev_enqueue_launch_start) {
        launch_start_time = get_time_sec();
    } 
    else if (prof_info->event_type == acc_ev_enqueue_launch_end) {
        acc_wait_all(); // Synchronize GPU so CPU waits for compute to finish
        double duration = get_time_sec() - launch_start_time;
        int line = prof_info->line_no;
        const char *file = prof_info->src_file ? prof_info->src_file : "unknown";
        printf("[PROFILER] %s:%d | Kernel Execution Time = %.6f s\n", file, line, duration);
    }
}

// Callback for Data Transfers (H2D / D2H)
void cb_data_transfer(acc_prof_info *prof_info, acc_event_info *event_info, acc_api_info *api_info) {
    (void)api_info;

    int line = prof_info->line_no;
    const char *file = prof_info->src_file ? prof_info->src_file : "unknown";
    size_t bytes = event_info ? event_info->data_event.bytes : 0;
    const char *var_name = (event_info && event_info->data_event.var_name) ? event_info->data_event.var_name : "unnamed";

    if (prof_info->event_type == acc_ev_enqueue_upload_start) {
        upload_start_time = get_time_sec();
    }
    else if (prof_info->event_type == acc_ev_enqueue_upload_end) {
        acc_wait_all(); // Synchronize GPU transfer
        double duration = get_time_sec() - upload_start_time;
        printf("[PROFILER] %s:%d | H2D Transfer ('%s') = %.6f s (%lu bytes)\n",
               file, line, var_name, duration, (unsigned long)bytes);
    }
    else if (prof_info->event_type == acc_ev_enqueue_download_start) {
        download_start_time = get_time_sec();
    }
    else if (prof_info->event_type == acc_ev_enqueue_download_end) {
        acc_wait_all(); // Synchronize GPU transfer
        double duration = get_time_sec() - download_start_time;
        printf("[PROFILER] %s:%d | D2H Transfer ('%s') = %.6f s (%lu bytes)\n",
               file, line, var_name, duration, (unsigned long)bytes);
    }
}

// Entry Point
void acc_register_library(acc_prof_reg reg, acc_prof_reg unreg, void (* (*lookup)(const char *))()) {
    (void)unreg;
    (void)lookup;

    reg(acc_ev_enqueue_launch_start, cb_enqueue_launch, acc_reg);
    reg(acc_ev_enqueue_launch_end, cb_enqueue_launch, acc_reg);

    reg(acc_ev_enqueue_upload_start, cb_data_transfer, acc_reg);
    reg(acc_ev_enqueue_upload_end, cb_data_transfer, acc_reg);
    reg(acc_ev_enqueue_download_start, cb_data_transfer, acc_reg);
    reg(acc_ev_enqueue_download_end, cb_data_transfer, acc_reg);
}