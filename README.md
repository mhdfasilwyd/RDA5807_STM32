# RDA5807 Library for STM32
RDA5807 library for STM32 using standard peripheral library. This is a port of [RDA5807 Library(PU2CLR)](https://github.com/pu2clr/RDA5807) with little bit of changes here and there..
# Code Inro
Library | Application | Support
------- | ----------- | -------
RDA_5807 | inc | setup.sh
         | src | stlink.sh
         |     | Makefile
# Application
Configuring the I2C port is application overhead and will not be done during API initialisation. This is to make all boards support the API without actually changing library, not sure though!
# Building
- Configure your linux for STM32 environment (Compiler sort of..)
- Install ST-Link driver for linux (Use **stlink.sh** script..)
- Export the path while configuring compiler or edit file **setup.sh** and execute each time
- Enter **make flash** and make sure if everything works
- Enjoy!
# Status
- [x] Development
# Contribution
You can too contribute to this project!

