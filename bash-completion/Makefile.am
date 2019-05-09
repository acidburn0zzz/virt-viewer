EXTRA_DIST = \
	$(PACKAGE)

install-data-local: install-bash-completion

uninstall-local: uninstall-bash-completion

if WITH_BASH_COMPLETION
install-bash-completion:
	$(MKDIR_P) "$(DESTDIR)$(BASH_COMPLETIONS_DIR)"
	$(INSTALL_SCRIPT) $(srcdir)/$(PACKAGE) \
		"$(DESTDIR)$(BASH_COMPLETIONS_DIR)/$(PACKAGE)"

uninstall-bash-completion:
	rm -f $(DESTDIR)$(BASH_COMPLETIONS_DIR)/$(PACKAGE)
	rmdir $(DESTDIR)$(BASH_COMPLETIONS_DIR) ||:
else ! WITH_BASH_COMPLETION
install-bash-completion:
uninstall-bash-completion:
endif ! WITH_BASH_COMPLETION
