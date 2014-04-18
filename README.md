Smart Drawer is:

Hardware:
* Raspberry Pi
* NFC reader (like ACR122U)
* Powered USB hub
* Lockable drawer

Software:
* (in this repo) C code that loops to reader NFC device for ID cards and stickers (uses libnfc)
* (in this repo) PHP code that outputs data from database in JSON. Presents RESTful interface to allow addition and removal of users or devices, assignment of devices and other actions required
* mySQL database with data of users and devices
* apache installation to serve webpages and PHP pages.


