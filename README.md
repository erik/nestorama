## Nestorama
This will be a NES emulator at some point hopefully.

### Building
To create an executable, run `make`.
For an executable with debugging symbols, run `make debug`.

Nestorama currently only requires the SDL library to build, though in
its present state, it is not yet utilized.

### Usage
Currently, nestorama has no graphics or audio capability, as well as
a partially completed CPU implementation. So for now, the project
cannot run NES ROMs with any kind of usefulness. Still, if you want to
test them, you can run the executable with `./nestorama [testrom.nes]`

Test ROMs and locations for finding other ROMs can be found in the
test/ directory.


### License
Nestorama is released under the GPLv3 license.

See COPYING for a full license text.
