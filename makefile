PROJECTS=kernel memoria swamp swamp-test tests carpincho_dummy carpincho_dummy_2 carpincho_dummy_3 carpincho_dummy_4

LIBS=matelib static

all: $(PROJECTS)

$(PROJECTS): $(LIBS)
	$(MAKE) -C $@ all

$(LIBS):
	$(MAKE) -C $@ all


clean:
	$(foreach P, $(LIBS) $(PROJECTS), $(MAKE) -C $P clean;)

release:
	$(foreach P, $(LIBS) $(PROJECTS), $(MAKE) -C $P release;)

.PHONY: all $(PROJECTS) $(LIBS) clean release