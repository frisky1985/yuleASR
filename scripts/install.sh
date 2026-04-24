#!/bin/bash
#
# ETH-DDS Integration Installation Script
# Version: 2.0.0
#

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Configuration
INSTALL_PREFIX="/usr/local"
BUILD_TYPE="Release"
ENABLE_ALL="ON"
SKIP_TESTS="OFF"
SKIP_EXAMPLES="OFF"

# Function to print colored messages
print_info() {
    echo -e "${GREEN}[INFO]${NC} $1"
}

print_warn() {
    echo -e "${YELLOW}[WARN]${NC} $1"
}

print_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

# Function to check dependencies
check_dependencies() {
    print_info "Checking dependencies..."
    
    # Check CMake
    if ! command -v cmake &> /dev/null; then
        print_error "CMake is not installed. Please install CMake >= 3.14"
        exit 1
    fi
    
    CMAKE_VERSION=$(cmake --version | head -n1 | cut -d' ' -f3)
    print_info "Found CMake version: $CMAKE_VERSION"
    
    # Check compiler
    if ! command -v gcc &> /dev/null && ! command -v clang &> /dev/null; then
        print_error "No C compiler found. Please install GCC or Clang"
        exit 1
    fi
    
    # Check pthread
    if ! ldconfig -p | grep -q libpthread; then
        print_warn "libpthread not found in library cache"
    fi
    
    print_info "Dependencies check completed"
}

# Function to parse arguments
parse_args() {
    while [[ $# -gt 0 ]]; do
        case $1 in
            --prefix)
                INSTALL_PREFIX="$2"
                shift 2
                ;;
            --debug)
                BUILD_TYPE="Debug"
                shift
                ;;
            --minimal)
                ENABLE_ALL="OFF"
                shift
                ;;
            --skip-tests)
                SKIP_TESTS="ON"
                shift
                ;;
            --skip-examples)
                SKIP_EXAMPLES="ON"
                shift
                ;;
            --help)
                show_help
                exit 0
                ;;
            *)
                print_error "Unknown option: $1"
                show_help
                exit 1
                ;;
        esac
    done
}

# Function to show help
show_help() {
    cat << EOF
ETH-DDS Integration Installation Script

Usage: $0 [OPTIONS]

Options:
    --prefix PATH       Set installation prefix (default: /usr/local)
    --debug             Build in debug mode
    --minimal           Build with minimal features only
    --skip-tests        Skip building tests
    --skip-examples     Skip building examples
    --help              Show this help message

Examples:
    $0                          # Install with default settings
    $0 --prefix /opt/eth-dds    # Install to custom location
    $0 --debug --minimal        # Debug build with minimal features
EOF
}

# Function to create build directory
setup_build() {
    print_info "Setting up build directory..."
    
    SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
    PROJECT_DIR="$(dirname "$SCRIPT_DIR")"
    BUILD_DIR="$PROJECT_DIR/build"
    
    mkdir -p "$BUILD_DIR"
    cd "$BUILD_DIR"
    
    print_info "Build directory: $BUILD_DIR"
}

# Function to configure project
configure_project() {
    print_info "Configuring project..."
    print_info "Build type: $BUILD_TYPE"
    print_info "Install prefix: $INSTALL_PREFIX"
    print_info "Enable all features: $ENABLE_ALL"
    
    CMAKE_ARGS=(
        -DCMAKE_BUILD_TYPE="$BUILD_TYPE"
        -DCMAKE_INSTALL_PREFIX="$INSTALL_PREFIX"
        -DBUILD_TESTS="$([[ "$SKIP_TESTS" == "ON" ]] && echo "OFF" || echo "ON")"
        -DBUILD_EXAMPLES="$([[ "$SKIP_EXAMPLES" == "ON" ]] && echo "OFF" || echo "ON")"
    )
    
    if [[ "$ENABLE_ALL" == "ON" ]]; then
        CMAKE_ARGS+=(
            -DENABLE_ETHERNET=ON
            -DENABLE_DDS=ON
            -DENABLE_TSN=ON
            -DENABLE_AUTOSAR_CLASSIC=ON
            -DENABLE_AUTOSAR_ADAPTIVE=ON
            -DENABLE_DDS_SECURITY=ON
            -DENABLE_DDS_ADVANCED=ON
            -DENABLE_DDS_RUNTIME=ON
        )
    else
        CMAKE_ARGS+=(-DENABLE_ETHERNET=ON -DENABLE_DDS=ON)
    fi
    
    cmake "${CMAKE_ARGS[@]}" "$PROJECT_DIR"
}

