# Maintainer targets. Students use the Arduino IDE and do not need this file.
#
#   make sync                 copy common/rt.h into every sketch folder
#   make compile              compile every sketch (no board needed)
#   make tutor                rebuild tutor/tutor_bundle.md (also done by compile)
#   make hil PORT=/dev/...    flash every sketch to a board and check its verdicts
#   make hil PORT=... ONLY="race inversion"

PORT ?=
ONLY ?=

.PHONY: sync tutor compile hil clean

sync:
	tools/sync_common.sh

tutor:
	tools/make_tutor_bundle.sh

compile: sync tutor
	tools/compile_all.sh

hil: sync
	@test -n "$(PORT)" || { echo "usage: make hil PORT=/dev/cu.usbmodemXXXX"; exit 1; }
	python3 tools/hil_smoke.py --port $(PORT) $(if $(ONLY),--only $(ONLY))

clean:
	rm -rf .arduino-user hil_smoke.log
