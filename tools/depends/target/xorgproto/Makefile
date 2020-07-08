include ../../Makefile.include
DEPS= ../../Makefile.include Makefile

# lib name, version
LIBNAME=xorgproto
VERSION=2020.1
SOURCE=$(LIBNAME)-$(VERSION)
ARCHIVE=$(SOURCE).tar.bz2
DEPS += ../../Makefile.include
PYTHON = $(NATIVEPREFIX)/bin/python3
MESON = $(NATIVEPREFIX)/bin/meson
NINJA = $(NATIVEPREFIX)/bin/ninja

BASE_URL=https://xorg.freedesktop.org/archive/individual/proto

export PKG_CONFIG_LIBDIR=$(PREFIX)/lib/pkgconfig

# configuration settings
CONFIGURE=$(PYTHON) $(MESON) --prefix $(PREFIX)

CONFIGURE+= --cross-file $(PREFIX)/share/cross-file.meson

LIBDYLIB=$(PLATFORM)/lib/.libs/lib$(LIBNAME).a

all: .installed-$(PLATFORM)

$(TARBALLS_LOCATION)/$(ARCHIVE):
	cd $(TARBALLS_LOCATION); $(RETRIEVE_TOOL) $(RETRIEVE_TOOL_FLAGS) $(BASE_URL)/$(ARCHIVE)

$(PLATFORM): $(TARBALLS_LOCATION)/$(ARCHIVE) $(DEPS)
	rm -rf $(PLATFORM)/*; mkdir -p $(PLATFORM)
	cd $(PLATFORM); $(ARCHIVE_TOOL) $(ARCHIVE_TOOL_FLAGS) $(TARBALLS_LOCATION)/$(ARCHIVE)
	cd $(PLATFORM); rm -rf build; mkdir -p build
	cd $(PLATFORM); $(CONFIGURE) . build

$(LIBDYLIB): $(PLATFORM)
	cd $(PLATFORM)/build; $(NINJA) -v

.installed-$(PLATFORM): $(LIBDYLIB)
	cd $(PLATFORM)/build; $(NINJA) -v install
	touch $@

clean:
	$(MAKE) -C $(PLATFORM) clean
	rm -f .installed-$(PLATFORM)

distclean::
	rm -rf $(PLATFORM) .installed-$(PLATFORM)
