Virt-Viewer Message Translation
===========================

Virt-Viewer translatable messages are maintained using the GNU Gettext tools
and file formats, in combination with the Zanata web service.

Source repository
=================

The virt-viewer GIT repository stores the master "virt-viewer.pot" file and
full "po" files for translations. The master "virt-viewer.pot" file can be
re-generated using

   make virt-viewer.pot

The full po files can have their source locations and msgids updated using

   make update-po

Normally these updates are only done when either refreshing translations from
Zanata, or when creating a new release.

Zanata web service
==================

The translation of virt-viewer messages has been outsourced to the Fedora
translation team using the Zanata web service:

  https://fedora.zanata.org/project/view/virt-viewer

As such, changes to translations will generally NOT be accepted as patches
directly to virt-viewer GIT. Any changes made to "$LANG.mini.po" files in
virt-viewer GIT will be overwritten and lost the next time content is imported
from Zanata.

The master "virt-viewer.pot" file is periodically pushed to Zanata to provide
the translation team with content changes. New translated text is then
periodically pulled down from Zanata to update the po files.
