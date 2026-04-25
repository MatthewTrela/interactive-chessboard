## ECE 445 Lab notebook for Matthew Trela (mtrela2)
## 1/29/26
Formed a complete lab group around the idea to make a chessboard that can detect where the pieces are on the board and then uses an array of LEDs under the squares to enhance gameplay by lighting up legal moves for the piece you pick up or showing the best move in the position. The group members are me, Tim Chen, and Jeanjuella Tipan. I wrote most of the RFA, and we got RFA approval and are working on ironing out details of construction and approach. 

## 2/6/26
Worked on some sketches on how we could configure our lights, sensors, and pcb board. Since sensors will likely have to be close to magnets I'm trying to figure out how the LEDs will fit into the board. The plan is to have all the sensors on a PCB board, but the best way to do that is up in the air. My 3 ideas are to have the LEDs connected to the bottom of the chess board, have the LEDs underneath the PCB board, or to have multiple PCB boards and then to alternate PCB board LEDs. 

<img width="500" alt="Image" src="https://github.com/user-attachments/assets/4473337b-8c63-43e8-a48f-39db775a495a" />
<img width="500" alt="Image" src="https://github.com/user-attachments/assets/3dbcf5db-d24f-44fc-9f96-2d69bc6a0351" />
<img width="500" alt="Image" src="https://github.com/user-attachments/assets/2620f29b-3262-4bd8-8b55-fd3a61a45f19" />

## 2/12/26
Working on getting the project proposal finished. For the project proposal I made the block diagram, some of the high level requirements, the subsystem goals, tolerance analysis for the reed sensors, and did all the formatting.

## 2/20/26
We are finalizing some details of our design and working on getting things ordered. Today, we got out initial PCB board design reviewed and finished out team contract. The next major milestone is the breadboard demo, so we made a list of the bare essentials we need for the breadboard demo and made an order. We got reed switches, the LED strip, a through hole GPIO expander, and magnets of different strengths for testing. The plan for the breadboard demo is a proof of concept that we can power and read 16 reed switches from one GPIO expander and that we can then parse and use those readings to control some LEDs. Also thinking ahead, I put together a sketch with some simple dimensions that outlines what the finished board will look like. The idea is to use this sketch for reference when talking to the machine shop, and while we continue to design and put stuff together. 

<img width="500" alt="Image" src="https://github.com/user-attachments/assets/bb86b5b0-39e2-43cd-a75d-9949fe2a9613" />

## 2/23/26
The first PCB order is due this week on Tuesday, so this weekend I spent most of my time working on the schematic and physical route of the sensor PCB board. It is not a very complicated board, so hopefully this first version is enough and we don't have to order another one. I added pull-up resistors for reset and chip select and then some other resistors to dampen reflection and noise. I also added a capacitor to the power side of the GPIO expander which should help keep the voltage input stable. This should account for most errors that could arise from this board. The only other thing to consider is if our current header setup will fit with the shell we get from the machine shop. Looking ahead we have the design document due this week and the breadboard demo due the following week.

## 3/2/26
Today we presented our design document and did the peer review. In one week we have the breadboard demo, so most of this week we are planning to work on that. The plan right now is to have the MCU connected to the GPIO expander which is connected to 16 reed switches. Instead of messing with LEDs, we want to display our chessboard state on an OLED display. I set up our repository with libraries and gave us a starting point for the breadboard demo. I used eclipse as the IDE and set up a LED strip blinking template using Espressif. In terms of next steps for the repository, we need to find the library for the OLED display we are using and set up the gpio expander. 

## 3/7/26
Today Tim and I got our breadboard demo working. For components we used the ESP32-s3, the IO expander, 4 reed switches, and an OLED display. Our demo is primarily of our sensor subsystem, we just included the OLED display as a convenient way to show our sensor functionality. Initially we had some conflict over whether to use ESP-IDF with eclipse or PlatformIO with VScode, so we ended up writing minimum functioning demo software using both. Ultimately, using PlatformIO tools proved to be more convenient so we are switching to that from now on. Looking ahead, this week we have a lot to do. We want to order the rest of our parts, get a final design to the workshop for the board frame, and send in our order for the revised PCB board.

## 3/11/26
Spring break is next week and the machine shop deadline is this Friday, so this week I have been working on the housing for our chessboard. I made a mock up design of the housing yesterday. I want to keep it simple so the plan is a wooden box 10.6" x 14.6". It needs to be large enough to fit two sensor boards width wise and then length wise it needs to fit two sensor boards and the microcontroller board. The most important part is very precise holes drilled into the bottom of the housing to hold our sensor PCB boards. This is because for proper spacing to have a reed switch in the exact center of each chess square, the sensor PCB boards need to be exactly 6.67 mm apart edge to edge. For the top of the box the plan is to have holes drilled out for the OLED displays and rotary encoders. The actual chess playing surface will sit on top of an inlay milled into the sizes of the top part so it sits flush with the surface. I went to the workshop today, and Greg helped me make a CAD diagram of the exact specifications I wanted. He told me that the machine shop will have it built by the time we come back from break. The only thing I am still iffy about is the spacing I chose for the height. I decided to have the sensor boards 1 inch from the bottom of the box which should be okay, but I also decided on a distance of 12mm from the surface of the PCB boards to the bottom of the 3D printed chessboard. It is hard to know if this spacing will work for triggering the reed switches, so I will have to see when everything is constructed. We can always lower or raise the standoffs later.

<img width="500" alt="Image" src="https://github.com/user-attachments/assets/8ae8f913-fd2b-491c-b993-62b51bc2490b" />
<img width="500" alt="Image" src="https://github.com/user-attachments/assets/80213c37-0a81-43db-a23d-b027e7f47bfb" />
<img width="909" height="327" alt="Image" src="https://github.com/user-attachments/assets/ec8c707c-018b-4c05-b312-34b72debcd6c" />

## 3/23/26
We are now back from spring break, and the next thing I'm working on is the 3D print for the chessboard playing surface. Geometry wise it is not a very complicated shape to model, but there are a couple considerations I need to make for functionality. First, to trap the light and make sure LEDs don't bleed through to adjacent squares, I need to add a grid of black walls underneath each square to trap and direct light. I still don't know how I want to mount the LED strips for the chess squares. I want to try mounting the LED strips sideways under the chessboard because I think this way they would take up the least amount of space. This means I need to add some small tunnels for mounting the strips to the grid. Lastly, I need a way to differentiate black and white squares without making some squares all black. The light will not bleed through straight black PCA so I need a solution here. The thing I came up with is adding some black diagonal lines to the black squares, so you can tell they are black squares while still leaving most of the background white. 

<img width="500" alt="Image" src="https://github.com/user-attachments/assets/5f6612af-fe8b-4737-92b5-8d824898908e" />
<img width="500" alt="Image" src="https://github.com/user-attachments/assets/e645f6a3-cc9b-449f-a2d4-f3e402d770e4" />

