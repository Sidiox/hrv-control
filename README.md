# BRINK (HRV/WTW) Control via Home-Assistant and OpenTherm
This repo contains the code for the Brink HRV control as I've set up at my home.

Utilizes Arduino with OpenTherm for the Brink HRV control, pushing and listening to MQTT for interaction.
Then Home-Assistant uses MQTT to receive sensor information and push speed control.


Makes use of a modified [opentherm_library](https://github.com/ihormelnyk/opentherm_library):
[my branch](https://github.com/Sidiox/opentherm_library/tree/hvac-support)/[my pr](https://github.com/ihormelnyk/opentherm_library/pull/36).
The hardware is based on the excellent: [jpraus Arduino OpenTherm Shield](https://github.com/jpraus/arduino-opentherm)


# Home-Assistant
Contains the example configuration and some example automations.

Automations:
- WTW Night Mode: Switches to a lower speed for quiter operation at night
- WTW Day Mode: Switches to day speed in the morning
- WTW Shower Mode: Enables max speed for 30 minutes for better ventilation when showering

Configuration contains a bunch of sensors subscribing to the various published MQTT channels from the Arduino.