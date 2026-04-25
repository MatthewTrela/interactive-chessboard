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

<img width="500" alt="Image" src="https://github.com/user-attachments/assets/6f904151-b655-42e1-8979-f74b2002d678" />
<img width="500" alt="Image" src="https://github.com/user-attachments/assets/aa3e02f2-3bee-4abb-85a5-555e0ba76012" />

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
We are now back from spring break and we got the housing back from the machine shop. We won't know for sure before we put everything together, but the dimensions all look correct, and the standoff holes look good too. The next thing I'm working on is the 3D print for the chessboard playing surface. Geometry wise it is not a very complicated shape to model, but there are a couple considerations I need to make for functionality. First, to trap the light and make sure LEDs don't bleed through to adjacent squares, I need to add a grid of black walls underneath each square to trap and direct light. I still don't know how I want to mount the LED strips for the chess squares. I want to try mounting the LED strips sideways under the chessboard because I think this way they would take up the least amount of space. This means I need to add some small tunnels for mounting the strips to the grid. Lastly, I need a way to differentiate black and white squares without making some squares all black. The light will not bleed through straight black PCA so I need a solution here. The thing I came up with is adding some black diagonal lines to the black squares, so you can tell they are black squares while still leaving most of the background white. 

<img width="500" alt="Image" src="https://github.com/user-attachments/assets/5f6612af-fe8b-4737-92b5-8d824898908e" />
<img width="500" alt="Image" src="https://github.com/user-attachments/assets/e645f6a3-cc9b-449f-a2d4-f3e402d770e4" />

## 3/24/26
Today I printed the model of the chess playing surface. I was worried about how thin the board surface was (4 layers or 0.8 mm), but it actually ended up printing on the first try with no issues. The print itself is pretty sturdy, but I have noticed a few issues that might mean we need to reprint. Mostly the grid that extends below the chess squares looks like it might be too long. We haven't connected the sensor boards to the housing yet because Tim is still working on soldering them, but a basic test of an empty sensor board screwed into standoffs and the 3D printed chessboard sitting on top of it has the chessboard popping above the surface of the box. Shouldn't be difficult to fix, but I am going to hold off on remodeling and printing it until we get more of the system put together. 

 <img width="500" alt="Image" src="https://github.com/user-attachments/assets/deeb529c-2552-448a-b76a-473d713d3a74" />
<img width="500" alt="Image" src="https://github.com/user-attachments/assets/78a57f06-426e-4240-93c0-6735579c7ffc" />

## 3/30/26
Most of the last week I have been working on getting the LED strips to fit the size of our chessboard. Something I noticed after 3D printing our chessboard is that the spacing of the LEDs on the LED strip we got is too big. The LEDs are 33mm apart, but our chess squares are 26.67 mm apart. I did some handwavey math before we got the LED strips that it would work out, but actually matching up the LED strip with our 3D print shows that it doesn't line up, and some squares don't even have an LED. There is not really an elegant solution to this problem. The thing I came up with is cutting the LED strip into pairs of 2 LEDs, trimming a little bit of the strip away on the junction between two pieces, using an exacto knife to strip away the black coating on the LED to expose the data, ground, and vcc copper, and then manually soldering the strips back together. It is not ideal, but it does work to resize the strips. Each strip takes me 20-40 minutes and I have to do 8 of them, but right now I am mostly done.  

<img width="1000" alt="Image" src="https://github.com/user-attachments/assets/694faab9-6348-4bc9-957b-f8cec1d7b944" />

## 3/31/26
I finally finished soldering the LED strips. I did a test run of every LED strip connected together to make sure all the soldered connections are solid, and all 64 LEDs lit up which is a good sign. However, my soldering is still not done. The next thing I have to figure out is how I will connect the LED strips so that they form a grid instead of a straight line. We have plastic connectors that clamp down on two ends to connect them together. I found that if I take two of those I can connect them at a 180 degree angle with wires that I manually solder onto the connectors. I made one and it is what I use in the image below to turn the LED strip chain. Now I just need to make 6 more of them. 

<img width="700" alt="Image" src="https://github.com/user-attachments/assets/97067145-38f9-45be-967f-9423e93568a5" />

## 4/1/26
Today our individual progress reports are due. As part of my IPR, I did some testing on the LED strips. I did some power testing by connecting the bench power supply to the LEDs to power them at 5 volts, and then ran some tests with a program that set the LEDs to a couple of patterns. I tested things like lighting up all the starting squares in different colors, lighting up a worst case queen legal moves in the middle of the board, and a worst case power draw which is all 64 LEDs turned on with color white and max brightness. The worst case current draw was measured at 1.38 A which is a lot, but our powerbank is rated at 3 A, and there is no way the rest of our system takes even close to 1 A, so our chessboard should support pretty much any configuration of lit up LEDs we want. I also tested LEDs sideways vs. straight up and found that straight up is much more visible. Still, another issue I found with the chessboard 3D print is that even at full brightness straight up the lights are not very visible. I will definitely have to reprint our chessboard and I'm thinking of making the top layer thinner to let more light through.

<img width="500" alt="Image" src="https://github.com/user-attachments/assets/a5e375f2-d250-48d7-a14c-6edf716eb948" />

