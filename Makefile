CXX ?= c++
CXXFLAGS ?= -std=c++17 -Wall -Wextra -pedantic -O2
CPPFLAGS ?=

BIN_DIR := bin
BUILD_DIR := build
SRC_DIR := src
PROGRAMS := wlp4scan wlp4parse wlp4type wlp4gen
BINARIES := $(addprefix $(BIN_DIR)/,$(PROGRAMS))
EXAMPLE_SOURCES := $(wildcard examples/*.wlp4)
EXAMPLE_OUTPUTS := $(patsubst examples/%.wlp4,$(BUILD_DIR)/examples/%.asm,$(EXAMPLE_SOURCES))
OUT ?= $(BUILD_DIR)/$(notdir $(basename $(INPUT))).asm

.DEFAULT_GOAL := all
.PHONY: all compile examples test clean help

all: $(BINARIES)

$(BIN_DIR):
	mkdir -p $@

$(BIN_DIR)/wlp4parse: $(SRC_DIR)/wlp4parse.cc $(SRC_DIR)/wlp4data.h | $(BIN_DIR)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $< -o $@

$(BIN_DIR)/%: $(SRC_DIR)/%.cc | $(BIN_DIR)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $< -o $@

compile: all
	@test -n "$(INPUT)" || { echo "usage: make compile INPUT=path/program.wlp4 [OUT=build/program.asm]" >&2; exit 2; }
	@./scripts/compile.sh "$(INPUT)" "$(OUT)"

examples: $(EXAMPLE_OUTPUTS)

$(BUILD_DIR)/examples/%.asm: examples/%.wlp4 $(BINARIES) scripts/compile.sh
	@./scripts/compile.sh "$<" "$@"

test: all
	@./tests/run-tests.sh

clean:
	rm -rf "$(BIN_DIR)" "$(BUILD_DIR)"

help:
	@echo "make                         Build all four compiler stages in bin/"
	@echo "make compile INPUT=file.wlp4 Compile one program (optional OUT=file.asm)"
	@echo "make examples                Compile every example into build/examples/"
	@echo "make test                    Run the regression suite"
	@echo "make clean                   Remove generated bin/ and build/ directories"
	@echo "Variables: CXX, CPPFLAGS, CXXFLAGS, INPUT, OUT"
