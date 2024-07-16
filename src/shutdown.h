// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2018 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file LICENSE or http://www.opensource.org/licenses/mit-license.php.

#ifndef DEFI_SHUTDOWN_H
#define DEFI_SHUTDOWN_H

#include <condition_variable>
#include <mutex>

void StartShutdown();
void AbortShutdown();
bool ShutdownRequested();

extern std::condition_variable shutdown_cv;
extern std::mutex shutdown_mutex;

#endif
