#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/resource.h>
#include <time.h>

/**
 * Get the total number of page faults for the current process
 * 
 * @return The total number of page faults
 */
long get_page_faults(){
    struct rusage usage;
    getrusage(RUSAGE_SELF, &usage);
    // There are two types of page faults:
    // 1. Major page faults
    // 2. Minor page faults
    // In this example we are concerned about the total number of page faults, so we add the two together
    return usage.ru_majflt + usage.ru_minflt;
}

/**
 * Get the CPU time for the current process
 * 
 * @return The CPU time in nanoseconds
 */
u_int64_t get_cpu_time(){
    struct timespec ts;
    clock_gettime(CLOCK_PROCESS_CPUTIME_ID,&ts);
    return ts.tv_sec * 1000000000LL + ts.tv_nsec;
}

/**
 * Helper function to convert nanoseconds to milliseconds
 * 
 * @param ns The number of nanoseconds
 * @return The number of milliseconds
 */
double nanoseconds_to_milliseconds(u_int64_t ns) {
    return ns / 1000000.0;
}

/**
 * Test the performance of a fresh allocation
 * 
 * @param buffer_size The size of the buffer to allocate
 */
u_int64_t test_fresh_allocation(size_t buffer_size){
    printf("\n=== Fresh allocation test (%zu KB) ===\n", buffer_size / 1024);

    long pf_start = get_page_faults();
    u_int64_t cpu_start = get_cpu_time();

    // Step 1: Allocate fresh buffer 
    char *buffer = (char *)malloc(buffer_size);
    if (!buffer){
        printf("Allocation failed\n");
        return 0;
    }

    // Step 2: Write to every byte (this should trigger page faults)
    for (size_t i=0; i<buffer_size; i++){
        buffer[i] = (char)(i & 0xFF);
    }

    long pf_write = get_page_faults();
    u_int64_t cpu_write = get_cpu_time();

    // Step 3: Read from every byte
    volatile int sum = 0;
    for (size_t i = 0; i < buffer_size; i++){
        sum+=buffer[i];
    }

    long pf_end = get_page_faults();
    u_int64_t cpu_end = get_cpu_time();

    long write_faults = pf_write - pf_start;
    long read_faults = pf_end - pf_write;
    
    u_int64_t write_time = cpu_write - cpu_start;
    u_int64_t read_time = cpu_end - cpu_write;
    u_int64_t total_time = cpu_end - cpu_start;

    printf("Write: %ld page faults, %.2f KB/fault, %.3f ms\n", 
        write_faults, 
        write_faults > 0 ? (double)buffer_size / (write_faults * 1024) : 0,
        nanoseconds_to_milliseconds(write_time));
    printf("Read:  %ld page faults, %.3f ms\n", 
        read_faults, nanoseconds_to_milliseconds(read_time));
    printf("Total time: %.3f ms\n", nanoseconds_to_milliseconds(total_time));
    
    free(buffer);
    return total_time;
}

/**
 * Test the performance of a reused buffer
 * 
 * @param buffer_size The size of the buffer to allocate
 */
u_int64_t test_reused_buffer(size_t buffer_size){
    printf("\n=== Reused buffer test (%zu KB) ===\n", buffer_size / 1024);

    // Step 1: Pre allocate and touch the buffer to map all pages 
    char *buffer = (char *)malloc(buffer_size);
    if (!buffer){
        printf("Allocation failed");
        return 0;
    }

    // Step 2: Touch all pages to map them 
    for (size_t i = 0; i < buffer_size; i++){
        buffer[i] = (char)(i & 0xFF);
    }

    long pf_start = get_page_faults();
    u_int64_t cpu_start = get_cpu_time();

    // Step 3: Write to every byte (pages have already been mapped)
    for (size_t i = 0; i<buffer_size; i++){
        buffer[i] = (char)((i+1) & 0xFF);
    }
    long pf_write = get_page_faults();
    u_int64_t cpu_write = get_cpu_time();

    // Step 4: Read from every byte
    volatile int sum = 0;
    for (size_t i = 0; i < buffer_size; i++){
        sum += buffer[i];
    }

    long pf_end = get_page_faults();
    u_int64_t cpu_end = get_cpu_time();

    long write_faults = pf_write - pf_start;
    long read_faults = pf_end - pf_write;
    
    u_int64_t write_time = cpu_write - cpu_start;
    u_int64_t read_time = cpu_end - cpu_write;
    u_int64_t total_time = cpu_end - cpu_start;

    printf("Write: %ld page faults, %.2f KB/fault, %.3f ms\n", 
        write_faults, 
        write_faults > 0 ? (double)buffer_size / (write_faults * 1024) : 0,
        nanoseconds_to_milliseconds(write_time));
    printf("Read:  %ld page faults, %.3f ms\n", 
        read_faults, nanoseconds_to_milliseconds(read_time));
    printf("Total time: %.3f ms\n", nanoseconds_to_milliseconds(total_time));
    
    free(buffer);
    return total_time;
}

void run_test_suite(const char* run_name, size_t test_sizes[], int num_tests){
    printf("\n==================== %s ====================\n", run_name);
    for (int i = 0; i < num_tests; i++) {
        u_int64_t fresh_time_ns = test_fresh_allocation(test_sizes[i]);
        u_int64_t reused_time_ns = test_reused_buffer(test_sizes[i]);
        
        double fresh_time_ms = nanoseconds_to_milliseconds(fresh_time_ns);
        double reused_time_ms = nanoseconds_to_milliseconds(reused_time_ns);

        double time_saved = fresh_time_ms - reused_time_ms;
        double speedup = reused_time_ms > 0 ? fresh_time_ms / reused_time_ms : 0;

        printf("\n--- Summary for %zu KB ---\n", test_sizes[i] / 1024);
        printf("Time saved: %.3f ms (%.1fx speedup)\n", time_saved, speedup);

        printf("\n");
    }
}

int main(){
    // NOTE for Reader: when i call malloc or new the OS will allocate (required bytes/page size) number of pages for my request
    size_t page_size = sysconf(_SC_PAGESIZE);
    printf("System page size %ld bytes (%ld KB)\n", page_size, page_size/1024);

    size_t test_size[] = {
        page_size,      // 1 page
        page_size * 4,  // 4 pages
        page_size * 64, // 64 pages
        1024 * 1024     // 256 pages 
    };

    int num_tests = sizeof(test_size) / sizeof(test_size[0]);

    run_test_suite("First Run", test_size, num_tests);
    run_test_suite("Second Run", test_size, num_tests);

    return 0;
}
