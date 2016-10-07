samples := multicore pcie_shared_mem gpio
#Add above all samples directories
clean_samples = $(addprefix clean_,$(samples))
install_samples = $(addprefix install_,$(samples))

$(info $(clean_samples))
.PHONY: all clean $(samples) $(clean_samples)

define generate_build_rule
$(1):
	make -C ./$(1)
endef

define generate_clean_rule
clean_$(1):
	make -C ./$(1) clean
endef

define generate_install_rule
install_$(1):
	make -C ./$(1) install
endef

all: $(samples)
clean: $(clean_samples)
install: $(install_samples)


$(foreach sample, $(samples), $(eval $(call generate_build_rule,$(sample))))
$(foreach sample, $(samples), $(eval $(call generate_clean_rule,$(sample))))
$(foreach sample, $(samples), $(eval $(call generate_install_rule,$(sample))))