# Function to build project
build_project() {
    print_info "Building project..."
    
    local jobs=$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)
    print_info "Using $jobs parallel jobs"
    
    make -j"$jobs"
}

# Function to run tests
run_tests() {
    if [[ "$SKIP_TESTS" == "ON" ]]; then
        print_warn "Skipping tests as requested"
        return 0
    fi
    
    print_info "Running tests..."
    
    if ! ctest --output-on-failure; then
        print_warn "Some tests failed. Continuing with installation..."
    fi
}

# Function to install project
install_project() {
    print_info "Installing project to $INSTALL_PREFIX..."
    
    if [[ "$EUID" -ne 0 && "$INSTALL_PREFIX" == "/usr/local" ]]; then
        print_warn "Installing to system directory requires root privileges"
        print_info "Running: sudo make install"
        sudo make install
    else
        make install
    fi
    
    # Update library cache
    if command -v ldconfig &> /dev/null; then
        print_info "Updating library cache..."
        if [[ "$EUID" -ne 0 ]]; then
            sudo ldconfig
        else
            ldconfig
        fi
    fi
}

# Function to create environment setup script
create_env_script() {
    local env_script="$INSTALL_PREFIX/bin/eth_dds_env.sh"
    
    print_info "Creating environment setup script: $env_script"
    
    cat > "$env_script" << 'EOF'
#!/bin/bash
# ETH-DDS Environment Setup Script

export ETH_DDS_INSTALL_PREFIX="@INSTALL_PREFIX@"
export PATH="$ETH_DDS_INSTALL_PREFIX/bin:$PATH"
export LD_LIBRARY_PATH="$ETH_DDS_INSTALL_PREFIX/lib:$LD_LIBRARY_PATH"
export PKG_CONFIG_PATH="$ETH_DDS_INSTALL_PREFIX/lib/pkgconfig:$PKG_CONFIG_PATH"
export CMAKE_PREFIX_PATH="$ETH_DDS_INSTALL_PREFIX:$CMAKE_PREFIX_PATH"

# For development
export ETH_DDS_INCLUDE_DIR="$ETH_DDS_INSTALL_PREFIX/include"
export ETH_DDS_LIB_DIR="$ETH_DDS_INSTALL_PREFIX/lib"
EOF
    
    sed -i "s|@INSTALL_PREFIX@|$INSTALL_PREFIX|g" "$env_script"
    chmod +x "$env_script"
}

# Function to print installation summary
print_summary() {
    echo ""
    echo "========================================"
    print_info "Installation completed successfully!"
    echo "========================================"
    echo ""
    echo "Installation directory: $INSTALL_PREFIX"
    echo "Build type: $BUILD_TYPE"
    echo ""
    echo "To use ETH-DDS in your project:"
    echo "  1. Source the environment script:"
    echo "     source $INSTALL_PREFIX/bin/eth_dds_env.sh"
    echo ""
    echo "  2. Include the header in your code:"
    echo "     #include <eth_dds.h>"
    echo ""
    echo "  3. Link with the library:"
    echo "     gcc -o myapp myapp.c -leth_dds -lpthread"
    echo ""
    echo "For CMake projects, add:"
    echo "  find_package(eth_dds REQUIRED)"
    echo "  target_link_libraries(myapp eth_dds::eth_dds)"
    echo ""
}

# Main function
main() {
    echo "========================================"
    echo "ETH-DDS Integration Installation"
    echo "Version 2.0.0"
    echo "========================================"
    echo ""
    
    parse_args "$@"
    check_dependencies
    setup_build
    configure_project
    build_project
    run_tests
    install_project
    create_env_script
    print_summary
}

# Run main function
main "$@"
