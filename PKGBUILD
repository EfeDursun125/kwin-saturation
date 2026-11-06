pkgname=kwin-saturation
pkgver=1.0.0
pkgrel=1
pkgdesc="KWin saturation boost effect for KDE Plasma 6"
arch=('x86_64')
url="https://github.com/EfeDursun125/kwin-saturation"
license=('GPL-3.0-or-later')
depends=(
    'kwin'
    'qt6-base'
    'qt6-svg'
    'vulkan-icd-loader'
)
makedepends=(
    'cmake'
    'extra-cmake-modules'
    'qt6-tools'
    'kconfig'
    'vulkan-headers'
    'ninja'
)

source=("$pkgname::git+$url.git")
sha256sums=('SKIP')
provides=('kwin-saturation')
conflicts=('kwin-saturation-git')

build() {
    cmake -B build -S "$srcdir/$pkgname" \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_INSTALL_PREFIX=/usr

    cmake --build build --parallel "$(nproc)"
}

package() {
    DESTDIR="$pkgdir" cmake --install build
}
