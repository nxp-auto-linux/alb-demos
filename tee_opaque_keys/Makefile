apps := host ta

# Accept any target, up to and including none
ifeq ($(MAKECMDGOALS),)
.PHONY: default
default:
else
$(MAKECMDGOALS):
endif
	$(foreach app, $(apps), make -C $(app) $(MAKECMDGOALS);)