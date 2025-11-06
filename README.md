# petrichord
Authors: Raquel Cervantes-Espinosa, Annabel Lee, Sam Ledden, and Eric Liu

## Project Structure
This monorepo contains the code for both the rpi and the seed.
`cd` into the respective directories in order to build for each microcontroller.
If you are building for the pico, please switch your vscode workspace to be based in the pico directory. (ctrl+k+o)


### TODO:
- [ ] Format key matrix scanning into class/object
    - [ ] add state for debouncing?
    - [ ] investigate doing it in pio

- [ ] make midi sending robust 
    - [ ] investigate polyphony in daisy seed
    - [ ] look at doing it in pio
    
- [ ] mock out strum plate

- [ ] 