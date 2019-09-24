// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include <limits.h>
#include <openenclave/host.h>
#include <openenclave/internal/error.h>
#include <openenclave/internal/tests.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <atomic>
#include <chrono>
#include "../../../host/hostthread.h"
#include "../../../host/strings.h"
#include "switchless_u.h"

// Increase this number to have a meaningful performance measurement
#define NUM_OCALLS (100000)

#define STRING_LEN 100

static double get_relative_time_in_microseconds()
{
    auto duration = std::chrono::system_clock::now().time_since_epoch();
    auto msecs =
        std::chrono::duration_cast<std::chrono::microseconds>(duration).count();
    return msecs;
}

// Total number of switchless calls made.
std::atomic<uint64_t> total_switchless_calls(0);

int host_echo_switchless(
    const char* in,
    char* out,
    const char* str1,
    char str2[STRING_LEN])
{
    OE_TEST(strcmp(str1, "host string parameter") == 0);
    OE_TEST(strcmp(str2, "host string on stack") == 0);

    strcpy(out, in);

    total_switchless_calls += 1;

    return 0;
}

int host_echo_regular(
    const char* in,
    char* out,
    const char* str1,
    char str2[STRING_LEN])
{
    OE_TEST(strcmp(str1, "host string parameter") == 0);
    OE_TEST(strcmp(str2, "host string on stack") == 0);

    strcpy(out, in);
    return 0;
}

double make_repeated_switchless_ocalls(oe_enclave_t* enclave)
{
    char out[STRING_LEN];
    int return_val;
    double start, end;

    double switchless_microseconds = 0.0;

    start = get_relative_time_in_microseconds();
    OE_TEST(
        enc_echo_switchless(
            enclave, &return_val, "Hello World", out, NUM_OCALLS) == OE_OK);
    OE_TEST(return_val == 0);
    end = get_relative_time_in_microseconds();
    switchless_microseconds += end - start;

    printf(
        "%d switchless calls took %d msecs.\n",
        NUM_OCALLS,
        (int)(switchless_microseconds / 1000.0));
    return switchless_microseconds;
}

void* launch_enclave_thread(void* e)
{
    make_repeated_switchless_ocalls((oe_enclave_t*)e);
    return NULL;
}

int main(int argc, const char* argv[])
{
    oe_enclave_t* enclave = NULL;
    oe_result_t result;

    if (argc < 2)
    {
        fprintf(
            stderr,
            "Usage: %s ENCLAVE_PATH [num-host-threads] [num-enclave-threads]\n",
            argv[0]);
        return 1;
    }

    uint64_t num_host_threads = 2;
    uint64_t num_enclave_threads = 2;

    if (argc >= 3)
    {
        sscanf(argv[2], "%lu", &num_host_threads);
    }

    if (argc == 4)
    {
        sscanf(argv[3], "%lu", &num_enclave_threads);
    }

    const uint32_t flags = oe_get_create_flags();

#ifdef OE_CONTEXT_SWITCHLESS_EXPERIMENTAL_FEATURE
    // Enable switchless and configure host worker number
    oe_enclave_config_context_switchless_t config = {num_host_threads, 0};
    oe_enclave_config_t cfg;
    cfg.config_type = OE_ENCLAVE_CONFIG_CONTEXT_SWITCHLESS;
    cfg.u.context_switchless_config = &config;
    oe_enclave_config_t configs[] = {cfg};

    if ((result = oe_create_switchless_enclave(
             argv[1],
             OE_ENCLAVE_TYPE_SGX,
             flags,
             configs,
             OE_COUNTOF(configs),
             &enclave)) != OE_OK)
#else
    if ((result = oe_create_switchless_enclave(
             argv[1], OE_ENCLAVE_TYPE_SGX, flags, NULL, 0, &enclave)) != OE_OK)
#endif
        oe_put_err("oe_create_enclave(): result=%u", result);

    char out[STRING_LEN];
    int return_val = 0;

    double switchless_microseconds = 0;
    double regular_microseconds = 0;
    double start, end;

    // Increase this number to have a meaningful performance measurement
    int repeats = 100000;

    uint64_t num_extra_enc_threads = num_enclave_threads - 1;
    oe_thread_t tid[32] = {0};
    for (uint64_t i = 0; i < num_extra_enc_threads; ++i)
    {
        oe_thread_create(&tid[i], launch_enclave_thread, enclave);
        if (tid[i])
        {
            printf("Launched enclave producer thread %ld\n", i);
        }
    }

    printf("Using main enclave thread\n");
    switchless_microseconds += make_repeated_switchless_ocalls(enclave);

    printf("Making regular ocalls\n");
    start = get_relative_time_in_microseconds();
    OE_TEST(
        enc_echo_regular(enclave, &return_val, "Hello World", out, repeats) ==
        OE_OK);
    OE_TEST(return_val == 0);
    end = get_relative_time_in_microseconds();
    regular_microseconds += end - start;

    for (uint64_t i = 0; i < num_extra_enc_threads; ++i)
    {
        if (tid[i])
            oe_thread_join(tid[i]);
    }

    result = oe_terminate_enclave(enclave);
    OE_TEST(result == OE_OK);

    printf("total switchless calls %lu\n", total_switchless_calls.load());
    OE_TEST(total_switchless_calls == NUM_OCALLS * num_enclave_threads);

    printf(
        "Time spent in repeating OCALL %d times: switchless %d vs "
        "regular %d ms, speed up: %.2f\n",
        repeats,
        (int)switchless_microseconds / 1000,
        (int)regular_microseconds / 1000,
        (double)regular_microseconds / switchless_microseconds);
    printf("=== passed all tests (switchless)\n");

    return 0;
}
