//
// Copyright (c) 2014, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the University of Illinois/NCSA Open
// Source License found in the LICENSE file in the root directory of this
// source tree. An additional grant of patent rights can be found in the
// PATENTS file in the same directory.
//

#define __DS2_LOG_CLASS_NAME__ "Target::Thread"

#include "DebugServer2/Host/Platform.h"
#include "DebugServer2/Target/Process.h"
#include "DebugServer2/Target/Windows/Thread.h"
#include "DebugServer2/Utils/Log.h"

#define super ds2::Target::ThreadBase

namespace ds2 {
namespace Target {
namespace Windows {

Thread::Thread(Process *process, ThreadId tid, HANDLE handle)
    : super(process, tid), _handle(handle) {
  //
  // Initially the thread is stopped.
  //
  _state = kStopped;
}

Thread::~Thread() { CloseHandle(_handle); }

ErrorCode Thread::resume(int signal, Address const &address) {
  // TODO(sas): Not sure how to translate the signal concept to Windows yet.
  // We'll probably have to get rid of these at some point.
  DS2ASSERT(signal == 0);
  // TODO(sas): Continuing a thread from a given address is not implemented yet.
  DS2ASSERT(!address.valid());

  ErrorCode error = kSuccess;

  if (_state == kStopped || _state == kStepped) {
    ProcessInfo info;

    error = process()->getInfo(info);
    if (error != kSuccess)
      return error;

    BOOL result = ContinueDebugEvent(_process->pid(), _tid, DBG_CONTINUE);
    if (!result)
      return Host::Platform::TranslateError();

    _state = kRunning;
  } else if (_state == kTerminated) {
    error = kErrorProcessNotFound;
  }

  return error;
}

void Thread::updateState() {}

void Thread::updateState(DEBUG_EVENT const &de) {
  DS2ASSERT(de.dwThreadId == _tid);

  switch (de.dwDebugEventCode) {
  case EXCEPTION_DEBUG_EVENT:
    _state = kStopped;
    _trap.event = TrapInfo::kEventTrap;
    _trap.reason = TrapInfo::kReasonNone;
    break;

  case LOAD_DLL_DEBUG_EVENT:
    if (de.u.LoadDll.hFile != NULL)
      CloseHandle(de.u.LoadDll.hFile);

  case UNLOAD_DLL_DEBUG_EVENT:
  case OUTPUT_DEBUG_STRING_EVENT:
    _state = kStopped;
    _trap.event = TrapInfo::kEventNone;
    _trap.reason = TrapInfo::kReasonNone;
    break;

  default:
    DS2BUG("unknown debug event code: %d", de.dwDebugEventCode);
  }
}
}
}
}
