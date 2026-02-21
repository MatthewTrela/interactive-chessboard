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
