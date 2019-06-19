# T_Breakout

Complete design process written up on https://swicano.github.io/T_breakout_pt1/

<p align="center">
  <img src="https://raw.githubusercontent.com/Swicano/swicano.github.io/master/images/T_breakout/Complete1.jpg" alt="Complete1" align="center" width="500" />
  <p align="center"> Current state of the project </p>
</p>

A breakout board for the Adafruit Feather form factor which breaks out the SPI bus into 10 channels each with a unique C/S pin, for the purpose of attaching multiple adafruit MAX31856 breakout to a single feather.

This includes a [PCB design for the breakout board itself on Upverter](https://upverter.com/design/swicano/06dc52c63de35df1/kompass-iii/), imported into this repository
<p align="center">
  <img src="https://raw.githubusercontent.com/Swicano/swicano.github.io/master/images/T_breakout2/Breakout%20PCB%20layout%20v1.JPG" alt="Complete1" align="center" width="500" />
  <p align="center"> PCB design </p>
</p>
a 3d print file for a holder to mount the populated breakout, made in Fusion 360 and imported to this repository

<p>
<p align="center">
    <p align="left"><img src="https://raw.githubusercontent.com/Swicano/swicano.github.io/master/images/T_breakout2/Thermocouple_reader_unpopulated.PNG" align="left" width="410"/></p>
    <p align="right"><img src="https://raw.githubusercontent.com/Swicano/swicano.github.io/master/images/T_breakout2/Thermocouple_reader_populated.png" alt="populated" align="right" width="410"/></p>
</p>
<p align="center"> 3d printed holder </p>
</p>

<p>
as well as a very basic program to gather data from one data pin, which can be easily extended, in this repository.
</p>
