# Diagnostics Module CMakeLists.txt
# UDS Diagnostic Stack Implementation

cmake_minimum_required(VERSION 3.16)

# Add subdirectories
add_subdirectory(isotp)
# DoIP: 使用主线 AUTOSAR 实现 (src/bsw/services/doip)，diagnostics/doip stub 版已删除
# add_subdirectory(dcm)  # Already handled separately
# add_subdirectory(dem)  # Future implementation
