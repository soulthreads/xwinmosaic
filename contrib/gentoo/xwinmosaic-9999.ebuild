# Copyright 1999-2012 Gentoo Foundation
# Distributed under the terms of the GNU General Public License v2
# $Header: $

EAPI=4

inherit git-2 cmake-utils

DESCRIPTION="X11 window switcher with fancy look."
HOMEPAGE="http://github.com/soulthreads/xwinmosaic"
SRC_URI=""

EGIT_REPO_URI="git://github.com/soulthreads/xwinmosaic.git"

LICENSE="BSD-2"
SLOT="0"
KEYWORDS="~x86 ~amd64"
IUSE="debug scripts"

RDEPEND="x11-libs/libX11
	=x11-libs/gtk+-2*"
DEPEND="${RDEPEND}
	virtual/pkgconfig"

DOCS="README.md"

src_unpack() {
    git-2_src_unpack
}

src_configure() {
    local mycmakeargs=(
	$(cmake-utils_use_with scripts SCRIPTS)
    )

    cmake-utils_src_configure
}
