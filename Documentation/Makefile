# Guard against environment variables
MAN8_TXT =

# Top level commands
MAN8_TXT += btrfs.txt
MAN8_TXT += btrfs-convert.txt
MAN8_TXT += btrfs-debug-tree.txt
MAN8_TXT += btrfs-find-root.txt
MAN8_TXT += btrfs-image.txt
MAN8_TXT += btrfs-map-logical.txt
MAN8_TXT += btrfs-show-super.txt
MAN8_TXT += btrfstune.txt
MAN8_TXT += btrfs-zero-log.txt
MAN8_TXT += fsck.btrfs.txt
MAN8_TXT += mkfs.btrfs.txt

# Sub commands for btrfs
MAN8_TXT += btrfs-subvolume.txt
MAN8_TXT += btrfs-filesystem.txt
MAN8_TXT += btrfs-balance.txt
MAN8_TXT += btrfs-device.txt
MAN8_TXT += btrfs-scrub.txt
MAN8_TXT += btrfs-check.txt
MAN8_TXT += btrfs-rescue.txt
MAN8_TXT += btrfs-inspect-internal.txt
MAN8_TXT += btrfs-send.txt
MAN8_TXT += btrfs-receive.txt
MAN8_TXT += btrfs-quota.txt
MAN8_TXT += btrfs-qgroup.txt
MAN8_TXT += btrfs-replace.txt
MAN8_TXT += btrfs-restore.txt
MAN8_TXT += btrfs-property.txt

# Mount manpage
MAN5_TXT += btrfs-mount.txt

MAN_TXT = $(MAN8_TXT) $(MAN5_TXT)
MAN_XML = $(patsubst %.txt,%.xml,$(MAN_TXT))

DOC_MAN5 = $(patsubst %.txt,%.5,$(MAN5_TXT))
GZ_MAN5 = $(patsubst %.txt,%.5.gz,$(MAN5_TXT))

DOC_MAN8 = $(patsubst %.txt,%.8,$(MAN8_TXT))
GZ_MAN8 = $(patsubst %.txt,%.8.gz,$(MAN8_TXT))

mandir ?= $(prefix)/share/man
man8dir = $(mandir)/man8
man5dir = $(mandir)/man5

ASCIIDOC = asciidoc
ASCIIDOC_EXTRA =
MANPAGE_XSL = manpage-normal.xsl
XMLTO = xmlto
XMLTO_EXTRA =
XMLTO_EXTRA = -m manpage-bold-literal.xsl
GZIPCMD = gzip
INSTALL ?= install
RM ?= rm -f
LNS ?= ln -sf
BTRFS_VERSION = $(shell sed -n 's/.*BTRFS_BUILD_VERSION "Btrfs \(.*\)"/\1/p'\
		  ../version.h)

ifneq ($(findstring $(MAKEFLAGS),s),s)
ifndef V
	QUIET_RM	= @
	QUIET_ASCIIDOC	= @echo "    [ASCII]  $@";
	QUIET_XMLTO	= @echo "    [XMLTO]  $@";
	QUIET_GZIP	= @echo "    [GZ]     $@";
	QUIET_STDERR	= 2> /dev/null
	QUIET_SUBDIR0	= +@subdir=
	QUIET_SUBDIR1	= ;$(NO_SUBDIR) echo '   ' SUBDIR $$subdir; \
			  $(MAKE) $(PRINT_DIR) -C $$subdir
	export V
endif
endif

all: man
man: man5 man8
man5: $(GZ_MAN5)
man8: $(GZ_MAN8)

install: install-man

install-man: man
	$(INSTALL) -d -m 755 $(DESTDIR)$(man5dir)
	$(INSTALL) -d -m 755 $(DESTDIR)$(man8dir)
	$(INSTALL) -m 644 $(GZ_MAN5) $(DESTDIR)$(man5dir)
	# the source file name of btrfs.5 clashes with section 8 page, but we
	# want to keep the code generic
	mv $(DESTDIR)$(man5dir)/btrfs-mount.5.gz $(DESTDIR)$(man5dir)/btrfs.5.gz
	$(INSTALL) -m 644 $(GZ_MAN8) $(DESTDIR)$(man8dir)
	$(LNS) btrfs-check.8.gz $(DESTDIR)$(man8dir)/btrfsck.8.gz

uninstall:
	cd $(DESTDIR)$(man8dir); rm -f btrfs-check.8.gz $(GZ_MAN8)
	rmdir -p --ignore-fail-on-non-empty $(DESTDIR)$(man8dir)

clean:
	$(QUIET_RM)$(RM) *.xml *.xml+ *.5 *.5.gz *.8 *.8.gz

%.5.gz : %.5
	$(QUIET_GZIP)$(GZIPCMD) -n -c $< > $@

%.8.gz : %.8
	$(QUIET_GZIP)$(GZIPCMD) -n -c $< > $@

%.5 : %.xml
	$(QUIET_XMLTO)$(RM) $@ && \
	$(XMLTO) -m $(MANPAGE_XSL) $(XMLTO_EXTRA) man $<

%.8 : %.xml
	$(QUIET_XMLTO)$(RM) $@ && \
	$(XMLTO) -m $(MANPAGE_XSL) $(XMLTO_EXTRA) man $<

%.xml : %.txt asciidoc.conf
	$(QUIET_ASCIIDOC)$(RM) $@.tmp[12] $@ && \
	sed -e "s/\(<[^>]\+>\)/'\1'/g" < $< > $@.tmp1 && \
	$(ASCIIDOC) -b docbook -d manpage -f asciidoc.conf \
		$(ASCIIDOC_EXTRA) -abtrfs_version=$(BTRFS_VERSION) \
		-o $@.tmp2 $@.tmp1 && \
	mv $@.tmp2 $@ && \
	rm -f -- $@.tmp1
