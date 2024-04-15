CFG_TEE_TA_LOG_LEVEL ?= 4

# The UUID for the Trusted Application
BINARY=5fe52085-5121-4cdf-b800-4964b8dd0c94

-include $(TA_DEV_KIT_DIR)/mk/ta_dev_kit.mk

ifeq ($(wildcard $(TA_DEV_KIT_DIR)/mk/ta_dev_kit.mk), )
clean:
	@echo 'Note: $$(TA_DEV_KIT_DIR)/mk/ta_dev_kit.mk not found, cannot clean TA'
	@echo 'Note: TA_DEV_KIT_DIR=$(TA_DEV_KIT_DIR)'
endif

# TAs usually reside in a separate folder from normal world applications.
# A standard tee-supplicant is configured to load TAs from /lib/optee_armtz.
# Have a separate variable to store this path in case it's needed.
ifeq ($(TA_INSTALLDIR),)
TA_INSTALLDIR = $(INSTALLDIR)
endif

# TAs are built different (with the help of ta_dev_kit.mk). They don't just
# resort to being 'common.mk'. Still, we want to configure an installation rule
# for convenience and consistency with the other apps.
ifeq ($(TA_INSTALLDIR),)
install:
	$(info >> TA_INSTALLDIR not set, skipping install.)
else
install: all
# Livin' on the edge: if $(TA_INSTALLDIR) is a file, remove it; otherwise,
# we'll just add to a pre-existing folder.
	if [ -f $(TA_INSTALLDIR) ]; then rm -f $(TA_INSTALLDIR); fi
	mkdir -p $(TA_INSTALLDIR)
	install -v $(BINARY).ta  $(TA_INSTALLDIR)
endif
