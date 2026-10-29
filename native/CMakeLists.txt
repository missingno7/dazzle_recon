cmake_minimum_required(VERSION 3.21)
project(dazzle_native VERSION 0.1 LANGUAGES C)
set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)
option(DAZZLE_CONVERSION_WARNINGS "Audit implicit integer/float conversions" OFF)
find_package(SDL3 CONFIG REQUIRED)
if(WIN32)
    enable_language(RC)
endif()
file(GLOB_RECURSE DAZZLE_SOURCES CONFIGURE_DEPENDS "src/*.c")
list(REMOVE_ITEM DAZZLE_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/main.c")
add_library(dazzle_core STATIC ${DAZZLE_SOURCES})
target_include_directories(dazzle_core PUBLIC include ../include src)
target_link_libraries(dazzle_core PUBLIC SDL3::SDL3 m)
target_compile_options(dazzle_core PUBLIC -Wall -Wextra)
if(DAZZLE_CONVERSION_WARNINGS)
    target_compile_options(dazzle_core PUBLIC -Wconversion -Wsign-conversion)
endif()

function(dazzle_executable target source)
    add_executable(${target} ${source})
    target_link_libraries(${target} PRIVATE dazzle_core)
    if(WIN32)
        add_custom_command(TARGET ${target} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE:SDL3::SDL3> $<TARGET_FILE_DIR:${target}>)
    endif()
endfunction()
enable_testing()
find_package(Python3 COMPONENTS Interpreter REQUIRED)
dazzle_executable(dazzle src/main.c)
if(WIN32)
    add_executable(dazzle_scr WIN32
        scr/main.c scr/cli.c scr/settings.c scr/dialog.c scr/dazzle.rc)
    set_target_properties(dazzle_scr PROPERTIES OUTPUT_NAME dazzle SUFFIX .scr)
    target_include_directories(dazzle_scr PRIVATE include src scr)
    target_link_libraries(dazzle_scr PRIVATE dazzle_core user32 gdi32 comctl32 comdlg32 shell32)
    target_compile_options(dazzle_scr PRIVATE -Wall -Wextra)
    set_source_files_properties(scr/dazzle.rc PROPERTIES COMPILE_FLAGS "-I${CMAKE_CURRENT_SOURCE_DIR}/scr")
    add_custom_command(TARGET dazzle_scr POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:SDL3::SDL3> $<TARGET_FILE_DIR:dazzle_scr>)

    add_executable(test_scr_cli tests/test_scr_cli.c scr/cli.c)
    target_include_directories(test_scr_cli PRIVATE include src scr)
    target_link_libraries(test_scr_cli PRIVATE SDL3::SDL3)
    target_compile_options(test_scr_cli PRIVATE -UNDEBUG -Wall -Wextra)
    add_test(NAME screensaver_cli COMMAND test_scr_cli)

    add_executable(test_scr_settings tests/test_scr_settings.c scr/settings.c)
    target_include_directories(test_scr_settings PRIVATE include src scr)
    target_link_libraries(test_scr_settings PRIVATE dazzle_core)
    target_compile_options(test_scr_settings PRIVATE -UNDEBUG -Wall -Wextra)
    add_test(NAME screensaver_settings COMMAND test_scr_settings)
    set_tests_properties(screensaver_settings PROPERTIES TIMEOUT 30)

    add_test(NAME screensaver_lifecycle
        COMMAND ${Python3_EXECUTABLE} -B -I
            "${CMAKE_CURRENT_SOURCE_DIR}/tests/verify_screensaver.py"
            "$<TARGET_FILE:dazzle_scr>" "${CMAKE_BINARY_DIR}/screensaver-appdata")
    set_tests_properties(screensaver_lifecycle PROPERTIES TIMEOUT 30
        ENVIRONMENT "SDL_VIDEODRIVER=dummy;SDL_AUDIODRIVER=dummy")
endif()
file(GLOB DAZZLE_TESTS CONFIGURE_DEPENDS "tests/test_*.c")
if(WIN32)
    list(REMOVE_ITEM DAZZLE_TESTS
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/test_scr_cli.c"
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/test_scr_settings.c")
endif()
foreach(source IN LISTS DAZZLE_TESTS)
    get_filename_component(test_name "${source}" NAME_WE)
    dazzle_executable(${test_name} "${source}")
    # Test assertions remain active even with optimized configurations.
    target_compile_options(${test_name} PRIVATE -UNDEBUG)
    # Link seam: a test may define its own doubles for program symbols (e.g. counting help_screen calls);
    # the test object comes first, so its definition wins. Never used for the dazzle executable.
    target_link_options(${test_name} PRIVATE -Wl,--allow-multiple-definition)
    add_test(NAME ${test_name} COMMAND ${test_name})
    set_tests_properties(${test_name} PROPERTIES TIMEOUT 30
        ENVIRONMENT "SDL_VIDEODRIVER=dummy;SDL_AUDIODRIVER=dummy")
endforeach()
add_test(NAME slice_cli COMMAND dazzle --headless --deterministic --seed 1
    --slice sierpinski --frames 4 --steps 10000)
set_tests_properties(slice_cli PROPERTIES TIMEOUT 30)

add_test(NAME snapshot_png
    COMMAND ${Python3_EXECUTABLE} -B -I
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/verify_snapshot_png.py"
        "${CMAKE_BINARY_DIR}/snapshot-png")
set_tests_properties(snapshot_png PROPERTIES TIMEOUT 30)
add_test(NAME acceptance_reference
    COMMAND ${Python3_EXECUTABLE} -B -I
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/verify_acceptance.py"
        "$<TARGET_FILE:dazzle>" "${CMAKE_BINARY_DIR}/acceptance")
set_tests_properties(acceptance_reference PROPERTIES TIMEOUT 60)
set(DAZZLE_ORIGINAL_EXE "${CMAKE_CURRENT_SOURCE_DIR}/../assets/DAZZLE.EXE" CACHE FILEPATH
    "Original DAZZLE.EXE for resource tests (appended manual and logo)")
add_test(NAME test_dlogo_reference
    COMMAND ${Python3_EXECUTABLE} -B -I
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/verify_dlogo.py"
        "$<TARGET_FILE:test_dlogo>"
        "${DAZZLE_ORIGINAL_EXE}"
        "${CMAKE_BINARY_DIR}")
set_tests_properties(test_dlogo_reference PROPERTIES TIMEOUT 30)
add_test(NAME integration_controls
    COMMAND ${Python3_EXECUTABLE} -B -I
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/verify_controls.py"
        "$<TARGET_FILE:dazzle>" "${CMAKE_BINARY_DIR}/integration/controls" "${DAZZLE_ORIGINAL_EXE}")
set_tests_properties(integration_controls PROPERTIES TIMEOUT 60)

foreach(scenario vga ega tall keys repeat speed)
    add_test(NAME integration_${scenario}
        COMMAND ${Python3_EXECUTABLE} -B -I
            "${CMAKE_CURRENT_SOURCE_DIR}/tests/verify_integration.py"
            "$<TARGET_FILE:dazzle>" "${CMAKE_BINARY_DIR}/integration/${scenario}" "${scenario}")
    set_tests_properties(integration_${scenario} PROPERTIES TIMEOUT 30)
endforeach()
