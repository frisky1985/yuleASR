# Diagnostics Module CMakeLists.txt
# UDS Diagnostic Stack Implementation

cmake_minimum_required(VERSION 3.16)

# Add subdirectories
add_subdirectory(isotp)
add_subdirectory(doip)
# add_subdirectory(dcm)  # Already handled separately
# add_subdirectory(dem)  # Future implementation
