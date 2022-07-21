# $Id$
# $URL$

ACLOCAL_AMFLAGS = -I m4 --install

SUBDIRS = src include docs example

EXTRA_DIST = bootstrap.sh AUTHORS TODO NEWS README.md mbt.pc codemeta.json

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = mbt.pc

ChangeLog: NEWS
	git pull; git2cl > ChangeLog

docker:
	docker build -t mbt:latest .

docker-dev:
	docker build -t mbt:dev --build-arg VERSION=development .

deps:
	./build-deps.sh
