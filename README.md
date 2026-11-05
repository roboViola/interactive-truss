# Interactive Truss - Statics Learning Tool
## Description
The goal of this project is to provide a low cost, effective solution to students taking a Statics (or similar) course. Since truss structures do not visibly move or change shape, it makes visualization of the load distribution such as compression, tension, and zero-force members difficult to visualize.

This project introduces a build-your-own kit where truss members can be setup in many configurations and have forces applied. These members measure the axial forces on them and show the approximate force via built in LEDs. A hub module is also provided with the kit to connect all truss members together via ESP-NOW and produces a web app to view live data from.

## Setup
1. Assemble desired truss structure
2. Power hub module and all truss members using the sliding switch
3. Open your computer's Wi-Fi settings
4. Select the network "StationDemo" and enter "StationDemo" as the password when prompted
6. Open your web browser and go type the IP Address 192.168.4.1 into the search bar and press enter

## How to Use
### Visual Indicators
**Web-App and LED Colors**
- Red = Compresssion
- Blue = Tension
- Yellow = Zero-Force

**LED Force Readout**
- 1 LED = 0.5-10 N (Red or Blue)
- 2 LED = 10-20 N
- 3 LED = 20-30 N
- 4 LED = 30-40 N
- 5 LED = 40-50 N

**Web-App Force Readout**
- Forces are displayed as a number directly beneath the link number
- Units are Newtons (N)

### Module Control
**Zeroing**
- Press and hold button on hub module until module LEDs turn yellow
- 1 LED will be on

**Self-Identification**
- On Web-App click on the link you would like to find
- Look at the truss and watch for one of the member's LEDs to begin pulsing
- The member whose LEDs pulse is that Link number in the web-app.
