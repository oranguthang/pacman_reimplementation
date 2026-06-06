ROM        = rom/nes-starter-kit-example.nes
CNG        = node tools/create-nes-game/index.js
FCEUX      = tools/emulators/fceux/fceux.exe
POWERSHELL = powershell -NoProfile -ExecutionPolicy Bypass

build:
	$(CNG) build

start: build
	cmd /c start "" "$(FCEUX)" "$(ROM)"

run:
	cmd /c start "" "$(FCEUX)" "$(ROM)"

clean:
	$(CNG) clean

install-dev:
	$(POWERSHELL) -File tools/scripts/install-dev.ps1

split-rom:
	node tools/scripts/split-rom.js

.PHONY: build start run clean install-dev split-rom
