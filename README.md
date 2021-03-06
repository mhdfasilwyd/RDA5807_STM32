# RDA5807 Library for STM32
**RDA5807** library for **STM32** using standard peripheral library. This is a port of [RDA5807 Library(PU2CLR)](https://github.com/pu2clr/RDA5807) with little bit of changes here and there.. (Tested in **STM32F103C8T6**)
# Application
Configuring the **I2C** port is application overhead and will not be done during **API** initialisation. This is to make all boards support the **API** without actually changing library, not sure though!
# Building
- Download compiler from [ARM GNU Toolchain](https://developer.arm.com/tools-and-software/open-source-software/developer-tools/gnu-toolchain/gnu-rm/downloads) for linux (Eg. gcc-arm-none-eabi-10.3-2021.10-x86_64-linux.tar.bz2)
- Get **STM32F10x standard peripheral library** library from official site [STM32F10x standard library](https://www.st.com/en/embedded-software/stsw-stm32054.html) and extract to the project directory (Download according to your board)
- Extract to **/opt/** or different location
- Export the compiler path using bash (Google it..) or edit file **setup.sh** with compiler path and execute each time
- Install **ST-Link** driver for linux (Use **stlink.sh** script..)
- Enter **make flash** and make sure if everything works
- Enjoy!
# Status
- [x] Basic features
  - [x] Tune & Seek
  - [x] Status
  - [x] Volume Adjust
  - [x] Bass control
  - [x] Mute and more...
- [x] RDS Data
  - [x] Status and property
  - [ ] RDS features (In progress)
# Contribution
You can too contribute to this project!

