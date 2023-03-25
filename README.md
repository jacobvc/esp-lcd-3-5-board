# ESP LCD 3.5 DIY System

This is a board support package for a DIY system using the [Lcd3.5Connectors board](https://github.com/jacobvc/ESP32-Hardware-Boards/tree/main/Lcd3.5Connectors). This board uses one entire side of an ESP32 Development Module to host a [320 x 480 touch screen display and SD Card](http://www.lcdwiki.com/3.5inch_SPI_Module_ILI9488_SKU:MSP3520) as well as an I2C interface.

The primary motiviation for this was to use some existing hardware (and LVGL v7x library) with a Squareline generated screen. It was immediately obvious that it would be very painful to use Squareline with LVGL pre-8.0. In the process, it also made sense to move into managed components.

The provided [Squareline example](examples/SquareLine) project implements a trivial Squareline Studio generated application.

The [custom_board](custom_board] is a board definition for Squareline Studio's [Open Board Platform](https://docs.squareline.io/docs/obp).

This is called a DIY system because one can get an ESP32 development board, a [Lcd3.5Connectors board](https://github.com/jacobvc/ESP32-Hardware-Boards/tree/main/Lcd3.5Connectors) (less than $5 for 3 boards from OSH Park), and a 3.5 inch touch screen display. Then anyone with limited soldering skills can assemble this to utilize one half of the development board for a touch screen system with a SD Card interface.

The other half of the ESP32 can even be populated with one of the other connectivity boards for various combinations of physical I/O.

Note: I will probablty be putting together board support packages for those other boards as well intendted to make a single device supported by two BSP's.


