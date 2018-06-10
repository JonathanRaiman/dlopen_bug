cmake_minimum_required(VERSION 2.8 FATAL_ERROR)
set(CMAKE_LEGACY_CYGWIN_WIN32 0)

project("cmake_test")
set(PROJECT_VERSION_MAJOR 1)
set(PROJECT_VERSION_MINOR 0)
set(PROJECT_VERSION_PATCH 0)

set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake
                      ${CMAKE_MODULE_PATH})

if( UNIX AND NOT APPLE )
    execute_process(COMMAND /usr/bin/lsb_release -i -s
                            OUTPUT_VARIABLE LINUX_DISTRO
                            OUTPUT_STRIP_TRAILING_WHITESPACE)
endif(UNIX AND NOT APPLE)

################################################################################
##                            COMPILER SETUP                                  ##
################################################################################

# ensure correct GCC version
set(GCC_MINIMUM_REQUIRED "4.9.2")
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
  if(CMAKE_CXX_COMPILER_VERSION VERSION_LESS ${GCC_MINIMUM_REQUIRED})
    message(FATAL_ERROR "Mimimum required version of gcc is ${GCC_MINIMUM_REQUIRED}")
  endif()
endif()

if (APPLE)
    set (CMAKE_CXX_COMPILER "clang++")
endif(APPLE)

set(ENABLED_WARNINGS "-Wno-return-type -Wno-undefined-bool-conversion -Wno-potentially-evaluated-expression")

if(CMAKE_BUILD_TYPE MATCHES "debug")
    set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -g -fPIC ${ENABLED_WARNINGS} -gdwarf-2 -fno-standalone-debug -ggnu-pubnames")
    if (APPLE)
        # inspiration is here https://chromium.googlesource.com/external/dynamorio/+/f4eaad844089ed8422fb47fba1a2518dfcfd7fbb/CMakeLists.txt
        find_program(DSYMUTIL_PROGRAM dsymutil)
        if (DSYMUTIL_PROGRAM)
          set(CMAKE_C_LINK_EXECUTABLE
            "${CMAKE_C_LINK_EXECUTABLE}"
            "${DSYMUTIL_PROGRAM} <TARGET>")
          set(CMAKE_C_CREATE_SHARED_LIBRARY
            "${CMAKE_C_CREATE_SHARED_LIBRARY}"
            "${DSYMUTIL_PROGRAM} <TARGET>")
          set(CMAKE_CXX_LINK_EXECUTABLE
            "${CMAKE_CXX_LINK_EXECUTABLE}"
            "${DSYMUTIL_PROGRAM} <TARGET>")
          set(CMAKE_CXX_CREATE_SHARED_LIBRARY
            "${CMAKE_CXX_CREATE_SHARED_LIBRARY}"
            "${DSYMUTIL_PROGRAM} <TARGET>")
        endif ()
    endif ()
elseif(CMAKE_BUILD_TYPE MATCHES "nooptimize")
    set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 ${ENABLED_WARNINGS}")
else(CMAKE_BUILD_TYPE MATCHES "debug")
    set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -O3 -w ${ENABLED_WARNINGS}")
endif(CMAKE_BUILD_TYPE MATCHES "debug")

if (CMAKE_BUILD_TYPE STREQUAL "")
    set(CMAKE_BUILD_TYPE "default")
endif (CMAKE_BUILD_TYPE STREQUAL "")
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")


################################################################################
##                       GENERATING VIRTUAL SOURCE TREE                       ##
################################################################################

if (NOT DEFINED TEST_PROJECT_SOURCE_DIRS)
    set(TEST_PROJECT_SOURCE_DIRS "test_project")
endif()


set(GENERATED_FILES_DIR ${CMAKE_BINARY_DIR}/test_project_generated)
file(MAKE_DIRECTORY ${GENERATED_FILES_DIR})

function(test_project_generate_file OldPath NewPath)
    add_custom_command(OUTPUT ${NewPath}
            COMMAND ${CMAKE_COMMAND} -E copy ${OldPath} ${NewPath}
            WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
            DEPENDS ${OldPath})
endfunction()

foreach(SourceDir ${TEST_PROJECT_SOURCE_DIRS})
    message(STATUS "Scanning sources under ${SourceDir}...")
    file(GLOB_RECURSE FilesInDir "${PROJECT_SOURCE_DIR}/${SourceDir}/*")
    foreach(FilePath ${FilesInDir})
        get_filename_component(FileNameWe ${FilePath} NAME_WE)
        get_filename_component(FileExt    ${FilePath} EXT)
        get_filename_component(FileDir  ${FilePath} DIRECTORY)
        file(RELATIVE_PATH FileRelDir  "${PROJECT_SOURCE_DIR}" "${FileDir}")
        file(RELATIVE_PATH FileRelPath "${PROJECT_SOURCE_DIR}" "${FilePath}")
        SET(IGNORE_FILE FALSE)
        foreach(ExcludePattern ${TEST_PROJECT_SOURCE_DIRS_EXCLUDE})
            if (FileRelPath MATCHES ${ExcludePattern})
                message(STATUS "Ignoring: ${FileRelPath}")
                SET(IGNORE_FILE TRUE)
            endif()
        endforeach()
        if (NOT IGNORE_FILE)
            if (FileExt STREQUAL ".cpp" OR (FileExt STREQUAL ".cu.cpp" AND NOT WITH_CUDA))
                set(NewFilePath "${GENERATED_FILES_DIR}/${FileRelDir}/${FileNameWe}.cpp")
                test_project_generate_file("${FilePath}" "${NewFilePath}")
                list(APPEND TestProjectSources "${NewFilePath}")
            elseif(FileExt STREQUAL ".cu" OR (FileExt STREQUAL ".cu.cpp" AND WITH_CUDA))
                set(NewFilePath "${GENERATED_FILES_DIR}/${FileRelDir}/${FileNameWe}.cu")
                test_project_generate_file("${FilePath}" "${NewFilePath}")
                list(APPEND TestProjectCudaSources "${NewFilePath}")
            elseif(FileExt STREQUAL ".h.in")
                set(NewFilePath "${GENERATED_FILES_DIR}/${FileRelDir}/${FileNameWe}.h")
                configure_file("${FilePath}" "${NewFilePath}")
                list(APPEND TestProjectHeaders ${NewFilePath})
            elseif(FileExt STREQUAL ".h")
                set(NewFilePath "${GENERATED_FILES_DIR}/${FileRelDir}/${FileNameWe}.h")
                test_project_generate_file("${FilePath}" "${NewFilePath}")
                list(APPEND TestProjectHeaders ${NewFilePath})
            elseif(FileExt STREQUAL ".DS_Store")
            else()
                message(Warning ": File ignored by compiler \"${FileRelPath}\"")
            endif()
        endif()
    endforeach()
endforeach()


################################################################################
##                      CONFIGURATION AND COMPILATION                         ##
################################################################################

## CPP TARGET
add_library(test_project SHARED ${TestProjectSources} ${TestProjectHeaders})
add_dependencies(test_project test_project_cuda_headers)
add_executable(app app.cpp)
target_link_libraries(app test_project)
