# seed-blink
Before you do anything. Run:
```
git submodule update --init --recursive
./rebuild_all.sh
```

in order to build/program:
hold down boot
hold down reset
release reset
release boot
use the vscode "build and program" task or:
```
make
make program-dfu
```

## Author

Shensley

## Description

seed-blinks the Seed's onboard LED at a constant rate.

[Source Code](https://github.com/electro-smith/DaisyExamples/tree/master/seed/seed-blink)
