# Maintainer: Hubert Dej <hubert.dej@student.uj.edu.pl>
# Maintainer: Katzper Michno  <katzper.michno@student.uj.edu.pl>
# Maintainer: Maciej Matys <maciej.matys@student.uj.edu.pl>
# Maintainer: Tomasz Mazur <tomasz.mazur@student.uj.edu.pl>
pkgname=debugger
pkgver=1.0.0
pkgrel=1
pkgdesc="Tool for tracing program output and creating structured logs using plain text or html."
arch=('any')
url="https://github.com/Student-Team-Projects/Debugger.git"
license=('GPL3')
depends=('bpf' 'sysdig')
makedepends=('clang' 'compiler-rt' 'llvm' 'llvm-libs' 'spdlog' 'tclap' 'fmt')
source=("$pkgname-$pkgver.zip::https://github.com/Student-Team-Projects/$pkgname/archive/refs/heads/main.zip")
md5sums=("SKIP")
noextract=()

build() {
	cd "$pkgname-main"
	make
}

package() {
	cd "$pkgname-main"
	make DESTDIR="$pkgdir" PKGNAME="$pkgname" install
}