## 4/3/26
As part of our parts order we ordered smaller magnets because the ones we used in the breadboard demo were huge and way too strong. We ordered the magnets from digikey, and once again the magnets did not come. We gave up on digikey entirely and I ordered a package of 2mm thick magnets on amazon. I got a bunch of different diameters and my thinking is that we could stack them to get a stronger magnetic field if they aren't strong enough. I plan to test them with our chessboard to finetune the strength. 

## 4/6/26
Tim finished soldering all of the sensor PCB boards, and he was able to connect them all together on the same SPI bus. We screwed all the sensor PCBs into the standoffs and now we are trying to make sure every single reed switch can be triggered by a magnet. The progress demo is tomorrow so we really need to get this working. We were getting this confusing behavior where we would read that every reed switch has a high value across it, but when we actually measured the voltage across the switch with the multimeter it would have 0 voltage. This stumped me for a while and I thought it was some weird bug with hardware addressing and the IO expanders listening to commands before they were told to work in hardware addressing mode. Eventually I found that the actual problem was much simpler, and the MISO and MOSI spi pin assignments were backwards. Quickly switching those, 3/4 of the sensor boards work perfectly but the third one does not. Also on one of the working boards I noticed that there is a short between two IO expander pins. A magnet on either reed switch triggers both pins due to the short. It is getting pretty late, so I am handing these issues off to Tim for him to fix tomorrow.

## 4/7/26
Tim was able to get all 4 boards working for the demo. The problem ended up being the pin header on the fourth board. After he added more solder to it, and then used the head gun on the shorted pins, our sensor boards worked flawlessly. For the demo we wanted to show that a magnet on a reed switch activates the LED below it. Unfortunately, we never finished mapping the reed switch that triggers to its physical location on the chessboard. We got it working for some, but during the progress demo triggering most of the reed switches would trigger an LED in a different location. Tim got the mapping working after the demo so we are all good now. JJ had some problems with the buck converter on our main PCB board, so for the demo we didn't use an ESP32-S3 soldered to a PCB and instead used a breadboard and a devkit. We are supposed to get our 4th round PCB board later this week anyways, so we are planning to integrate the main PCB board once we get the order and JJ finishes soldering it. The next thing I’m working on is a redesign of our 3D printed chessboard.

## 4/8/26
I remodeled the chessboard. The main changes I made were reducing the height of the grid that extends below the chessboard, getting rid of the tunnel for mounting the LEDs sideways, and adding a circular cutout underneath every chess square to let more light through. In the area where the circle is cutout, the top layer is 0.4 mm thick. Also a big problem I realized is I did the math wrong for the first board when centering the squares. I made the edges of the board the same size as the lines between the squares, but the edges actually have to be half that length because each edge is only touching one playing square, not two. I have run into some issues trying to print this. I tried printing it twice and both times one of the squares in the first layer gets messed up. I think this is because the circle cutouts made the surface too thin.

<img width="500" alt="Image" src="https://github.com/user-attachments/assets/b0a55e0f-de33-4198-9cad-7089030c5c0d" />
<img width="500" alt="Image" src="https://github.com/user-attachments/assets/3182ad2c-2892-4d63-bb7c-1e2d1dc3a838" />

## 4/9/26
I was able to print the new chessboard, and it works a lot better. I figured out that the problem was just the first layer printing too fast, so I lowered the print speed of the first layer and that fixed it. The circle pattern looks really nice, and it has the added benefit of showing the players where the chess piece should be placed with a circle. This is nice because it shows that you need to place the chess in the center of the square because the reed switch might not trigger if it is placed at the edge.

<img width="500" alt="Image" src="https://github.com/user-attachments/assets/4664866b-c688-4c74-8424-dcd793174a39" />
<img width="500" alt="Image" src="https://github.com/user-attachments/assets/ec50feae-6ec4-4b56-ad0b-998542c7f672" />
<img width="500" alt="Image" src="https://github.com/user-attachments/assets/16dc37be-b4e4-4c8c-9d29-a9d413d9aa59" />

## 4/11/26
The next thing I am working on is printing our actual chess pieces. I found a set of already modeled chess pieces online which were allowed for commercial use. They were modeled by Jacob G, and can be found at this [link](https://www.printables.com/model/32741-chess-set/files). I needed to modify them slightly by making them smaller to fit the size of our smaller chessboard and also by adding a hole on the bottom for a magnet. I printed one pawn as a test and I am running into this problem where detection of the piece is very spotty. It works like 1/3 of the time when placing the pawn on a square

<img width="500" alt="Image" src="https://github.com/user-attachments/assets/5c16fe44-c007-45c5-9312-da8d00f1862d" />

## 4/12/26
I figured out a better way to model the chess pieces such that they trigger the reed switch every time. I noticed that if the magnet is in the dead center of the reed switch it doesn't actually trigger. It only triggers when the magnet is a little bit off center, over one of the edges of the reed switch. With this figured out, if I put two magnets under the chess piece lined up diagonally with the reed switch, the magnets are now at the edges when the piece is in the center of the square and the chances of triggering the switch are doubled. This new method triggers the reed switch every time, and has the nice property of forcing the chess piece to a certain orientation, like forcing the knight to face forward.

<img width="500" alt="Image" src="https://github.com/user-attachments/assets/d48000c6-b2fc-4820-bfd1-0fd341c97066" />
<img width="500" alt="Image" src="https://github.com/user-attachments/assets/554dfb44-2c88-4eb4-9534-a4367b56f791" />

## 4/13/26
I modeled and printed the rest of the chess pieces.


