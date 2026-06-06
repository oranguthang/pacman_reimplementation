const fs = require('fs');
const path = require('path');

const repoRoot = path.resolve(__dirname, '..', '..');
const romName = 'Pac-Man (J) (V1.0) [!].nes';
const romPath = path.join(repoRoot, romName);
const outputDir = path.join(repoRoot, 'temp', 'pacman_rom_split');

function ensureDir(dirPath) {
  fs.mkdirSync(dirPath, { recursive: true });
}

function writeFile(name, data) {
  fs.writeFileSync(path.join(outputDir, name), data);
}

if (!fs.existsSync(romPath)) {
  console.error(`ROM not found: ${romPath}`);
  console.error(`Place ${romName} in the repository root, then run "make split-rom".`);
  process.exit(1);
}

const rom = fs.readFileSync(romPath);
if (rom.length < 16 || rom[0] !== 0x4e || rom[1] !== 0x45 || rom[2] !== 0x53 || rom[3] !== 0x1a) {
  console.error('Input file is not a valid iNES ROM.');
  process.exit(1);
}

const prgBanks = rom[4];
const chrBanks = rom[5];
const flags6 = rom[6];
const flags7 = rom[7];
const hasTrainer = (flags6 & 0x04) !== 0;
const trainerSize = hasTrainer ? 512 : 0;
const headerSize = 16;
const prgSize = prgBanks * 16 * 1024;
const chrSize = chrBanks * 8 * 1024;
const expectedSize = headerSize + trainerSize + prgSize + chrSize;

if (rom.length < expectedSize) {
  console.error(`ROM is truncated. Expected at least ${expectedSize} bytes, got ${rom.length}.`);
  process.exit(1);
}

const trainerStart = headerSize;
const prgStart = trainerStart + trainerSize;
const chrStart = prgStart + prgSize;

ensureDir(outputDir);

writeFile('header.bin', rom.subarray(0, headerSize));
if (hasTrainer) {
  writeFile('trainer.bin', rom.subarray(trainerStart, prgStart));
}
writeFile('prg.bin', rom.subarray(prgStart, chrStart));
writeFile('chr.bin', rom.subarray(chrStart, chrStart + chrSize));

const metadata = {
  romName,
  sourcePath: romPath,
  outputDir,
  size: rom.length,
  prgBanks,
  chrBanks,
  mapper: ((flags7 & 0xf0) | (flags6 >> 4)),
  mirroring: (flags6 & 0x01) ? 'vertical' : 'horizontal',
  hasBattery: (flags6 & 0x02) !== 0,
  hasTrainer,
  hasFourScreenVram: (flags6 & 0x08) !== 0,
};

writeFile('metadata.json', Buffer.from(`${JSON.stringify(metadata, null, 2)}\n`, 'utf8'));

console.log(`Split ${romName} into ${outputDir}`);
