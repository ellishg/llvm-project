set(MIP_LIT_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR})

set(MIP_TESTSUITES)
set(MIP_TEST_DEPS ${SANITIZER_COMMON_LIT_TEST_DEPS})
if(NOT COMPILER_RT_STANDALONE_BUILD)
  list(APPEND MIP_TEST_DEPS mip llvm-objcopy llvm-mipdata)
  if(NOT APPLE AND COMPILER_RT_HAS_LLD AND "lld" IN_LIST LLVM_ENABLE_PROJECTS)
    list(APPEND MIP_TEST_DEPS lld)
  endif()
endif()

set(MIP_TEST_ARCH ${MIP_SUPPORTED_ARCH})
if (COMPILER_RT_BUILD_MIP AND COMPILER_RT_HAS_MIP)
  foreach(arch ${MIP_TEST_ARCH})
    set(MIP_TEST_TARGET_ARCH ${arch})
    string(TOLOWER "-${arch}-${OS_NAME}" MIP_TEST_CONFIG_SUFFIX)
    get_test_cc_for_arch(${arch} MIP_TEST_TARGET_CC MIP_TEST_TARGET_CFLAGS)
    string(TOUPPER ${arch} ARCH_UPPER_CASE)
    set(CONFIG_NAME ${ARCH_UPPER_CASE}${OS_NAME}Config)

    configure_lit_site_cfg(
      ${CMAKE_CURRENT_SOURCE_DIR}/lit.site.cfg.py.in
      ${CMAKE_CURRENT_BINARY_DIR}/${CONFIG_NAME}/lit.site.cfg.py)
    list(APPEND MIP_TESTSUITES ${CMAKE_CURRENT_BINARY_DIR}/${CONFIG_NAME})
  endforeach()
endif()

add_lit_testsuite(check-mip "Running the MIP tests"
  ${MIP_TESTSUITES}
  DEPENDS ${MIP_TEST_DEPS})
set_target_properties(check-mip PROPERTIES FOLDER "Compiler-RT Misc")
