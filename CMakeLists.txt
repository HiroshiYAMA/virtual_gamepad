cmake_minimum_required(VERSION 3.0.0)
project(virtual_gamepad)

set(CMAKE_INSTALL_PREFIX "../install")
set(exe_install_path ".")
set(lib_install_path ".")

set(exe_target_send "vgmpad_send")
set(exe_target_recv "vgmpad_recv")

set(SRC_DIR "src")

# for exe.
add_executable(${exe_target_send}
    ${SRC_DIR}/vgmpad_send.cpp
    ${SRC_DIR}/devGamepad.cpp
)
add_executable(${exe_target_recv}
    ${SRC_DIR}/vgmpad_recv.cpp
    ${SRC_DIR}/devGamepad.cpp
)

set_target_properties(${exe_target_send} PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED YES
    CXX_EXTENSIONS NO
)
if(APPLE)
    set_target_properties(${exe_target_send} PROPERTIES
        BUILD_RPATH "@executable_path"
        INSTALL_RPATH "@executable_path"
    )
else(APPLE)
    set_target_properties(${exe_target_send} PROPERTIES
        BUILD_RPATH "$ORIGIN"
        INSTALL_RPATH "$ORIGIN"
    )
endif(APPLE)
set_target_properties(${exe_target_recv} PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED YES
    CXX_EXTENSIONS NO
)
if(APPLE)
    set_target_properties(${exe_target_recv} PROPERTIES
        BUILD_RPATH "@executable_path"
        INSTALL_RPATH "@executable_path"
    )
else(APPLE)
    set_target_properties(${exe_target_recv} PROPERTIES
        BUILD_RPATH "$ORIGIN"
        INSTALL_RPATH "$ORIGIN"
    )
endif(APPLE)

### Linux specific configuration ###
if(UNIX AND NOT APPLE)
    if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
        if(CMAKE_CXX_COMPILER_VERSION VERSION_LESS 8)
            # Must use std::experimental namespace if older than GCC8
            message("[${PROJECT_NAME}] GCC version less than 8. Using std::experimental namespace.")
            target_compile_definitions(${exe_target_send} PRIVATE USE_EXPERIMENTAL_FS)
            target_compile_definitions(${exe_target_recv} PRIVATE USE_EXPERIMENTAL_FS)
        endif()

        if(CMAKE_CXX_COMPILER_VERSION VERSION_LESS 9)
            # Must explicitly link separate std::filesystem if older than GCC9
            message("[${PROJECT_NAME}] GCC version less than 9. Explicitly linking separate std::filesystem library.")
            target_link_libraries(${exe_target_send} stdc++fs)
            target_link_libraries(${exe_target_recv} stdc++fs)
        endif()
    endif()
endif(UNIX AND NOT APPLE)

# SDL2.
if(NOT WIN32)
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(SDL2 REQUIRED sdl2)
else(NOT WIN32)
    set(SDL2_INCLUDE_DIRS "c:/SDL2/include")
    set(SDL2_LIB_DIR "c:/SDL2/lib/x64")
    set(SDL2_LIBRARIES "${SDL2_LIB_DIR}/SDL2.lib" "${SDL2_LIB_DIR}/SDL2main.lib")
endif(NOT WIN32)
message("SDL2 : ${SDL2_VERSION}")
message("SDL2 include dirs: ${SDL2_INCLUDE_DIRS}")
message("SDL2 libraries: ${SDL2_LIBRARIES}")
message("SDL2 cflags: ${SDL2_CFLAGS}")
add_definitions(-D_REENTRANT)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
include_directories(${SDL2_INCLUDE_DIRS})
target_link_libraries(${exe_target_send} ${SDL2_LIBRARIES})
target_link_libraries(${exe_target_recv} ${SDL2_LIBRARIES})

# SRT.
set(USE_SRT "YES")
if(USE_SRT)
    add_definitions(-DUSE_SRT)
if(NOT WIN32)
    pkg_check_modules(SRT REQUIRED srt)
else(NOT WIN32)
    set(SRT_INCLUDE_DIRS "C:/Program Files (x86)/SRT/include/srt")
    set(SRT_LIB_DIR "C:/Program Files (x86)/SRT/lib")
    set(OPENSSL_LIB_DIR "C:/OpenSSL-Win64/lib")
    set(SRT_LIBRARIES "${SRT_LIB_DIR}/srt_static.lib"
        "${OPENSSL_LIB_DIR}/libssl_static.lib"
        "${OPENSSL_LIB_DIR}/libcrypto_static.lib"
        "ws2_32.lib"
    )
endif(NOT WIN32)
    message("SRT : ${SRT_VERSION}")
    message("SRT include dirs: ${SRT_INCLUDE_DIRS}")
    message("SRT libraries: ${SRT_LIBRARIES}")
    message("SRT cflags: ${SRT_CFLAGS}")
    include_directories(${SRT_INCLUDE_DIRS})
    target_link_libraries(${exe_target_send} ${SRT_LIBRARIES})
    target_link_libraries(${exe_target_recv} ${SRT_LIBRARIES})
endif()

## Install path defined in parent CMakeLists
install(TARGETS ${exe_target_send} DESTINATION ${exe_install_path})
install(TARGETS ${exe_target_recv} DESTINATION ${exe_install_path})
