/*****
 * ftserver.c
 * Author:Jesse Thoren
 * Description: This is the server part of a 2-connection client-server
 *    network application designed for a programming assignment for
 *    CS372 at Oregon State University.
 * References:
 *    www.linushowtos.org/C_C++/socket.htm
 *    beej.us/guide/bgnet/output/print/bgnet_USLetter_2.pdf
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
