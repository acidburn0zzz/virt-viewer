NULL =
CLEANFILES =
MANUFACTURER = Virt Manager Project

EXTRA_DIST =					\
	virt-viewer.wxs.in			\
	gettext/its/mime.its			\
	gettext/its/mime.loc			\
	$(NULL)

PO_FILES = $(wildcard $(srcdir)/../po/*.po)

# this make sure those files are regenerated when they change
# (in maintainer-mode)
all-local: virt-viewer.wxs

if OS_WIN32

if HAVE_LIBVIRT
HaveLibvirt = True
else
HaveLibvirt = False
endif

if HAVE_OVIRT
HaveOVirt = True
else
HaveOVirt = False
endif

if HAVE_SPICE_GTK
HaveSpiceGtk = True
else
HaveSpiceGtk = False
endif

if HAVE_GTK_VNC
HaveGtkVnc = True
else
HaveGtkVnc = False
endif

buildenv.txt:
	$(AM_V_GEN)rpm -qa | sort | unix2dos > $@

virt-viewer-$(WIXL_ARCH)-$(VERSION).msi: virt-viewer.wxs buildenv.txt
	$(AM_V_GEN)DESTDIR=`mktemp -d` &&				\
	make -C $(top_builddir) install DESTDIR=$$DESTDIR >/dev/null &&	\
	find $$DESTDIR | wixl-heat -p $$DESTDIR$(prefix)/		\
	    --component-group CG.virt-viewer --var var.DESTDIR		\
	    --directory-ref=INSTALLDIR > virt-viewer-files.wxs &&	\
	MANUFACTURER="$(MANUFACTURER)" wixl -D SourceDir=$(prefix)	\
             -D DESTDIR=$$DESTDIR$(prefix)				\
             -D HaveSpiceGtk=$(HaveSpiceGtk)				\
             -D HaveGtkVnc=$(HaveGtkVnc)				\
             -D HaveLibvirt=$(HaveLibvirt)				\
             -D HaveOVirt=$(HaveOVirt)					\
             --arch $(WIXL_ARCH)					\
             -o $@							\
	     $< virt-viewer-files.wxs &&				\
	rm -rf $$DESTDIR virt-viewer-files.wxs

msi: virt-viewer-$(WIXL_ARCH)-$(VERSION).msi

CLEANFILES +=					\
	buildenv.txt				\
	virt-viewer-$(WIXL_ARCH)-$(VERSION).msi	\
	$(NULL)

else #!WIN32

desktopdir       = $(datadir)/applications
DESKTOPFILES     = remote-viewer.desktop.in
desktop_DATA     = $(DESKTOPFILES:.desktop.in=.desktop)
%.desktop: %.desktop.in $(PO_FILES)
	$(AM_V_GEN)$(MSGFMT) --desktop --template $< -d $(top_srcdir)/po -o $@

MIMEFILES = virt-viewer-mime.xml.in
mimedir = $(datadir)/mime/packages
mime_DATA = virt-viewer-mime.xml
%-mime.xml: %-mime.xml.in $(PO_FILES)
	$(AM_V_GEN)GETTEXTDATADIR=$(srcdir)/gettext $(MSGFMT) --xml --template $< -d $(top_srcdir)/po -o $@

appdatadir       = $(datadir)/appdata
APPDATAFILES     = remote-viewer.appdata.xml.in
appdata_DATA     = $(APPDATAFILES:.xml.in=.xml)
%.appdata.xml: %.appdata.xml.in $(PO_FILES)
	$(AM_V_GEN)$(MSGFMT) --xml --template $< -d $(top_srcdir)/po -o $@

install-data-hook:
if ENABLE_UPDATE_MIMEDB
	@if $(AM_V_P); then set -x; else echo "  INSTALL  update-mime-database"; fi; \
	$(UPDATE_MIME_DATABASE) "$(DESTDIR)$(datadir)/mime";
endif

uninstall-hook:
if ENABLE_UPDATE_MIMEDB
	@if $(AM_V_P); then set -x; else echo "  UNINSTALL update-mime-database"; fi; \
	$(UPDATE_MIME_DATABASE) "$(DESTDIR)$(datadir)/mime";
endif

CLEANFILES += $(mime_DATA) $(desktop_DATA) $(appdata_DATA)
EXTRA_DIST += $(MIMEFILES) $(DESKTOPFILES) $(APPDATAFILES)

endif

-include $(top_srcdir)/git.mk
