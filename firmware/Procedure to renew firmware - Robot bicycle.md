# Procedure to renew the firmware 
### Tim Huiskens & Gabriele Dell'Orto, - 16 September 2023

## ** Robot bicycle, from TU Delft Bicycle Lab  **

### Procedure for Ubuntu 18.04

The procedure is meant to create a new robot.bike.elf file, to be sent to the robot bicycle when connected via JTAG cable. The first part of the procedure is used to build the robot.bike.elf file. The second part is the procedure to upload the file onto the robot bicycle Olimex board.
 
If you need to reflash the firmware but no changes have been made on the robot bicycle code, you can directly upload the robot.bike.elf file on the Olimex board on the bicycle. To do this, you can skip the first instructions and directly move to the instruction ""# Connect the JTAG cable and power the microcontroller"". You can upload the file robot.bike.elf which has been already built, and stored in \robot.bicycle\firmware\build


#### Open Command terminal and follow the instructions below. 

PS Olimex board is OK for working only if both the red led (power supply) and green led (OK status) are ON.
The green led is switched OFF during the renew procedure, it should switch ON again after typing the command (gdb) run in openocd

#### First part: build the robot.bike.elf file

#Install dependencies. 
PS On Tim's laptop (Ubuntu 20), it works with "libqt4-def" (no errors). On TU Delft laptop (Dell Latitude 5410, Ubuntu 18.04), it shows errors, and we need to rename the package "libqt4-dev", as written below.
```
~$ sudo apt-get install libeigen3-dev liblapacke-dev libfftw3-dev libqt4-dev libprotobuf-dev protobuf-compiler python-protobuf
```

#Create the folder "toolchain" (in the "Home" folder, not in "robot.bicycle" folder). Download ARM gnu version, the last one available from the website https://developer.arm.com/downloads/-/arm-gnu-toolchain-downloads. Extract it (unzip) and move the folders to ~/toolchain folder.

#Get a local copy of the code
```
~$ git clone https://github.com/thuiskens/robot.bicycle.git
~$ cd robot.bicycle/
```
#Initialize submodules
#Now solved the issue. We don't need anymore to type this line, since Tim has fixed it on github branch. He updated the url to https. Skip to the next.
#~/robot.bicycle$ git submodule set-url Singleton https://github.com/hazelnusse/Singleton.git
Just type as follows
```
~/robot.bicycle git submodule update --init
```

#Build
#With build.sh you are creating the firmware. Since we are using a different version of the Ubuntu and ARM GNU with respect to Tim's one (Ubuntu 18.04 on Dell Latitude 5410 VS Ubuntu 20 on Tim's laptop), there are some issues in building the new firmware. To have the green led ON and everything OK, you have to start from the following steps. Tim has created the new firmware on his own laptop and and sent it to Dell Latitude 5410 lalptop.

```
~/robot.bicycle$ cd firmware/
~/robot.bicycle/firmware$ ./build.sh
```
#### Second part: upload the robot.bike.elf file onto Olimex board.
#### You can directly start from here if you haven't updated the robot bicycle code. 
#Connect the JTAG cable and power the microcontroller
#Connect to the Olimex
```
~/robot.bicycle/firmware$ openocd -f openocd.cfg
```
#Open another shell instance
#Type in the new shell instance
#Start the debugger
```
~/robot.bicycle/firmware$  ~/toolchain/bin/arm-none-eabi-gdb -tui build/robot.bike.elf
```
#Finally, connect to target
```
(gdb) target extended-remote localhost:3333
```
#Flash the firmware and reset
```
(gdb) load
(gdb) mon reset init
(gdb) run
```
#Green LED is ON.
```
(gdb) quite --> y (yes)
```
#Back to the first shell instance, type ctrl C to quit the openocd programme.

#Disconnect the JTAG cable and press the reset button of the Olimex
