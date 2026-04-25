# cmake/modules/packaging.cmake
# Package generation configuration

#==============================================================================
# Package Generation Setup
#==============================================================================
if(BUILD_PACKAGE)
    message(STATUS "Configuring package generation")
    
    # General package information
    set(CPACK_PACKAGE_NAME "eth-dds")
    set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
    set(CPACK_PACKAGE_VERSION_MAJOR ${PROJECT_VERSION_MAJOR})
    set(CPACK_PACKAGE_VERSION_MINOR ${PROJECT_VERSION_MINOR})
    set(CPACK_PACKAGE_VERSION_PATCH ${PROJECT_VERSION_PATCH})
    
    set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "ETH-DDS Integration Framework")
    set(CPACK_PACKAGE_DESCRIPTION 
"High-performance DDS middleware with Ethernet and TSN support.
Designed for automotive and industrial applications.
Features:
- DDS/RTPS compliant middleware
- TSN (Time-Sensitive Networking) support
- AUTOSAR Classic and Adaptive integration
- ASIL-D functional safety")
    
    set(CPACK_PACKAGE_VENDOR "ETH-DDS Project")
    set(CPACK_PACKAGE_CONTACT "support@eth-dds.org")
    set(CPACK_PACKAGE_HOMEPAGE_URL "https://github.com/frisky1985/yuleASR")
    
    # License
    set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE")
    if(EXISTS ${CPACK_RESOURCE_FILE_LICENSE})
        message(STATUS "  License file: ${CPACK_RESOURCE_FILE_LICENSE}")
    else()
        message(WARNING "  License file not found")
    endif()
    
    # README
    set(CPACK_RESOURCE_FILE_README "${CMAKE_CURRENT_SOURCE_DIR}/README.md")
    
    #==============================================================================
    # DEB Package Configuration
    #==============================================================================
    set(CPACK_DEBIAN_PACKAGE_SECTION "devel")
    set(CPACK_DEBIAN_PACKAGE_PRIORITY "optional")
    set(CPACK_DEBIAN_PACKAGE_DEPENDS "libc6 (>= 2.17)")
    set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON)
    set(CPACK_DEBIAN_PACKAGE_GENERATE_SHLIBS ON)
    
    # Architecture detection
    if(CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64")
        set(CPACK_DEBIAN_PACKAGE_ARCHITECTURE "amd64")
    elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "i686|i386")
        set(CPACK_DEBIAN_PACKAGE_ARCHITECTURE "i386")
    elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "aarch64")
        set(CPACK_DEBIAN_PACKAGE_ARCHITECTURE "arm64")
    elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "arm")
        set(CPACK_DEBIAN_PACKAGE_ARCHITECTURE "armhf")
    else()
        set(CPACK_DEBIAN_PACKAGE_ARCHITECTURE ${CMAKE_SYSTEM_PROCESSOR})
    endif()
    
    #==============================================================================
    # RPM Package Configuration
    #==============================================================================
    set(CPACK_RPM_PACKAGE_GROUP "Development/Libraries")
    set(CPACK_RPM_PACKAGE_LICENSE "Apache-2.0")
    set(CPACK_RPM_PACKAGE_REQUIRES "glibc >= 2.17")
    
    # Architecture mapping
    if(CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64")
        set(CPACK_RPM_PACKAGE_ARCHITECTURE "x86_64")
    elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "aarch64")
        set(CPACK_RPM_PACKAGE_ARCHITECTURE "aarch64")
    else()
        set(CPACK_RPM_PACKAGE_ARCHITECTURE ${CMAKE_SYSTEM_PROCESSOR})
    endif()
    
    #==============================================================================
    # TGZ (tar.gz) Configuration
    #==============================================================================
    set(CPACK_ARCHIVE_COMPONENT_INSTALL ON)
    
    #==============================================================================
    # Component-Based Installation
    #==============================================================================
    set(CPACK_COMPONENTS_ALL
        libraries
        headers
        examples
        documentation
        tools
    )
    
    # Component descriptions
    set(CPACK_COMPONENT_LIBRARIES_DISPLAY_NAME "Libraries")
    set(CPACK_COMPONENT_LIBRARIES_DESCRIPTION "Core ETH-DDS libraries")
    
    set(CPACK_COMPONENT_HEADERS_DISPLAY_NAME "Headers")
    set(CPACK_COMPONENT_HEADERS_DESCRIPTION "C/C++ header files for development")
    set(CPACK_COMPONENT_HEADERS_DEPENDS libraries)
    
    set(CPACK_COMPONENT_EXAMPLES_DISPLAY_NAME "Examples")
    set(CPACK_COMPONENT_EXAMPLES_DESCRIPTION "Example applications and configurations")
    set(CPACK_COMPONENT_EXAMPLES_DEPENDS libraries)
    
    set(CPACK_COMPONENT_DOCUMENTATION_DISPLAY_NAME "Documentation")
    set(CPACK_COMPONENT_DOCUMENTATION_DESCRIPTION "API documentation and user guides")
    
    set(CPACK_COMPONENT_TOOLS_DISPLAY_NAME "Tools")
    set(CPACK_COMPONENT_TOOLS_DESCRIPTION "Configuration and analysis tools")
    set(CPACK_COMPONENT_TOOLS_DEPENDS libraries)
    
    #==============================================================================
    # Installation Prefix
    #==============================================================================
    if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
        set(CMAKE_INSTALL_PREFIX "/usr/local" CACHE PATH "Installation prefix" FORCE)
    endif()
    
    set(CPACK_PACKAGING_INSTALL_PREFIX ${CMAKE_INSTALL_PREFIX})
    
    #==============================================================================
    # Generator Selection
    #==============================================================================
    if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
        set(CPACK_GENERATOR "TGZ;DEB;RPM")
    elseif(CMAKE_SYSTEM_NAME STREQUAL "Windows")
        set(CPACK_GENERATOR "ZIP;NSIS")
    elseif(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
        set(CPACK_GENERATOR "TGZ;DragNDrop")
    else()
        set(CPACK_GENERATOR "TGZ")
    endif()
    
    message(STATUS "  Package generators: ${CPACK_GENERATOR}")
    
    #==============================================================================
    # Include CPack
    #==============================================================================
    include(CPack)
    
    #==============================================================================
    # Package Target
    #==============================================================================
    add_custom_target(package-all
        COMMAND ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR} --target package
        COMMENT "Building all packages"
    )
