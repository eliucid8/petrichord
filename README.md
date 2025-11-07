# petrichord
Authors: Raquel Cervantes-Espinosa, Annabel Lee, Sam Ledden, and Eric Liu

# getting started
after cloning:
`git submodule update --init --recursive`

## project structure
This monorepo contains the code for both the rpi and the seed.
`cd` into the respective directories in order to build for each microcontroller.
If you are building for the pico, please switch your vscode workspace to be based in the pico directory. (ctrl+k+o)

## pico
use the rpi pico extension to create a new project in the pico folder. The .vscode and build folder are essential to building but are gitignored because they have caused the world too much pain.

## seed
follow the README there. it's good, I promise

# TODO:
## I/O
- [ ] Format key matrix scanning into class/object
    - [ ] add state for debouncing?
    - [ ] investigate doing it in pio

- [ ] make midi sending robust 
    - [ ] deduplicate extra sends on oscope (maybe a debounce issue?)
    - [ ] investigate polyphony in daisy seed
    - [ ] look at doing it in pio
    
- [x] mock out strum plate as keyboard switches
- [ ] connect cap sensor

- [ ] integrate imu

## Theory
- [ ] add scale support
- [ ] re-interrogate the note struct (add support for shifting?)

## Infra
- [ ] roll back libdaisy versions to versions happy with pluck.cpp

## Organization
- [ ] move pio into a folder
