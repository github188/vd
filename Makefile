BUILD_TYPE = Release
#BUILD_TYPE = Debug

BUILDDIR = build

.PHONY: CMAKE

CMAKE:
	@if [ ! -d $(BUILDDIR) ]; then	\
		echo "$(BUILDDIR) not exist.";	\
		mkdir $(BUILDDIR);	\
	fi
	@cd $(BUILDDIR) && cmake -DCMAKE_BUILD_TYPE=$(BUILD_TYPE) ../

all:
	CMAKE
	@$(MAKE) -C $(BUILDDIR)

install:
	@$(MAKE) install -C $(BUILDDIR)

clean:
	rm -rf $(BUILDDIR)


