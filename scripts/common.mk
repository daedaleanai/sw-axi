.RECIPEPREFIX = >
SHELL := bash
.ONESHELL:
.SHELLFLAGS := -eu -o pipefail -c
.DELETE_ON_ERROR:
MAKEFLAGS += --warn-undefined-variables
MAKEFLAGS += --no-builtin-rules
.DEFAULT_GOAL := all
MAKECMDGOALS ?= $(.DEFAULT_GOAL)

define code-format
for i in $^; do                               \
  if [[ $$i == *.sv ]]; then                  \
    verible-verilog-format --inplace $$i;     \
  elif [[ $$i == *.cc || $$i == *.hh ]]; then \
    clang-format -i $$i;                      \
  fi;                                         \
done
endef
