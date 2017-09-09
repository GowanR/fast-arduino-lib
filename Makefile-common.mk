#   Copyright 2016-2017 Jean-Francois Poilpret
#
#   Licensed under the Apache License, Version 2.0 (the "License");
#   you may not use this file except in compliance with the License.
#   You may obtain a copy of the License at
#
#       http://www.apache.org/licenses/LICENSE-2.0
#
#   Unless required by applicable law or agreed to in writing, software
#   distributed under the License is distributed on an "AS IS" BASIS,
#   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#   See the License for the specific language governing permissions and
#   limitations under the License.

# Makefile inspired by https://gist.github.com/maxtruxa/4b3929e118914ccef057f8a05c614b0f
# Adapted for FastArduino library and projects
#
# This is included by either Makefile-lib or Makefile-app
#

# Output directories
config:=$(VARIANT)-$(subst 000000UL,MHz,$(F_CPU))
objdir:=build/$(config)
depdir:=deps/$(config)
distdir:=dist/$(config)
target:=$(distdir)/$(TARGET)

# Libraries dependencies
ifeq ($(FASTARDUINO_ROOT),)
	FASTARDUINO_ROOT := .
	fastarduinolib:=
else
	fastarduinolib:=$(FASTARDUINO_ROOT)/dist/$(config)/libfastarduino.a
endif
libs:=$(fastarduinolib) $(ADDITIONAL_LIBS)

# Input directories
includes:=$(patsubst %,-I %,$(FASTARDUINO_ROOT)/cores $(ADDITIONAL_INCLUDES))

# List of source files
sources:=$(shell find $(SOURCE_ROOT) -name "*.cpp")
# List of generated files: objects and dependencies
objects:=$(patsubst %,$(objdir)/%.o,$(basename $(sources)))
deps:=$(patsubst %,$(depdir)/%.d,$(basename $(sources)))

# Create all needed subdirectories
$(shell mkdir -p $(dir $(objects)) >/dev/null)
$(shell mkdir -p $(dir $(deps)) >/dev/null)
$(shell mkdir -p $(distdir) >/dev/null)

# Environment
rm:=rm -f
ar:=ar
ranlib:=ranlib
cxx:=avr-g++
nm:=avr-nm
objcopy:=avr-objcopy
objdump:=avr-objdump
objsize:=avr-size

# Flags for compilation and build
cxxflags:=-mmcu=$(MCU) -DF_CPU=$(F_CPU) -D$(VARIANT) -DNO_ABI -fno-exceptions -Wextra -flto -std=gnu++11 -felide-constructors -Os -ffunction-sections -fdata-sections -mcall-prologues -g -Wall $(includes) -std=c++11 
ldflags = -mmcu=$(MCU) -DF_CPU=$(F_CPU) -D$(VARIANT) -fno-exceptions -Wextra -flto -std=gnu++11 -felide-constructors -Os -ffunction-sections -fdata-sections -mcall-prologues -Wl,--gc-sections -Wl,--relax -Wl,-Map,$@.map
depflags = -MT $@ -MMD -MP -MF $(depdir)/$*.Td

compile.cc = $(cxx) $(depflags) $(cxxflags) -c -o $@
precompile =
postcompile = mv -f $(depdir)/$*.Td $(depdir)/$*.d
link.o = $(cxx) $(ldflags) -o $@

# Flags for target programming (upload)
avrdude_options:=-p $(MCU) $(DUDE_OPTION) $(if $(DUDE_SERIAL),-P $(DUDE_SERIAL))

build: $(target)

.PHONY: clean
clean:
	$(rm) -r $(objdir) $(depdir) $(distdir)

.PHONY: help
help:
	@echo Available targets: build clean

# Target for generating dependencies
$(objdir)/%.o: %.cpp
$(objdir)/%.o: %.cpp $(depdir)/%.d
	$(precompile)
	$(compile.cc) $<
	$(postcompile)

.PRECIOUS = $(depdir)/%.d
$(depdir)/%.d: ;
