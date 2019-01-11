ATTO_BASEDIR=.
include atto.mk

EXAMPLES = app tri cube fb tribench

EXAMPLES_SOURCES = $(EXAMPLES:%=examples/%.c)
EXAMPLES_EXECUTABLES = $(EXAMPLES:%=$(OBJDIR)/examples/%)
EXAMPLE_OBJS = $(EXAMPLE_SOURCES:%=$(OBJDIR)/%.o)

$(OBJDIR)/examples/%: $(ATTO_OBJS) $(OBJDIR)/examples/%.c.o
	@mkdir -p $(dir $@)
	$(CC) $(LIBS) $^ -o $@

examples: $(EXAMPLES_EXECUTABLES)

all: examples
