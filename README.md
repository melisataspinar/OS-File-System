In this project we implement a simple file system called sfs. 

The file system is implemented as a library (libsimplefs.a) and stores files in a virtual disk. The virtual disk is a simple regular Linux file of some certain size. 

An application that would like to create and use files is linked with the library. 

We assume that the virtual disk will be used by one process at a time. When the process using the virtual disk terminates, then another process that will use the virtual disk can be started. With this assumption, we do not worry about race conditions.

