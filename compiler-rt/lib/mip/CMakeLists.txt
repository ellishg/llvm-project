add_compiler_rt_component(mip)

set(MIP_SOURCES
  MIPHelper.c
  MIPRuntime.cpp
)

set(MIP_HEADERS
  MIPHelper.h
)

set(aarch64_SOURCES
  MIPHelperAArch64.S
)

set(arm64_SOURCES
  ${aarch64_SOURCES}
)

set(x86_64_SOURCES
  MIPHelperX86_64.S
)

include_directories(../../include)

set(ASM_SOURCES "")
foreach(arch ${MIP_SUPPORTED_ARCH})
  list(APPEND ASM_SOURCES ${${arch}_SOURCES})
  if(APPLE)
    set_source_files_properties(
      ${${arch}_SOURCES}
      PROPERTIES COMPILE_FLAGS
      "-arch ${arch}"
    )
  endif()
endforeach()

if(APPLE)
  add_compiler_rt_runtime(clang_rt.mip
    STATIC
    OS ${MIP_SUPPORTED_OS}
    ARCHS ${MIP_SUPPORTED_ARCH}
    CFLAGS ${EXTRA_FLAGS}
    SOURCES ${MIP_SOURCES} ${ASM_SOURCES}
    ADDITIONAL_HEADERS ${MIP_HEADERS}
    PARENT_TARGET mip)
else()
  add_compiler_rt_runtime(clang_rt.mip
    STATIC
    ARCHS ${MIP_SUPPORTED_ARCH}
    CFLAGS ${EXTRA_FLAGS}
    SOURCES ${MIP_SOURCES} ${ASM_SOURCES}
    ADDITIONAL_HEADERS ${MIP_HEADERS}
    PARENT_TARGET mip)
endif()
