# Robin_Dual_Ant_ESC-6-24V_2-6S_3.6A
This ESC is designed for 150g Ant weight combat robots, to drive N20 and N10 brushed motors.
It takes standard RC PWM signals as inputs and supports internal mixing and overhead driving.
The ESC is designed to be used with 2-6S, and a maximum current of 3.6A.
The ESC has internal current limmiting and thermal shutdown.
The internal BEC can supply up to 100mA for a receiver. An external BEC should be used for voltages over 4S(16.8V).
The PCB without the pannels is 22x22mm and weighs 2.3g.

## Wiring section
<img width="5011" height="3600" alt="Dual_ESC_6-24V_3 6A_V3_Wiring" src="https://github.com/user-attachments/assets/c53a9662-847e-455c-bca9-0dda34f41c5b" />
The Panels at the top and bottom are just for manufacturing and can be snapped off to reduce the overall size and weight.
<img width="3000" height="4000" alt="ESC_without_panels" src="https://github.com/user-attachments/assets/79431841-af35-4cf3-a399-0d985aca78da" />
The 3 pins at the bottom are for flashing the MCU and are not used in normal operation.
The polarity of the motors does not matter, and can be flipped later in software through the third signal line;

## Programming section
To program the ESC, power it on and wait for the red LED blink a second time.
The ESC is now in programming mode. 
Any non zero input on the Signal 1 and Signal 2 will end the programming mode. The red LED will be permanently on when in driving mode. 
To enter prgramming mode again, the ESC needs to be power cycled.
In programming mode changing the signal 3 to high (>80) increases the programming counter and the red led blinkes once.
If the right menu number is reached, wait for the red led to blink without any input. The setting has now been activated/deactivated.

| Number  | Function | Default |
| ------------- | ------------- | ------------- |
| 1  | None  |  |
| 2  | No mixing | No mixing |
| 3  | 25% mixing | No mixing |
| 4  | 50% mixing | No mixing |
| 5  | 75% mixing | No mixing |
| 6  | 100% mixing | No mixing |
| 7  | Breaking on/off | Breaking on |
| 8  | Invert channel 1 | Not inverted |
| 9  | Invert channel 2 | Not inverted |
| 10  | Switch channel 1 & 2 | Not switched |
| 11  | (Without function) Under voltage lockout at 3V per cell  | UVLO off |
| 12  | Wider deadzone on/off | off |
| 13  | reset to default |  |

The Under voltage lockout is without function for now, sice I mixed the pin functions between different packages.

## Mixing section
How to set up the ESC for internal mixing with signal 1 for steering and signal 2 for forwards/reverse.
The polarity of the motors does not matter for this one.
1. chose on of the 4 mix settings. I recommend to try out every one to get the steering level you are most comfortable with.
2. If the steering and forward/revers are switched, use the "Switch channel 1 & 2" function to change it.
3. Now if one or both of the channels are reversed use the "Invert channel X" functions to fix the reversed channels.

## Manufacturing with EasyEDA and JLCPCB
The ESC is designed to be cheaply manufactured in batches as small as 5. (The lowest quantity JLC will manufacture)
The last batch of 5 from JLC cost 22€ including shipping.

The easiest way to get your own ESC manufactured is by importing the .epro file to EasyEDA as a professional projekt.
You can find the file under Hardware/Version/...
Now you need to press order/Order PCB and it will automatically import all the necessary data. Before ordering it will do a design rule check. It should not return any errors and you can continue with the order after it. 
You don't need to change any settings this way except selecting PCB Assembly. You can change the color, or to lead free finish on the PCB if you want, this does not impact the function at all.
Now you can continue to the folliwing steps without needing to change anything and finish your order. Don't forget to appy coupons JLC gives out at checkout.

If you want to order larger quanteties, you can pannelize the PCB in easyEDA under Tools/Panelize. This can be cheaper compared to just ordering a larger number of PCBs. You need to chose "Panel by Customer" when ordering if you do that.

## Manufacturing with gerber files
The other way to get your own PCB manufactured is to upload the Gerber, BOM and CPL file to a manufacturer of choice.
You can find the files under Hardware/Version/...
If you do it with JLC you might need to realign the components. For Other PCB manufacturers everything should work the same, but this was not tested.

## Substituting components for manufacturing
All resistors, TVS Diodes and capacitors are generic. They can be replaced with same spec ones if necessary.
There are generics of the DRV8870 which should work the same, but those were not tested.
The Attiny816 can be replaced with an Attiny1616 if the 816 is not available for whatever reason.
The LDO can be replaced with a same spec LDO if necessary too.



## Flashing section
The microcontroller is flashed using the Arduino IDE and MegaTinyCore from Spencer Konde https://github.com/SpenceKonde/megaTinyCore
To flash the microcontroller a USB to serial adapter is necessary. I used one based on a CP2102M and a schottky diode between RX and TX.
Details to make your own can be found here https://github.com/SpenceKonde/AVR-Guidance/blob/master/UPDI/jtag2updi.md
Once you have installed the MegaTinyCore into the Arduino IDE you need to set the upload settings
<img width="1502" height="1640" alt="Arduino Settings" src="https://github.com/user-attachments/assets/2c7c8970-1905-48ec-869b-44d9cad1d92b" />
Now you can connect the UDPI, 5V and GND from the serial adapter to the 3 pins at the bottom.
<img width="2137" height="3023" alt="Dual_ESC_6-24V_3 6A_V3_Flash_Wiring" src="https://github.com/user-attachments/assets/22267136-7ebc-4148-8c87-93503a2305af" />
<img width="4000" height="3000" alt="MCU_Flashing_Photo" src="https://github.com/user-attachments/assets/989af4ba-63e3-4a8d-903a-875a5fa6acf0" />
Now press upload in the Arduino IDE. 
You can check if the microcontroller was flashed sucsecfully by the red led blinking, indicating the programming mode. The ESC can now be used.
