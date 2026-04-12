# DSDL Paths
PATHS := dronecan-dsdl/uavcan \
         dronecan-dsdl/dronecan \
         dronecan-dsdl/com \
		 dronecan-dsdl/ardupilot \
		 custom/warg

# System Python used to *create* the venv (can be overridden: make SYS_PYTHON=python3)
SYS_PYTHON ?= python

VENV_PYTHON := .venv/Scripts/python
VENV_PIP    := .venv/Scripts/pip

ROOT := $(CURDIR)
VENV_DIR := .venv
DEPS_STAMP := $(VENV_DIR)/.deps
SCRIPT := $(ROOT)/dronecan-dsdlc/dronecan_dsdlc.py -O generated $(PATHS)
REQUIREMENTS := dronecan empy pexpect

all: setup build

# Default: set up env if needed, then run the script
build: 
	@echo "Generating headers from DSDL"
	$(VENV_PYTHON) $(SCRIPT)

# Only create / update the environment if necessary
setup: $(DEPS_STAMP)

# Create the venv if it doesn't exist
$(VENV_DIR):
	$(SYS_PYTHON) -m venv $(VENV_DIR)

# Install requirements if they changed (or if this is first time)
$(DEPS_STAMP): | $(VENV_DIR)
	$(VENV_PIP) install $(REQUIREMENTS)
	$(VENV_PYTHON) -c "from pathlib import Path; Path('$(DEPS_STAMP)').touch()"

# Optional: remove venv completely
clean:
	@echo "Removing virtual environment..."
	@if [ -d "$(VENV_DIR)" ]; then rm -rf "$(VENV_DIR)"; fi
	rm -rf generated
