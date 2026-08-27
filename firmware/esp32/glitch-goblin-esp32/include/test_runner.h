#ifndef TEST_RUNNER_H
#define TEST_RUNNER_H

#include "fault_engine.h"

void TestRunner_RunFault(
    FaultMode fault,
    bool expect_ack
);

void TestRunner_RunStats(void);

void TestRunner_RunReliablePing(void);

void TestRunner_RunBenchmark(void);

void TestRunner_RunChaos(void);
void TestRunner_RunReplay(void);

#endif