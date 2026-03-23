#indx#	Makefile - Build and install pandemic simulation
#@HDR@	$Id$
#@HDR@
#@HDR@	Copyright (c) 2024-2026 Christopher Caldwell (Christopher.M.Caldwell0@gmail.com)
#@HDR@
#@HDR@	Permission is hereby granted, free of charge, to any person
#@HDR@	obtaining a copy of this software and associated documentation
#@HDR@	files (the "Software"), to deal in the Software without
#@HDR@	restriction, including without limitation the rights to use,
#@HDR@	copy, modify, merge, publish, distribute, sublicense, and/or
#@HDR@	sell copies of the Software, and to permit persons to whom
#@HDR@	the Software is furnished to do so, subject to the following
#@HDR@	conditions:
#@HDR@	
#@HDR@	The above copyright notice and this permission notice shall be
#@HDR@	included in all copies or substantial portions of the Software.
#@HDR@	
#@HDR@	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY
#@HDR@	KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
#@HDR@	WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE
#@HDR@	AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
#@HDR@	HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
#@HDR@	WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
#@HDR@	FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
#@HDR@	OTHER DEALINGS IN THE SOFTWARE.
#
#hist#	2026-02-19 - Christopher.M.Caldwell0@gmail.com - Created
########################################################################
#doc#	Makefile - Build and install pandemic simulation
########################################################################
PROJECTSDIR?=$(shell echo $(CURDIR) | sed -e 's+/projects/.*+/projects+')
PROGRAMS=pandemic pandaplot
PROGRAM=pandemic

include $(PROJECTSDIR)/common/Makefile.std

LDLIBS+=-lm

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
		@$(MAKE) std_vars

$(BINDIR)/$(PROGRAM):	$(OBJS) $(BINDIR)/.must_exist
		$(CC) $(LDFLAGS) $(OBJS) $(LDLIBS) -o $@

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
		@$(MAKE) ORIGINAL_TARGET=$@ std_$@
