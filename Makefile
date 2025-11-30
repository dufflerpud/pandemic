#@HDR@	$Id$
#@HDR@		Copyright 2024 by
#@HDR@		Christopher Caldwell/Brightsands
#@HDR@		P.O. Box 401, Bailey Island, ME 04003
#@HDR@		All Rights Reserved
#@HDR@
#@HDR@	This software comprises unpublished confidential information
#@HDR@	of Brightsands and may not be used, copied or made available
#@HDR@	to anyone, except in accordance with the license under which
#@HDR@	it is furnished.
PROJECTSDIR?=$(shell echo $(CURDIR) | sed -e 's+/projects/.*+/projects+')
PROGRAMS=pandemic pandaplot
PROGRAM=pandemic

include $(PROJECTSDIR)/common/Makefile.std

LDFLAGS+=-lm

CSRC=$(notdir $(subst .c,,$(wildcard $(SRCDIR)/*.c)))
OBJS=$(foreach f,$(CSRC),$(OBJDIR)/$(f).o)
TESTS=$(notdir $(subst .args,,$(wildcard tests/*.args)))
TEST_DATA=$(foreach t,$(TESTS),$(RESDIR)/$(t).data)

vars:
		@echo "Variables:"
		@echo "	TESTS			$(TESTS)"
		@echo "	TEST_DATA		$(TEST_DATA)"
		@echo " CSRC			$(CSRC)"
		@echo " OBJS			$(OBJS)"
		@make std_vars

$(BINDIR)/$(PROGRAM):	$(OBJS) $(BINDIR)/.must_exist
		$(CC) $(LDFLAGS) $(OBJS) -o $@

$(RESDIR)/%.data:	tests/%.args $(RESDIR) $(BINS)
		@sed -e 's/^/	/' $<
		$(BINDIR)/$(PROJECT) `cat $<` output_file=$@

$(RESDIR)/%.jpg:	$(RESDIR)/%.data
		@sed -e 's/^/	/' $<
		pandaplot -i $< -o $@

$(RESDIR)/%.pdf:	$(RESDIR)/%.data
		@sed -e 's/^/	/' $<
		pandaplot -i $< -o $@

$(RESDIR)/%.screen:	$(RESDIR)/%.data
		@sed -e 's/^/	/' $<
		pandaplot -i $< -o $@

test:		$(TEST_DATA)
		@: Do nothing

%.gdb:		tests/%.args $(BINS)
		echo "run " `cat $<` > /tmp/setclip
		setclip /tmp/setclip
		rm /tmp/setclip
		@sed -e 's/^/	/' $<
		gdb $(BINDIR)/$(PROJECT)
%:
		@echo "Invoking std_$@ rule:"
		@$(MAKE) std_$@ ORIGINAL_TARGET=$@
