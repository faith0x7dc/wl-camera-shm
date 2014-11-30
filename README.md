wl-camera-shm
===============

Camera capture program for Wayland based on weston-simple-shm

Dependencies
--------------

- wayland-client

Build
-------

    $ git clone https://github.com/faith0x7dc/wl-camera-shm.git
    $ cd wl-camera-shm
    $ make

Run
-----

    $ ./wl-camera-shm

Or, in order to specify video capture device execute below

    $ ./wl-camera-shm --device /dev/videoX

(/dev/videoX is path to video capture device)

