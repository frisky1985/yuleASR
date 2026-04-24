#!/bin/bash
#
# ETH-DDS Integration Packaging Script
# Version: 2.0.0
#

set -e

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

# Configuration
PACKAGE_NAME="eth-dds-integration"
PACKAGE_VERSION="2.0.0"
PACKAGE_RELEASE="1"
OUTPUT_DIR="$(pwd)/packages"
BUILD_DIR="$(pwd)/build"

# Print functions
print_info() {
    echo -e "${GREEN}[INFO]${NC} $1"
}

print_warn() {
    echo -e "${YELLOW}[WARN]${NC} $1"
}

print_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

print_step() {
    echo -e "${BLUE}[STEP]${NC} $1"
}

# Show help
show_help() {
    cat << EOF
ETH-DDS Integration Packaging Script

Usage: $0 [OPTIONS] [TARGETS]

Targets:
    debian          Create Debian package (.deb)
    rpm             Create RPM package (.rpm)
    tarball         Create source tarball (.tar.gz)
    zip             Create ZIP archive (.zip)
    docker          Create Docker image
    all             Create all packages

Options:
    --version VER   Set package version (default: $PACKAGE_VERSION)
    --release REL   Set package release (default: $PACKAGE_RELEASE)
    --output DIR    Set output directory (default: $OUTPUT_DIR)
    --help          Show this help message

Examples:
    $0 all                      # Create all packages
    $0 debian rpm               # Create Debian and RPM packages
    $0 --version 2.1.0 tarball  # Create tarball with specific version
EOF
}

