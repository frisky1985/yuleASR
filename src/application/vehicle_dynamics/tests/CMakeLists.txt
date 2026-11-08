# src/application/vehicle_dynamics/tests/CMakeLists.txt
# VehicleDynamics SWC host unit tests

add_executable(test_vehicle_dynamics
    test_vehicle_dynamics.c
)

target_link_libraries(test_vehicle_dynamics PRIVATE vehicle_dynamics)

add_test(NAME vehicle_dynamics COMMAND test_vehicle_dynamics)
