<pre>
**************************************************************************
* For the impatient (try this first, then come back if it doesn't work)  *
**************************************************************************
0. "make"
1. "./xtux"
2. Select "play", then "Start New Game" in the menu.

****************
* REQUIREMENTS *
****************
To run XTux (client) you will need:

* X11 libraries
* Xpm libraries

Most Linux and FreeBSD distributions have both of these libraries already
present. The latest Xpm libraries can be obtained from ftp://ftp.x.org/contrib
in the libraries directory.

The server doesn't use X and just needs the c/unix/socket libraries.

****************
* INSTALLATION *
****************

1. Put the data directory where you want it to go (or just leave it)
2. Edit the toplevel Makefile and change DATADIR to the FULL path of the
   data directory. (default is "data", which should be ok unless you move it)
3. Change the Makefile(s) to suit your system. Probably the only one you'll
   have to change is the X11_PATH variable in "src/client/Makefile". Solaris
   users will have to add -lsocket and -lnsl (from memory) because the network
   libraries are not linked against by default. Sun also puts X in weird places.
4. Type "make". This will build the common libraries then the client and server
   binaries.

********************
* Playing the game *
********************

You can configure the controls to be the same as most first person shooters.
Remember to turn mouse-mode on in the menu.

If you wish to run the server on your machine, start a server with the
command "tux_serv" or "tux_serv -m MAP_NAME.map" to specify a specific map.
(for more info on the server, type "tux_serv -h")
The default frames per second is 40, this may be too high for interet play
on slow connections, try "tux_serv -f 15"

*************************
* HACKING/ CONTRIBUTING *
*************************

Changing how the weapons or entities work is pretty easy. For example, edit
the "data/weapons" file, and scroll down until you see the [RAILGUN] section.
Remove the "#" that is infront of the lines "number", "spread" and "reload_time"
at the end of the entry.

Try changing around some of the other variables and see what they do.
Stupidly high values are often quite fun. Add the lines "number 50" and
"spread 255" to the end of the [EGUN] entry for a ridiculously powerful weapon.

Now load the slashdot map and see how the new weapons behave.
You will need to close and reload the program for the new values to have an
effect. Have fun!

A link to the XTux-devel mailing list is on http://xtux.sourceforge.net
</pre>
