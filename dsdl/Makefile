# DSDL Paths
PATHS := dronecan-dsdl/uavcan \
         dronecan-dsdl/dronecan \
         dronecan-dsdl/com \
         dronecan-dsdl/ardupilot \
         custom/warg

# System Python used to create the venv
SYS_PYTHON ?= $(shell \
	if command -v python3 >/dev/null 2>&1; then echo python3; \
	elif command -v python >/dev/null 2>&1; then echo python; \
	elif command -v py >/dev/null 2>&1; then echo py -3; \
	else echo python; fi)

VENV_DIR := .venv
DEPS_STAMP := $(VENV_DIR)/.deps

SCRIPT := dronecan-dsdlc/dronecan_dsdlc.py -O generated $(PATHS)

REQUIREMENTS := dronecan
REQUIREMENTS_FILE := dronecan-dsdlc/requirements.txt

# Find Python inside the venv when this variable is used
VENV_PYTHON = $(shell $(SYS_PYTHON) -c "from pathlib import Path; v=Path('$(VENV_DIR)'); candidates=[v/'Scripts'/'python.exe', v/'bin'/'python', v/'bin'/'python.exe']; print(next((str(p) for p in candidates if p.exists()), ''))")

all: setup build

setup: $(DEPS_STAMP)

build: setup
	@echo Generating headers from DSDL
	"$(VENV_PYTHON)" $(SCRIPT)

$(VENV_DIR):
	$(SYS_PYTHON) -m venv $(VENV_DIR)

$(DEPS_STAMP): $(REQUIREMENTS_FILE) | $(VENV_DIR)
	"$(VENV_PYTHON)" -m pip install -r $(REQUIREMENTS_FILE) $(REQUIREMENTS)
	$(SYS_PYTHON) -c "from pathlib import Path; Path(r'$(DEPS_STAMP)').touch()"

clean:
	$(SYS_PYTHON) -c "from pathlib import Path; import shutil; [shutil.rmtree(p, ignore_errors=True) for p in ['$(VENV_DIR)', 'generated']]"