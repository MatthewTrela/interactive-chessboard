# Worklog - Jeanjuella Tipan

Jeanjuella Tipan's digital lab notebook for ECE445 SP26. 

## 2026-02-11 : Project Proposal

I worked on the project proposal. I mainly worked on writing out the overview paragraphs for all of the subsystems. 

## 2026-02-16 : Weekly Meeting with TA Wenjing Song & Introductory Meeting with Gregg Bennet (Machine Shop) & Thermal Junction Calculations

At the TA meeting, we briefly discussed the project proposal that we submitted and we reviewed the upcoming deadlines for this week. We also talked about our next steps as a group as we move forward with our project. At the meeting with Gregg from the Machine Shop, we introduced ourselves and our project. We spoke conceptually about how to design an external structure that a separate playing surface and PCB(s) could be mounted to. 

After both meetings, we planned out how we wanted to divide up the work for this week. We plan to have a design that can be discussed at the PCB Review on Friday (2/20/26). 

After this meeting, I began working on the schematic that we will present at the PCB Review. As part of this work, I calculated the junction temperature for every linear regulator available in the ECEB Electronics Services Shop to see which ones, if any, would work for our project. The equation for calculating junction temperature is as follows: $T_j = i_{out}(v_{in} - v_{out})(\Theta_{jc} + \Theta_{ca}) + T_a$ (from the [ECE445 Wiki Page](https://courses.grainger.illinois.edu/ece445/wiki/#/regulators/index)). $v_{in}$ is 5V, $v_{out}$ is 3.3V, and I have decided to make $T_a$ to account for a warm board or hot ambient conditions. $\Theta_{jc}$ and $\Theta_{ca}$ are specified in the datasheets for the linear regulators. In most datasheets, a value which combines both thermal resistances, $\Theta_{ja}$, is provided which I used for my calculations. 

To determine $i_{out}$, I looked through the datasheets for the GPIO expander, microcontroller, and OLED display that we wanted to use. These are the components in our design that need to be connected to a 3.3V source. 

For the microcontroller, I referenced Table 6-4 and Table 6-6 from the [datasheet](https://documentation.espressif.com/esp32-s3-wroom-1_wroom-1u_datasheet_en.pdf). According to Table 6-4, current consumption while using the microcontroller in Active mode with Wi-Fi at 1 Mbps should peak at 355 mA. For the linear regulator, I would like to budget for current consumption in worst case scenarios. Therefore, I want to consider the full capabilities of the microcontroller in these calculations. At the moment, I am not expecting our design to require Wi-Fi so I also considered this in a separate calculation. For this scenario, according to Table 6-6, using the microcontroller in Modem-sleep mode at 240 MHz with all peripherals enabled should peek at 107.9 mA. According to the third footnote, accessing flash could increase current consumption. Based on the information in the footnote, I added 10 mA to bring the total current consumption for this scenario up to 117.9 mA. 

For the OLED display, the [datasheet](https://www.uctronics.com/download/Amazon/U602602.pdf?srsltid=AfmBOoqhVAECAvaruKvK-QxBZ18XktN4YsC0RKrHZ_P2r6m8IqFOcJ4H) that I found was pretty sparse. It did say that the OLED display should consume 0.04W under normal operating conditions. If the OLED display is supplied 3.3V and consumes 0.04W, I estimated the current consumption to be about 12.12 mA. This is likely not super accurate but the OLED display did not seem to be a limiting factor because current consumption of the other components should be much larger than the OLED display. The junction temperature that is calculated should also be at least $20\degree-30\degree$ below the rated temperature for the linear regulator so I found this value to be acceptable for these calculations. 

For the GPIO expanders, according to the [datasheet](https://ww1.microchip.com/downloads/aemDocuments/documents/APID/ProductDocuments/DataSheets/MCP23017-Data-Sheet-DS20001952.pdf), the maximum current out of $V_{ss}$ should be 150 mA. Since we want to use four GPIO expanders, the total current consumption in the worst case would be 600 mA. 

Adding up all of these current consumption values, $i_{out}$ comes out to be 730.02 mA with the microcontroller in Modem-sleep mode and 967.12 mA with the microcontroller in Active mode. 

The following table outlines the junction temperature calculations for every linear regulator that is available in the Service shop. 
| Linear Regulator | $\Theta_{ja}$  | Modem-sleep $T_j$  | Active $T_j$ | $T_j$ Rating |
| ---------------- |:--------------:|:------------------:|:------------:|:------------:|
| [AZ1117CD-33TRG1](https://www.diodes.com/assets/Datasheets/AZ1117C.pdf)      | 100 $\Omega$ | $162.1\degree$ | $202.39\degree$ | $150\degree$ |
| [AP2112K-3.3TRG1](https://www.diodes.com/assets/Datasheets/AP2112.pdf)      | 184 $\Omega$ | $266.3\degree$ | $340.5\degree$ | $150\degree$ |
| [BD50FC0FP-E2](https://fscdn.rohm.com/en/products/databook/datasheet/ic/power/linear_regulator/bdxxfc0wefj-e.pdf) | 115.3 $\Omega$ | $181.1\degree$ | $227.5\degree$ | $150\degree$ |
| [LD1117-3.3](https://mm.digikey.com/Volume0/opasdata/d220001/medias/docus/5055/LD1117.pdf?_gl=1*1ry47ln*_up*MQ..*_gs*NQ..&gclid=EAIaIQobChMI9ZvsxJzkkgMVS0hHAR2e7TXQEAAYASAAEgLeMPD_BwE&gclsrc=aw.ds) | 88 $\Omega$ | $147.2\degree$ | $182.6\degree$ | $125\degree$ |
| [TLV73333PDBVT](https://www.ti.com/lit/ds/symlink/tlv733p.pdf?ts=1771424315244) | 228.4 $\Omega$ | $321.5\degree$ | $413.5\degree$ | $125\degree$ |
| [TPS7A2050PDBVR](https://www.ti.com/lit/ds/symlink/tps7a20.pdf?HQS=dis-dk-null-digikeymode-dsf-pf-null-wwe&ts=1771397968787&ref_url=https%253A%252F%252Fwww.izzitionelec.com%252F) | 187.1 $\Omega$ | $270.2\degree$ | $345.6\degree$ | $150\degree$ |

Based on the information in the table, none of the linear regulators are able to comfortably handle the demands of all of our components. Therefore, we will need to explore other options to step our supply voltage down from 5V to 3.3V. We will either need to find a linear regulator with a higher junction temperature rating or look into switching converters (namely buck converters). 

## 2026-02-17 - 2026-02-20 : Main Board PCB Design Work

My focus for the next two weeks would be directed towards finishing a PCB design for the main MCU board that can be submitted for the first round of PCB orders. For this first week, I focused on researching the components and subcircuits that are necessary for the microcontroller and power subsystems.

Since we were able to check out an ESP32-S3 DevKitC unit for use on a breadboard, we decided to move forward with the [ESP32-S3-WROOM](https://documentation.espressif.com/esp32-s3-wroom-1_wroom-1u_datasheet_en.pdf) microcontroller for the MCU board. It was also helpful that there was an [example board design](https://courses.grainger.illinois.edu/ece445/wiki/#/esp32_example/index) provided through the ECE445 Wiki. From a financial standpoint, it would save us a little bit of money because we can order SMD units of the microcontoller for free through the ECE Service Shop. 

For the sections of the MCU board design that directly relate to the ESP32-S3, I referenced the [example board design](https://courses.grainger.illinois.edu/ece445/wiki/#/esp32_example/index) for the majority of them. As part of my research, I had to look over the example design and parse through the sections that were relevant to our project. Ultimately, I chose to copy over the ESP32-S3 Module section, some parts of the I/O section, and the Debug Header section. I also included four mounting holes on our board. For our project, we do not need H-bridges for motor control or any current sensing circuity. The current draw of any of our components should be steady and small enough to not need any monitoring. Since the power supply that we want to use is a power bank with either a USB-C or USB-A output, the Battery Input and RVP section does not really apply. Based on my earlier research, it would be better for us to use a buck converter so the 3.3V Power section cannot be used. For the I/O section, I copied over the subcircuit for the Micro-B USB connection and the setup for the pin headers that would connect to Tx and Rx on the microcontroller.  

[schems: ESP32-S3 module, Debug Header, ESP32 I/O]

Since the Breadboard Demo would be happening in a couple of weeks, a lot of our prototyping and testing would happen through preparation for that. Therefore, the main purpose of this first order was to create a proof-of-concept design which we could use to test SMD components, like the microcontroller and power conversion circuitry. As a result, connections to LEDs and User I/O components was not really factored in. We also ultimately decided to move the GPIO expanders to the sensor board. 

With that in mind, the further research that I needed to do was directed towards the power subsystem. The power bank that we wanted to use had USB-A and USB-C outputs. USB-A to USB-C cables are very common so it would be feasible to include either connection on the MCU board. I initially wanted to work with a USB-C connector because I felt it would be easier to work with since it is a newer technology that is more widely becoming the standard. The research that I did seemed to validate this thinking for this project. I found [this](https://forum.digikey.com/t/simple-way-to-use-usb-type-c-to-get-5v-at-up-to-3a-15w/7016) forum post on the TechForum on the DigiKey website. In this post, a DigiKey employee shared a USB-C Breakout Board from Sparkfun. According to the DigiKey employee, if a USB-C connector is set up in a similar way to the design of the breakout board, 5V at up to 3A can be supplied through the USB-C connector from an external power source. This perfectly matches the specifications of our power bank. From this information, I added a USB-C receptacle to the design on KiCad. Since this is only for power, I added the version that is power only. I grounded the GND connection and I left the SHIELD connection unconnected based on how the Micro-B USB connector was set up. I connected one 5.1k ohm resistor between GND and each CC pin on the USB-C connector based on the information in the forum post. From on my research, VBUS is where the power comes from. In order to more safely test and validate that we were able to receive 5V with this setup, I added a [test point](https://www.digikey.com/en/products/detail/keystone-electronics/5000/255326) followed by a solder jumper to isolate this section. I also connected a 1000 uF capacitor after the solder jumper to help stabilize the 5V supply for the LEDs. To make testing and validation easier and more flexibile, I connected one banana jack footprint to the 5V net after the solder jumper and another to the GND net. This would allow us to connect a lab power supply to the board to provide a more reliable power supply. For this supply, there is also a test point for verification. 

[schem: 5v power]

For the 3.3V power conversion subcircuit, I needed to find a buck converter that could step down to 3.3V from 5V while still  providing 3A. I also considered it important that the buck converter was reliable, easy to work with, and low-cost. Through my research, I identified [this](https://www.digikey.com/en/products/detail/diodes-incorporated/AP62300TWU-7/12702558) converter. Looking through the [datasheet](https://www.diodes.com/assets/Datasheets/AP62300_AP62301_AP62300T.pdf) for this device, I felt that it fit all of my requirements. The datasheet includes a schematic of a typical application circuit and a table of recommended components for the application circuit which lists components for a circuit that steps down to 3.3V. This allowed me to very easily integrate this buck converter into our MCU board PCB design. To isolate this circuit for testing and verification, there is a test point and solder jumper like with the 5V power subcircuit. There are also banana jack connection points and a test point for the lab power supply. 

[typical circuit pic & recommended comp pic]
[schem: 3.3V power] 

[full r1 schem]

## Week of 2026-02-23 : Main Board PCB Design Work (cont.)

My work this week was focused on finishing up the main MCU board design so that it can be submitted. Since the component research and subcircuit schematic representations were generally finalized last week, I finished up the layout for the PCB. As part of this work, I researched and verified the footprint assignments for the components. This largely involved looking through datasheets for more unique components to make sure that the correct footprint has been assigned. For more standard components like SMD resistors and capacitors, I decided to use the hand solder footprints for the 0805 size. This size should be accessible and easier to work with when soldering. 

The PCB has the maximum allowed dimensions of 100 mm x 100 mm. Since we are not implementing anything with Wifi or Bluetooth, the microcontroller can be centered in the middle area of the board. This allows the pin headers that connect to the GPIO pins to be organized along the edges of the board. This makes the pin headers easily accessible by external components which is a priority since we have a lot of things that we would like to connect to the microcontroller. The programming circuit provided by the ECE445 Wiki is arranged below the microcontroller, trying to keep it as close to the microcontroller pins as possible. The Micro-B USB subcircuit is located in the top left because that makes it both accessible and close to the D+/D- pins on the microcontroller that it connects to. The power subsystem is arranged along the right side of the board. I felt that this physically isolated the power subcircuits and made them more accessible and easier to test. Generally, I tried to arrange the sections of the design in a way that is efficient in terms of space while also considering accessibility for testing and soldering. 

For routing, I used the custom track widths recommended in the CAD Assignment. I used track widths that are thinner for data signals and thicker track widths for power connections. The mounting holes are placed wherever there is space. 

[layout: pcb v1]

## Week of 2026-03-02 : ECE445 Assignments

Since the MCU board PCB design was submitted last week, my work this week was mostly preparing for the Design Review and Breadboard Demo with Tim and Matthew. I also started looking into specific components to order for the MCU board. 

## Week of 2026-03-09 : Round 1 Main MCU Board

The MCU board PCB from the Round 1 order arrived this week. My work this week involved starting to solder on any components that we had on hand. I also finalized the final list of components that need to be ordered for both the inital order and any future orders. I also had a goal to put in a revised MCU board PCB design for the third round of orders. This order would've hopefully arrived a little bit after spring break which would hopefully give us a lot of runway to test the design before the Progress Demo. I started to revise the design this week but I was not able to finish it before the Round 3 submission deadline.

I was able to finish organizing the pin header connections to the microcontroller GPIO pins. These pin headers were organized in a way that would more efficiently connect to the user I/O and sensor subsystems. This was also based on information provided by Tim and Matthew on how they wanted GPIO pins assigned. There is a 4.7k ohm pull-up resistor on the interrupt pin for the SPI pins.

I did some research into how to set up the hardware and connectors for the LED strip. I found this guide on a [WLED Wiki](https://kno.wled.ge/basics/getting-started/) which showed a set-up for the WS281B LEDs on the LED strip. I followed this guide and incorporated its suggestions in the revisions. This mostly involved connecting a data resistor and level shifter to the pin assigned to the data signal for the LEDs. This is necessary because the microcontroller outputs signals at too low of a voltage level to be reliably read by the LEDs. Therefore, a level shifter is necessary to increase that voltage. 


[schem: r4 i/o]

I included some [diodes](https://www.digikey.com/en/products/detail/littelfuse-inc/SP1250-01ETG/12759480?curr=usd&utm_campaign=buynow&utm_medium=aggregator&utm_source=octopart) and [fuses](https://www.digikey.com/en/products/detail/littelfuse-inc/0805L300SLWR/3661960) for ESD protection in the 5V power section.

[schem: r4 5v]

I also changed the footprint for the capacitor on the 5V net to a through hole footprint for an aluminum capacitor because a capacitor with 1000 uF of capacitance is more accessible when it's an aluminum capacitor. 

[schems: board v2]

## Week of 2026-03-23 : Round 1 Main MCU Board (cont.) & PCB Revisions

Since the components arrived, my work this week focused on the finishing soldering the components onto the MCU board and testing the functionality. During my testing, I was able to verify using a multimeter that there was a stable 5V supply to the board when connected to the power bank. When I tested the 3.3V conversion circuit, there was not a stable 3.3V supply. In hindight, it is a bit of a silver lining that we were unable to submit a revision for Round 3 because this is really important information. Without reliable 3.3V power, our device would have basically no functionaility. 

Having identified that the 3.3V conversion circuitry had issues, I looked into how to address this issue. I compared the schematic on KiCad with the example circuit on the [datasheet](https://www.diodes.com/assets/Datasheets/AP62300_AP62301_AP62300T.pdf). I saw no major discrepancies. I knew that this circuit should work, in theory, because it was from the datasheet. After reading through the datasheet more closely, I determined that the issue(s) were either with irregularities in the components, soldering issues, or inefficient routing that introduces too much noise or limits heat dissipation. I did use the continuity feature on a multimeter to test for bridging. I did not identify anything but there could have been things that I missed. I revised the PCB design to make the routing more efficient for the Round 4 submission. I also asked for 2 oz copper layers instead of 1 oz layers to also improve heat dissipation. 

[layout: new 3.3v section]

I also finalized the layout for the other sections of the PCB design that were already revised at the schematic level. The ESP32-S3 Module section was the only section that did not have major revisions.  

[layout: board v2]

## Week of 2026-03-30 : Round 1 Main MCU Board (cont.) & Round 4 Main MCU Board Revisions

I continued testing the Round 1 MCU board to try to get it work for the Progress Demo the following week. Even when I tried soldering on new components, I could not get the 3.3V conversion circuit to work. With 5V supplied by the power bank and 3.3V supplied by a lab power supply, I was able to flash a very simple program onto the microcontroller which just oscillates the output at one of the GPIO pins. This is setup is not super easy to work with for the Progress Demo but it was encouraging to see that there should be little issue, from a hardware standpoint, implementing full functionality if we are able to get the 3.3V conversion circuit working. 

[oscill]

I also adjusted some of the traces on the PCB layout because of the requirements laid out by JLCPCB regarding boards with 2 oz layers was different from the requirements for boards with 1 oz layers. 

I also worked on the Individual Progress Report. 

## Week of 2026-04-06 : Progress Demo & Round 4 Board Fabrication

I did not have as much work in the beginning because the Round 1 MCU board was not working for the Progress Demo, so I was waiting for the arrival of the Round 4 PCB to be able to start working on that. I also put in an order for some more components for the Round 4 board. 

When the board did arrive at the end of the week, I worked on soldering on the components so that I would be able to test it over the weekend. To prevent bridging and have more consistent and efficient soldering overall, I used the reflow oven to solder on most of the components. 

## Week of 2026-04-13 : Final Fabrication and Testing

I finished soldering on any other components that were missing. Afterwards, I started the process of testing and validating the functionality of the Round 4 MCU board. Once the soldering was completed, I was able to verify with the multimeter that, when connected to only the power bank, there was steady 3.3V and 5V supply. I also flashed a simple program that oscillates the output at one of the GPIO pins onto the microcontroller. I used an oscilloscope to confirm that the signal could be read and was behaving as expected. This served as the inital testing and verification process for the MCU board functionality. 

[mult: 3.3v]

Once Matthew and Tim started working with the MCU board, they had some issues that seemed to be caused by discrepancies with the hardware. Once I did some testing, I agreed. I looked at the KiCad files for the Round 4 iteration more closely, and I noticed that the connections to one of the transistors in the programming circuit for the ESP32-S3 was routed incorrectly. This was a very small mistake that must have occurred when I copied over this section from the previous iteration. It did, unfortunately, have a big impact on the functionality of the board at a higher level. To address this issue, I used wires to physically reroute the connections to match the correct circuit design. Once I did this, there were no longer any major issues with the functionality of the MCU board. 

[schem w/ issue]

[correct schem]

[main board physical pic]

## Week of 2026-04-20 : Mock Demo & Presentation

We worked on preparing for the Mock Demo and the Mock Presentation. For the Mock Presentation, I worked on slides for the introduction, objective, and hardware systems. 

## Week of 2026-04-27 : Final Demo & Presentation

We worked on preparing for the Final Demo and the Final Presentation. For the Final Presentation, I worked on slides for the introduction, objective, and hardware systems. 

## Week of 2026-05-04 : Final Paper

We worked on finishing the final paper. I worked on the conclusion and design process sections. 












