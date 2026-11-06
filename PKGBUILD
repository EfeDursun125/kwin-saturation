pkgname=kwin-saturation
pkgver=1.0.0
pkgrel=2
pkgdesc="KWin saturation boost effect for KDE Plasma 6"
arch=('x86_64')
url="https://github.com/EfeDursun125/kwin-saturation"
license=('GPL-3.0-or-later')
depends=(
    'bash'
    'gcc-libs'
    'glibc'
    'kconfig'
    'kcoreaddons'
    'kwin>=6'
    'qt6-base'
    'qt6-svg'
    'qt6-tools'
)
makedepends=(
    'cmake'
    'extra-cmake-modules'
    'git'
    'vulkan-headers'
)

_commit=a65e2f92acea6a6dee082850e6e9a746210369b0
source=("$pkgname::git+$url.git#commit=$_commit")
sha256sums=('SKIP')
conflicts=('kwin-saturation-git')

prepare() {
    cd "$srcdir/$pkgname"

    # The upstream install script creates a desktop shortcut and updates host
    # caches, even with DESTDIR set. Package only the declared install files.
    local marker='# Copy launcher to User.s Desktop'
    grep -q "^$marker" CMakeLists.txt || return 1
    sed -i "/^$marker/,$ d" CMakeLists.txt
}

build() {
    cmake -B build -S "$srcdir/$pkgname" \
        -DCMAKE_BUILD_TYPE=None \
        -DCMAKE_INSTALL_PREFIX=/usr

    cmake --build build
}

package() {
    DESTDIR="$pkgdir" cmake --install build
}
