#============================================================================
# tests/CMakeLists.txt — Host-side unit tests for seat_control demo
#============================================================================

#----------------------------------------------------------------------------
# Mock BSW library (shared across all test targets)
#----------------------------------------------------------------------------
add_library(mock_bsw STATIC
    mocks/mock_bsw.c
)

target_include_directories(mock_bsw PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/mocks
    ${CMAKE_CURRENT_SOURCE_DIR}/../config
    ${CMAKE_CURRENT_SOURCE_DIR}/../include
)

target_compile_options(mock_bsw PRIVATE
    -std=c99
    -Wall -Wextra
    -Wno-unused-parameter
    -Wno-unused-variable
)

#----------------------------------------------------------------------------
# Test 1: Seat Control state machine
#----------------------------------------------------------------------------
add_executable(test_seat_control
    test_seat_control.c
    ../src/SeatControl.c
    ../src/SeatPosition.c
    ../src/SeatHeating.c
    ../src/SeatCommunication.c
    ../src/SeatMemory.c
)

target_link_libraries(test_seat_control PRIVATE mock_bsw)

target_include_directories(test_seat_control PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/mocks
    ${CMAKE_CURRENT_SOURCE_DIR}/../config
    ${CMAKE_CURRENT_SOURCE_DIR}/../include
)

target_compile_options(test_seat_control PRIVATE
    -std=c99
    -Wall -Wextra
    -Wno-unused-parameter
    -Wno-unused-variable
)

add_test(NAME test_seat_control
    COMMAND test_seat_control
)

#----------------------------------------------------------------------------
# Test 2: Seat Position PID control
#----------------------------------------------------------------------------
add_executable(test_seat_position
    test_seat_position.c
    ../src/SeatPosition.c
)

target_link_libraries(test_seat_position PRIVATE mock_bsw)

target_include_directories(test_seat_position PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/mocks
    ${CMAKE_CURRENT_SOURCE_DIR}/../config
    ${CMAKE_CURRENT_SOURCE_DIR}/../include
)

target_compile_options(test_seat_position PRIVATE
    -std=c99
    -Wall -Wextra
    -Wno-unused-parameter
    -Wno-unused-variable
)

add_test(NAME test_seat_position
    COMMAND test_seat_position
)

#----------------------------------------------------------------------------
# Test 3: Seat Heating PWM control
#----------------------------------------------------------------------------
add_executable(test_seat_heating
    test_seat_heating.c
    ../src/SeatHeating.c
)

target_link_libraries(test_seat_heating PRIVATE mock_bsw)

target_include_directories(test_seat_heating PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/mocks
    ${CMAKE_CURRENT_SOURCE_DIR}/../config
    ${CMAKE_CURRENT_SOURCE_DIR}/../include
)

target_compile_options(test_seat_heating PRIVATE
    -std=c99
    -Wall -Wextra
    -Wno-unused-parameter
    -Wno-unused-variable
)

add_test(NAME test_seat_heating
    COMMAND test_seat_heating
)

#----------------------------------------------------------------------------
# Summary
#----------------------------------------------------------------------------
message(STATUS "  Tests enabled: test_seat_control, test_seat_position, test_seat_heating")
