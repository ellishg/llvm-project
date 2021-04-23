add_compiler_rt_component(mip)

set(MIP_SOURCES
  MIPHelper.c
  MIPRuntime.cpp
)

set(MIP_HEADERS
  MIPHelper.h
)

include_directories(../../include)

if(APPLE)
  add_compiler_rt_runtime(clang_rt.mip
    STATIC
    OS ${MIP_SUPPORTED_OS}
    ARCHS ${MIP_SUPPORTED_ARCH}
    CFLAGS ${EXTRA_FLAGS}
    SOURCES ${MIP_SOURCES}
    ADDITIONAL_HEADERS ${MIP_HEADERS}
    PARENT_TARGET mip)
else()
  add_compiler_rt_runtime(clang_rt.mip
    STATIC
    ARCHS ${MIP_SUPPORTED_ARCH}
    CFLAGS ${EXTRA_FLAGS}
    SOURCES ${MIP_SOURCES}
    ADDITIONAL_HEADERS ${MIP_HEADERS}
    PARENT_TARGET mip)
endif()
