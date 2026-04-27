# Week of 2/8-2/14
Completed sections of project proposal:
- Introduction Section: Problem, solution, visual aid
- Ethics and Safety section: Ethical consideration, safety considerations, regulatory standards compliance
Also completed tolerance analysis for power consumption using datasheets of components.

# Week of 2/15-2/21
Completed the team contract with the rest of the group.

# Week of 2/22-2/28
Completed sections of design document:
- Cost Analysis table, List of parts, labor, total cost
- Requirements and Verification for subsystems

# Week of 3/1 - 3/7
Completed Breadboard demo code
- Code for IO expander connection (polling -> interrupt)
- Code for OLED display outputting IO expander input
- Debounced switches

Completed breadboard circuiting
Referenced datasheets and documentation to wire up all components

# Week of 3/22 - 3/28
Soldered one switch board
- 3D printed a spacing block for consistent switch heights

# Week of 3/22 - 3/28
Soldered all remaining switch boards
Tested all switch boards and made necessary soldering fixes

# Week of 3/29 - 4/4
Wired all switch boards to same SPI bus using wago connectors
Tested basic switch detection on shared bus on breadboard

# Week of 4/5 - 4/11
Remapped switch inputs into 8x8 array with physical to virtual mapping
Remapped virtual mapping to snaking pattern LEDs such that triggered switches now trigger same square LED
Suggested change to board square pattern to be a ring around the center for better LED light visibility

# Week of 4/12 - 4/18
Reorganized breadboard code for individual modules and global variables
Setup switch boards and leds on physical board frame with standoffs
Attempted to shift over breadboard code onto PCB board

# Week of 4/19 - 4/25
Successfully shifted over breadboard code onto PCB board
- Reconfigured wiring for correct pinning on PCB board
- Shifted code and physical pins used as interrupt pin was identified as not working
Finished code without engine
- Completed Initialization phase
- Completed Castling both ways (king and rook lift, king lift and place)
- Completed en passant
- Completed game over reason (checkmate by side, insufficient material, stalemate.
- Completed promotion phase
- Completed legal move suggestions
- Completed correct error detection and resolution

# Week of 4/26 - 5/2
Added individual piece error correction

Integration of game state with menu features
