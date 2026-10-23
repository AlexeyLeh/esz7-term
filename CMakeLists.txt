cmake_minimum_required(VERSION 3.25)

# set for ARM
set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR arm)

project(ESZ7-TERM)
set(EXE esz7-term)


set(CMAKE_C_STANDARD 99) 
set(CMAKE_C_COMPILER arm-none-linux-gnueabihf-gcc.exe)
#set(CMAKE_C_COMPILER arm-linux-gnueabihf-gcc)

set(CMAKE_C_STANDARD 11)
#set(CMAKE_C_FLAGS "-Wall -Wextra -std=gnu11")
add_compile_options(-Wall -Wextra -std=gnu11)
if(WIN32)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -mno-ms-bitfields")
endif()

set(CMAKE_EXE_LINKER_FLAGS ${CMAKE_EXE_LINKER_FLAGS} "-static")

message("CMAKE_HOST_SYSTEM:      ${CMAKE_HOST_SYSTEM}")
message("CMAKE_SYSTEM_NAME:      ${CMAKE_SYSTEM_NAME}")
message("CMAKE_SYSTEM:           ${CMAKE_SYSTEM}")
message("CMAKE_SYSTEM_PROCESSOR: ${CMAKE_SYSTEM_PROCESSOR}")
message("CMAKE_C_COMPILER:       ${CMAKE_C_COMPILER}")
message("CMAKE_SOURCE_DIR:       ${CMAKE_SOURCE_DIR}")

#set(CMAKE_BUILD_TYPE debug)
#set(CMAKE_BUILD_TYPE release)

#if (${CMAKE_BUILD_TYPE} MATCHES "debug")
#    add_compile_options(-O0)
#    add_compile_options(-g)
#    message("DEBUG")
#else ()
#    #add_compile_options(-O3)
#    message("RELEASE")
#endif ()

#set(CMAKE_VERBOSE_MAKEFILE "true")

include_directories(src)

set(SOURCES
        src/esz7_regs.h
        src/adc_t.h
        src/sram_t.h

        src/cfg.c
        src/cfg.h

        src/crc.c
        src/crc.h

        src/dev.c
        src/dev.h

        src/funcs.c
        src/funcs.h

        src/main.c

        src/mem.c
        src/mem.h

        src/setup_menu.c

        src/sram.c
        src/sram.h

        src/timecode.c
        src/timecode.h

        src/uart.c
        src/uart.h
        src/uart_lnx.c
        src/uart_lnx.h
        src/interrupts.c
        src/ver.c
        src/ver.h
        src/calibration.c
        src/lights.c
)

add_executable(${EXE} ${SOURCES})

if (WIN32)
    message(STATUS "add win lib")
    target_link_libraries(${EXE} wsock32 ws2_32)
endif ()

if (${CMAKE_SYSTEM_PROCESSOR} MATCHES arm)
    message(STATUS "add math lib")
    target_link_libraries(${EXE} m)
endif ()

function (make_dist TARGET)
    SET(VERFILE "${CMAKE_CURRENT_SOURCE_DIR}/src/ver.h")
    if(EXISTS "${VERFILE}")
        file(READ "${VERFILE}" FILE_CONTENT)

        # 2. Извлекаем номер версии (строка #define VER_NUM "3.2.1")
        if(FILE_CONTENT MATCHES [[#define[ \t]+VER_NUM[ \t]+"([^"]+)"]])
            set(VERSION_NUM "${CMAKE_MATCH_1}")
        endif()

        # 3. Извлекаем дату версии (строка #define VER_DATE "Jul 30 2026")
        if(FILE_CONTENT MATCHES [[#define[ \t]+VER_DATE[ \t]+"([^"]+)"]])
            set(VERSION_DATE "${CMAKE_MATCH_1}")
        endif()
    else()
        message(WARNING "Файл ver.h не найден! Используются версии по умолчанию.")
        set(VERSION_NUM "0.0.0")
    endif()
    message(STATUS "Версия: ${VERSION_NUM} от ${VERSION_DATE}")
    string(TOLOWER "${CMAKE_SYSTEM_PROCESSOR}" ARCH_NAME)

    set(DIST_NAME "${TARGET}-${VERSION_NUM}-bin-${ARCH_NAME}")
    set(DIST_DIR "${CMAKE_SOURCE_DIR}/dist-${CMAKE_BUILD_TYPE}")
    message(STATUS "Папка установки: ${DIST_DIR}")
    set(CMAKE_INSTALL_PREFIX "${DIST_DIR}/tmp" CACHE PATH "Install path" FORCE)
    message("CMAKE_INSTALL_PREFIX:   ${CMAKE_INSTALL_PREFIX}")
    install(TARGETS ${TARGET} RUNTIME DESTINATION ".")

    install(CODE "
    string(TIMESTAMP CURRENT_DT \"%Y%m%d-%H%M\")
    set(TARGET_NAME \"${DIST_NAME}-\${CURRENT_DT}\")
    set(TARGET_DIR \"${DIST_DIR}/\${TARGET_NAME}\")
    set(ZIP_NAME \"${DIST_DIR}/\${TARGET_NAME}.zip\")
    file(COPY \"\${CMAKE_INSTALL_PREFIX}/${TARGET}\" DESTINATION \"\${TARGET_DIR}\")
    message(STATUS \"Created: \${TARGET_DIR}\")
    execute_process(
        COMMAND \${CMAKE_COMMAND} -E tar cf \"\${ZIP_NAME}\" --format=zip \"\${TARGET_NAME}\"
        WORKING_DIRECTORY \"${DIST_DIR}\"
        RESULT_VARIABLE ARCHIVE_RESULT
    )

    if(NOT ARCHIVE_RESULT EQUAL 0)
        message(FATAL_ERROR \"Failed to create ZIP package \${ZIP_NAME}\")
    else()
        message(STATUS \"Created: \${ZIP_NAME} ...\")
    endif()

    file(REMOVE_RECURSE \"\${CMAKE_INSTALL_PREFIX}\")
")

endfunction()

make_dist(${EXE})
