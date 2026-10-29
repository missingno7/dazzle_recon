cmake_minimum_required(VERSION 3.21)
project(dazzle_native VERSION 0.1 LANGUAGES C)
set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)
option(DAZZLE_CONVERSION_WARNINGS "Audit implicit integer/float conversions" OFF)
find_package(SDL3 CONFIG REQUIRED)
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
dazzle_executable(dazzle src/main.c)
enable_testing()
file(GLOB DAZZLE_TESTS CONFIGURE_DEPENDS "tests/test_*.c")
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

find_package(Python3 COMPONENTS Interpreter REQUIRED)
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
