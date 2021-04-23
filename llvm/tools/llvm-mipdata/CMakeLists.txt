set(LLVM_LINK_COMPONENTS
  CodeGen
  Core
  Object
  Support
  Symbolize
  )

add_llvm_tool(llvm-mipdata
  llvm-mipdata.cpp
  SymReader.cpp
  DEPENDS
  intrinsics_gen
  )
