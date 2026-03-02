## ECE 445 Lab notebook for Matthew Trela (mtrela2)
## 1/29/26
Formed complete lab group arround the idea to make a chessboard that can detect where the pieces are on the board and then uses an array of LEDs under the squares to enhance gameplay by lighting up legal moves for the piece you pick up or showing the best move in the position. The group members are me, Tim Chen, and Jeanjuella Tipan. I wrote most of the RFA, and we got RFA approval and are working on ironing out details of construction and approach. 

## 2/6/26
Worked on some sketches on how we could configure our lights, sensors, and pcb board. Since sensors will likely have to be close to magnets I'm trying to figure out how the LEDs will fit into the board. The plan is to have all the sensors on a PCB board, but the best way to do that is up in the air. My 3 ideas are to have the LEDs connected to the bottom of the chess board, have the LEDs underneath the PCB board, or to have multiple PCB boards and then to alternate PCB board LEDs. 

<img width="500" alt="Image" src="https://github.com/user-attachments/assets/4473337b-8c63-43e8-a48f-39db775a495a" />
<img width="500" alt="Image" src="https://github.com/user-attachments/assets/3dbcf5db-d24f-44fc-9f96-2d69bc6a0351" />
<img width="500" alt="Image" src="https://github.com/user-attachments/assets/2620f29b-3262-4bd8-8b55-fd3a61a45f19" />

## 2/12/26
Working on getting the project proposal finished. For the project proposal I made the block diagram, some of the high level requirements, the subsystem goals, tolerence analysis for the reed sensors, and did all the formatting.

## 2/20/26
We are finalizing some details of our design and working on getting things ordered. Today, we got out initial PCB board design reviewed and finished out team contract. The next big milestone is the breadboard demo, so we made a list of the bare essentials we need for the breadboard demo and made an order. We got reed switches, the LED strip, a through hole GPIO expander, and magnets of different strengths for testing. The plan for the breadboard demo is a proof of concept that we can power and read 16 reed switches from one GPIO expander and that we can then parse and use those readings to control some LEDs. Also thinking ahead, I put together a sketch with some simple dimensions that outlines what are finsihed board will look like. The idea is to use this sketch for reference when talking to the machine shop, and while we continue to design and put stuff together. 

<img width="500" alt="Image" src="https://github.com/user-attachments/assets/bb86b5b0-39e2-43cd-a75d-9949fe2a9613" />

## 2/23/26
The first PCB order is due this week on tuesday, so this weekend I spent most of my time working on the schematic and physical route of the sensor PCB board. It it not a very complicated board, so hopefully this first version is enough and we don't have to order another one. I added pull-up resistors for reset and chip select and then some other resistors to dampen reflection and noise. I also added a capacitor to the power side of the GPIO expander which should help keep the voltage input stable. This should account for most errors that could arise in from this board. The only other thing to consider is if our current header setup will fit with the shell we get from the machine shop. Looking ahead we have the design doccument due this week and the breadboard demo due on the following week.

## 3/2/26
Today we presented our design doccument and did the peer review. In one week we have the breadboard demo, so most of this week we are planning to work on that. The plan right now is to have the MCU connected to the GPIO expander which is connected to 16 reed switches. Instead of messing with LEDs, we want to display our chessboard state on an OLED display. I set up our repository with libraries and gave us a starting point for the breadboard demo. I used eclipse as the IDE and setup a LED strip blinking template using Espressif. In terms of next steps for the repository, we need to find the library for the OLED display we are using and setup the gpio expander. 
