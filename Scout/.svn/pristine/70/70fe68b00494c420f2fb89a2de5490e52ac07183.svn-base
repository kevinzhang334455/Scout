# make cmake happy
cmake_minimum_required(VERSION 2.4)

project(ScoutClangAddons) 

# Scout clangAddons version information
set(SCOUT_CLANGADDONS_VERSION 1.0)
set(PACKAGE_NAME ScoutClangAddons)
set(PACKAGE_VERSION 1.0svn)
set(PACKAGE_STRING "${PACKAGE_NAME} ${PACKAGE_VERSION}")
set(PACKAGE_BUGREPORT "olaf.krzikalla@tu-dresden.de") 

# configuration
include(CMakeLists.config)

list(APPEND CMAKE_MODULE_PATH "${LLVM_DIR}/cmake/modules") 
link_directories(
  ${LLVM_DIR}/lib       
  ${CMAKE_CURRENT_SOURCE_DIR}/build
  )       
include(AddLLVM)

if( MSVC )
  # List of valid CRTs for MSVC
  set(MSVC_CRT
    MD
    MDd)

  set(LLVM_USE_CRT "" CACHE STRING "Specify VC++ CRT to use for debug/release configurations.")
  add_definitions( -D_CRT_SECURE_NO_DEPRECATE -D_CRT_SECURE_NO_WARNINGS )
  add_definitions( -D_SCL_SECURE_NO_WARNINGS -DCRT_NONSTDC_NO_WARNINGS )
  add_definitions( -D_SCL_SECURE_NO_DEPRECATE )
  add_definitions( -wd4146 -wd4503 -wd4996 -wd4800 -wd4244 -wd4624 )
  add_definitions( -wd4355 -wd4715 -wd4180 -wd4345 -wd4224 )

  # Suppress 'new behavior: elements of array 'array' will be default initialized'
  add_definitions( -wd4351 )

  if (NOT ${LLVM_USE_CRT} STREQUAL "")
    list(FIND MSVC_CRT ${LLVM_USE_CRT} idx)
    if (idx LESS 0)
      message(FATAL_ERROR "Invalid value for LLVM_USE_CRT: ${LLVM_USE_CRT}. Valid options are one of: ${MSVC_CRT}")
    endif (idx LESS 0)
    add_definitions("/${LLVM_USE_CRT}")
    message(STATUS "Using VC++ CRT: ${LLVM_USE_CRT}")
  endif (NOT ${LLVM_USE_CRT} STREQUAL "")
endif( MSVC ) 

macro(add_scout_library name)
  set(srcs ${ARGN})
  if(MSVC_IDE OR XCODE)
    file( GLOB_RECURSE headers *.h)
    set(srcs ${srcs} ${headers})
    string( REGEX MATCHALL "/[^/]+" split_path ${CMAKE_CURRENT_SOURCE_DIR})
    list( GET split_path -1 dir)
    file( GLOB_RECURSE headers ../../include/clang${dir}/*.h) 
    set(srcs ${srcs} ${headers})
  endif(MSVC_IDE OR XCODE)
	set( ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_ARCHIVE_OUTPUT_DIRECTORY} ) 
  add_library( ${name} ${srcs} )
  if(MSVC)
    get_target_property(cflag ${name} COMPILE_FLAGS)
    if(NOT cflag)
      set(cflag "")
    endif(NOT cflag)
    set_target_properties(${name} PROPERTIES COMPILE_FLAGS ${cflag})
  endif(MSVC)

  get_property(lib_deps GLOBAL PROPERTY LLVMBUILD_LIB_DEPS_${name})
  target_link_libraries(${name} ${lib_deps})
    
endmacro(add_scout_library)

macro(add_scout_executable name)
  add_llvm_executable( ${name} ${ARGN} )
  set_target_properties(${name} PROPERTIES FOLDER "Scout executable")
  target_link_libraries(${name} 
		scoutClangInterface 
		scoutClangVectorizing 
		scoutClangASTProcessing
		clangFrontend 
		clangDriver 
		clangStaticAnalyzerCore 
		clangIndex 
		clangSerialization 
		clangRewriteCore 
		clangParse 
		clangSema 
		clangEdit 
		clangAnalysis 
		clangAST 
		clangLex 
		clangBasic 
		LLVMCore 
		LLVMBitReader 
		LLVMBitWriter
		LLVMX86Desc
		LLVMX86AsmPrinter
		LLVMX86AsmParser
		LLVMX86Info
		LLVMX86Utils
		LLVMMCParser
		LLVMMC
		LLVMOption
		LLVMSupport 
		LLVMScalarOpts 
		LLVMTransformUtils
	)
endmacro(add_scout_executable) 

include_directories(
  ${CMAKE_CURRENT_SOURCE_DIR}/include
  ${BOOST_DIR}
  ${LLVM_DIR}/include
	${LLVM_DIR}/tools/clang/include
  )

add_definitions( -D_GNU_SOURCE )

set( CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/build )
set( CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/build ) 

add_subdirectory(lib)
