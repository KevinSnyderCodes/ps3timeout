# ps3timeout

Disconnect `hidraw` PS3 controllers due to inactivity.

## Purpose

[sixad](https://github.com/RetroPie/sixad) is a PS3 controller driver that supports timeout (i.e. disconnect the controller after _x_ time of inactivity).

The problem with sixad is that it overrides the default bluetooth daemon, making it impossible to use other bluetooth devices at the same time as your PS3 controller.

PS3 controllers do not need sixad to work. However, they must be manually turned off by holding the PS button. It's easy to forget to turn off your PS3 controller and have the battery drain needlessly.

This program enables timeout support for PS3 controllers without the need for sixad. It listens for controllers to appear as `hidraw` devices and disconnects them from the host machine using `hcitool`.