# Parse arguments
parse_args() {
    local targets=()
    
    while [[ $# -gt 0 ]]; do
        case $1 in
            --version)
                PACKAGE_VERSION="$2"
                shift 2
                ;;
            --release)
                PACKAGE_RELEASE="$2"
                shift 2
                ;;
            --output)
                OUTPUT_DIR="$2"
                shift 2
                ;;
            --help)
                show_help
                exit 0
                ;;
            debian|rpm|tarball|zip|docker|all)
                targets+=("$1")
                shift
                ;;
            *)
                print_error "Unknown option or target: $1"
                show_help
                exit 1
                ;;
        esac
    done
    
    if [[ ${#targets[@]} -eq 0 ]]; then
        print_error "No targets specified"
        show_help
        exit 1
    fi
    
    echo "${targets[@]}"
}

# Setup directories
setup_dirs() {
    print_step "Setting up directories..."
    
    mkdir -p "$OUTPUT_DIR"
    mkdir -p "$BUILD_DIR"
    
    print_info "Output directory: $OUTPUT_DIR"
    print_info "Build directory: $BUILD_DIR"
}

# Build project
build_project() {
    print_step "Building project..."
    
    cd "$BUILD_DIR"
    
    cmake .. \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_INSTALL_PREFIX=/usr \
        -DBUILD_EXAMPLES=OFF \
        -DBUILD_TESTS=OFF \
        -DENABLE_ALL_FEATURES=ON
    
    make -j$(nproc)
    
    print_info "Build completed"
}

# Create Debian package
create_deb() {
    print_step "Creating Debian package..."
    
    local pkg_name="${PACKAGE_NAME}_${PACKAGE_VERSION}-${PACKAGE_RELEASE}_amd64"
    local pkg_dir="$BUILD_DIR/$pkg_name"
    
    # Clean and create package directory
    rm -rf "$pkg_dir"
    mkdir -p "$pkg_dir/DEBIAN"
    mkdir -p "$pkg_dir/usr"
    
    # Install to package directory
    cd "$BUILD_DIR"
    make DESTDIR="$pkg_dir" install
    
    # Create control file
    cat > "$pkg_dir/DEBIAN/control" << EOF
Package: $PACKAGE_NAME
Version: $PACKAGE_VERSION-$PACKAGE_RELEASE
Section: libs
Priority: optional
Architecture: amd64
Depends: libc6 (>= 2.17), libpthread-stubs0-dev
Maintainer: ETH-DDS Team <support@eth-dds.org>
Description: ETH-DDS Integration Library
 High-performance data communication framework for automotive
 and industrial automation, integrating Ethernet, DDS middleware,
 TSN, and AUTOSAR standards.
EOF
    
    # Create postinst script
    cat > "$pkg_dir/DEBIAN/postinst" << 'EOF'
#!/bin/bash
ldconfig
exit 0
EOF
    chmod 755 "$pkg_dir/DEBIAN/postinst"
    
    # Build package
    dpkg-deb --build "$pkg_dir"
    
    # Move to output
    mv "${pkg_dir}.deb" "$OUTPUT_DIR/"
    
    print_info "Debian package created: ${pkg_name}.deb"
}

# Create RPM package
create_rpm() {
    print_step "Creating RPM package..."
    
    local rpm_build="$BUILD_DIR/rpmbuild"
    mkdir -p "$rpm_build"/{BUILD,RPMS,SOURCES,SPECS,SRPMS}
    
    # Create spec file
    cat > "$rpm_build/SPECS/$PACKAGE_NAME.spec" << EOF
Name:           $PACKAGE_NAME
Version:        $PACKAGE_VERSION
Release:        $PACKAGE_RELEASE%{?dist}
Summary:        ETH-DDS Integration Library

License:        Apache-2.0
URL:            https://github.com/eth-dds/eth-dds-integration
Source0:        %{name}-%{version}.tar.gz

BuildRequires:  cmake >= 3.14, gcc, make
Requires:       glibc >= 2.17

%description
High-performance data communication framework for automotive
and industrial automation, integrating Ethernet, DDS middleware,
TSN, and AUTOSAR standards.

%prep
%setup -q

%build
cmake . -DCMAKE_BUILD_TYPE=Release -DBUILD_EXAMPLES=OFF -DBUILD_TESTS=OFF
make %{?_smp_mflags}

%install
make install DESTDIR=%{buildroot}

%post
/sbin/ldconfig

%files
/usr/include/eth_dds/
/usr/lib/libeth_dds.so*
/usr/lib/cmake/eth_dds/

%changelog
* $(date '+%a %b %d %Y') ETH-DDS Team <support@eth-dds.org> - $PACKAGE_VERSION-$PACKAGE_RELEASE
- Initial RPM package
EOF
    
    # Create source tarball
    cd "$(dirname "$BUILD_DIR")"
    tar czf "$rpm_build/SOURCES/${PACKAGE_NAME}-${PACKAGE_VERSION}.tar.gz" \
        --exclude='build' --exclude='.git' --exclude='packages' \
        -C "$(dirname "$BUILD_DIR")" "$(basename "$(dirname "$BUILD_DIR")")"
    
    # Build RPM
    rpmbuild --define "_topdir $rpm_build" -ba "$rpm_build/SPECS/$PACKAGE_NAME.spec"
    
    # Copy RPM to output
    find "$rpm_build/RPMS" -name "*.rpm" -exec cp {} "$OUTPUT_DIR/" \;
    
    print_info "RPM package created"
}

# Create source tarball
create_tarball() {
    print_step "Creating source tarball..."
    
    local tarball_name="${PACKAGE_NAME}-${PACKAGE_VERSION}.tar.gz"
    local project_dir="$(dirname "$BUILD_DIR")"
    
    cd "$project_dir"
    
    tar czf "$OUTPUT_DIR/$tarball_name" \
        --exclude='build' --exclude='.git' --exclude='packages' \
        --exclude='*.o' --exclude='*.a' --exclude='*.so' \
        -C "$(dirname "$project_dir")" "$(basename "$project_dir")"
    
    print_info "Source tarball created: $tarball_name"
}

# Create ZIP archive
create_zip() {
    print_step "Creating ZIP archive..."
    
    local zip_name="${PACKAGE_NAME}-${PACKAGE_VERSION}.zip"
    local project_dir="$(dirname "$BUILD_DIR")"
    
    cd "$project_dir"
    
    zip -r "$OUTPUT_DIR/$zip_name" . \
        -x "build/*" -x ".git/*" -x "packages/*" \
        -x "*.o" -x "*.a" -x "*.so"
    
    print_info "ZIP archive created: $zip_name"
}

# Create Docker image
create_docker() {
    print_step "Creating Docker image..."
    
    local project_dir="$(dirname "$BUILD_DIR")"
    local docker_dir="$project_dir/scripts/docker"
    
    if [[ ! -f "$docker_dir/Dockerfile" ]]; then
        print_error "Dockerfile not found at $docker_dir/Dockerfile"
        return 1
    fi
    
    cd "$project_dir"
    
    docker build \
        -f "$docker_dir/Dockerfile" \
        -t "eth-dds:${PACKAGE_VERSION}" \
        -t "eth-dds:latest" \
        .
    
    # Save image to tar
    docker save "eth-dds:${PACKAGE_VERSION}" | gzip > "$OUTPUT_DIR/eth-dds-${PACKAGE_VERSION}-docker.tar.gz"
    
    print_info "Docker image created: eth-dds:${PACKAGE_VERSION}"
}

# Create checksums
create_checksums() {
    print_step "Creating checksums..."
    
    cd "$OUTPUT_DIR"
    
    # Create MD5 checksums
    md5sum * > checksums.md5
    
    # Create SHA256 checksums
    sha256sum * > checksums.sha256
    
    print_info "Checksums created"
}

# Print summary
print_summary() {
    echo ""
    echo "========================================"
    print_info "Packaging completed!"
    echo "========================================"
    echo ""
    echo "Output directory: $OUTPUT_DIR"
    echo ""
    echo "Generated packages:"
    ls -lh "$OUTPUT_DIR" | tail -n +2
    echo ""
}

# Main function
main() {
    echo "========================================"
    echo "ETH-DDS Integration Packaging"
    echo "Version $PACKAGE_VERSION"
    echo "========================================"
    echo ""
    
    local targets=($(parse_args "$@"))
    
    setup_dirs
    
    # Build only if needed
    local need_build=0
    for target in "${targets[@]}"; do
        case $target in
            debian|rpm|all)
                need_build=1
                ;;
        esac
    done
    
    if [[ $need_build -eq 1 ]]; then
        build_project
    fi
    
    # Create packages
    for target in "${targets[@]}"; do
        case $target in
            all)
                create_tarball
                create_zip
                command -v dpkg-deb &>/dev/null && create_deb || print_warn "dpkg-deb not found, skipping DEB"
                command -v rpmbuild &>/dev/null && create_rpm || print_warn "rpmbuild not found, skipping RPM"
                command -v docker &>/dev/null && create_docker || print_warn "docker not found, skipping Docker"
                ;;
            debian)
                command -v dpkg-deb &>/dev/null && create_deb || print_error "dpkg-deb not found"
                ;;
            rpm)
                command -v rpmbuild &>/dev/null && create_rpm || print_error "rpmbuild not found"
                ;;
            tarball)
                create_tarball
                ;;
            zip)
                create_zip
                ;;
            docker)
                command -v docker &>/dev/null && create_docker || print_error "docker not found"
                ;;
        esac
    done
    
    create_checksums
    print_summary
}

main "$@"
