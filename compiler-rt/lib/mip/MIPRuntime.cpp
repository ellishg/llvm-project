/*===- MIPRuntime.c - Machine IR Profile Runtime Helper -------------------===*\
|*
|* Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
|* See https://llvm.org/LICENSE.txt for license information.
|* SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
|*
\*===----------------------------------------------------------------------===*/

extern "C" {
#include "MIPHelper.h"
int MIP_RUNTIME_SYMBOL;
}

namespace {

class RegisterMIPRuntime {
public:
  RegisterMIPRuntime() { __llvm_mip_runtime_initialize(); }
};

RegisterMIPRuntime Registration;

} // namespace
