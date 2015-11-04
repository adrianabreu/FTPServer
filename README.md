#Sample FTP Server in C

This a simple implementation of a FTP Server based on its REF codes.

##What it supports?

It supports active and passive mode. 

##How to setup?

###You need the file conf.dat in the same directory, in UTF-8 format.

For compile, you have a makefile, use make.

Check that your firewall doesn't block the port 2121!!

##Tests done:
1. ls on active mode.
2. Upload a file in active mode.
3. Download the file with another name in active mode.
4. Upload the file in passive mode with other name.
5. ls on passive mode.
6. Download the file in passive mode.