endif()

#==============================================================================
# Installation Rules
#==============================================================================

# Libraries
install(TARGETS eth_dds
    COMPONENT libraries
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
)

# Headers
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include/
    COMPONENT headers
    DESTINATION include/eth_dds
    FILES_MATCHING PATTERN "*.h" PATTERN "*.hpp"
)

# Examples
if(BUILD_EXAMPLES)
    install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/examples/
        COMPONENT examples
        DESTINATION share/eth_dds/examples
        PATTERN "build" EXCLUDE
        PATTERN ".git" EXCLUDE
    )
endif()

# Documentation
if(BUILD_DOCUMENTATION AND DOXYGEN_FOUND)
    install(DIRECTORY ${CMAKE_BINARY_DIR}/docs/html/
        COMPONENT documentation
        DESTINATION share/doc/eth_dds
        OPTIONAL
    )
endif()

# Tools
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/tools/
    COMPONENT tools
    DESTINATION share/eth_dds/tools
    USE_SOURCE_PERMISSIONS
    PATTERN "__pycache__" EXCLUDE
    PATTERN "*.pyc" EXCLUDE
    PATTERN ".git" EXCLUDE
)

# License and README
install(FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/LICENSE
    ${CMAKE_CURRENT_SOURCE_DIR}/README.md
    DESTINATION share/doc/eth_dds
    COMPONENT documentation
    OPTIONAL
)
