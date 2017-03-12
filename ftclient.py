# ftclient.py
# Jesse Thoren
# Description: This is the client part of a 2-connection client-server
# network application designed for a programming assignment for
# CS372 at Oregon State University.
# References: docs.python.org/2/library for socket usage in python

import socket
import sys
import signal
import os.path

#Handle SIGINT
def signal_handler(signal, frame):
    print("\nSignal received, gracefully exiting the client.")
    sys.exit(0)

def request_directory(connSock, dataPortNo, hostname, portNo):
    commandSend = connSock.send(bytes("-l",'UTF-8'))
    if commandSend < 1:
        print("Error writing command to socket")
        connSock.close()
        sys.exit(0)
    commandRec = connSock.recv(1024).decode('UTF-8'); #BUFFER_LENGTH
    if len(commandRec) < 1:
        print("Error reading command from socket")
        connSock.close()
        sys.exit(0)
    return 1

def request_file(connSock, dataPortNo, hostname, portNo, filename):
    commandSend = connSock.send(bytes("-g",'UTF-8'))
    if commandSend < 1:
        print("Error writing command to socket")
        connSock.close()
        sys.exit(0)
    commandRec = connSock.recv(1024).decode('UTF-8'); #BUFFER_LENGTH
    if len(commandRec) < 1:
        print("Error reading command from socket")
        connSock.close()
        sys.exit(0)
    filenameSend = connSock.send(bytes(filename,'UTF-8'))
    if filenameSend < 1:
        print("Error writing command to socket")
        connSock.close()
        sys.exit(0)
    filenameRec = connSock.recv(1024).decode('UTF-8'); #BUFFER_LENGTH
    if len(filenameRec) < 1:    
        print("Error reading command from socket")
        connSock.close()
        sys.exit(0)
    if filenameRec == "none":
        print("File does not exist")
        connSock.close()
        sys.exit(0)
    return 1

#Main Method
BUFFER_LENGTH = 1024
#Handle improper usage of the command line
if len(sys.argv) < 5:
    print("USAGE (Request directory):")
    print("   python3 ftclient.py <SERVER_HOST> <SERVER_PORT> -l <DATA_PORT>")
    print("USAGE (Request file):")
    print("   python3 ftclient.py <SERVER_HOST> <SERVER_PORT> -g <FILENAME> <DATA_PORT>")
    print("NOTE: Use localhost for <SERVER_HOST> for testing in same directory\n")
    sys.exit(0)

#Handle SIGINT:
signal.signal(signal.SIGINT,signal_handler)

#Handle file transfer command line validation
if len(sys.argv) > 5:
    hostname = socket.gethostbyname(sys.argv[1])
    try:
        portNo = int(sys.argv[2])
        if portNo<1024 or portNo>65535:
            raise Exception()
    except:
        print("Error: Port Number must be an integer in [1024,65535]")
        print("Type python3 ftclient.py to see usage")
        sys.exit(0)
    try:
        command = sys.argv[3]
        if command != '-l' and command != '-g':
            raise Exception()
    except:
        print("Error: Command is not recognized. Must be -l or -g.")
        sys.exit(0)
    filename = sys.argv[4]
    try:
        dataPortNo = int(sys.argv[5])
        if dataPortNo<1024 or dataPortNo>65535:
            raise Exception()
    except:
        print("Error: Data Port Number must be an integer in [1024,65535]")
        print("Type python3 ftclient.py to see usage")
        sys.exit(0)

#Handle directory request command line validation
else:
    hostname = socket.gethostbyname(sys.argv[1])
    try:
        portNo = int(sys.argv[2])
        if portNo<1024 or portNo>65535:
            raise Exception()
    except:
        print("Error: Port Number must be an integer in [1024,65535]")
        print("Type python3 ftclient.py to see usage")
        sys.exit(0)
    try:
        command = sys.argv[3]
        if command != '-l' and command != '-g':
            raise Exception()
    except:
        print("Error: Command is not recognized. Must be -l or -g.")
        sys.exit(0)
    filename = ""
    try:
        dataPortNo = int(sys.argv[4])
        if dataPortNo<1024 or dataPortNo>65535:
            raise Exception()
    except:
        print("Error: Data Port Number must be an integer in [1024,65535]")
        print("Type python3 ftclient.py to see usage")
        sys.exit(0)

#Verify that ports are different
if portNo == dataPortNo:
    print("Error: Data Port no must be different than Command port no.")
    sys.exit(0)

#Establish connection with server
try:
    connSocket = socket.socket()
except:
    print("Connection could not be established with the server")
    sys.exit(0)

try:
    connSocket.connect((hostname,portNo))
except:
    print("Error connecting with server")
    sys.exit(0)

#Set up connection for data transfer
#Researched docs.python.org/2/library/socket.html
dataSocket = socket.socket()
dataSocket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
dataSocket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEPORT, 1)
dataSocket.bind(('', dataPortNo))
dataSocket.listen(1)

#Send port for data communication to server
if(connSocket.send(bytes(str(dataPortNo), 'UTF-8'))<1):
    print("Error transmitting data port to server")
    connSocket.close()
    sys.exit(0)

#Wait for confirmation that data port was received
responseStr = connSocket.recv(BUFFER_LENGTH)
responseStr = responseStr.decode('UTF-8')
if(len(responseStr)<5):
    print("Error receiving data port ack from server")
    connSocket.close()
    sys.exit(0)
print("Server response: " + responseStr)

#Send request for directory if applicable
if command == '-l':
    request_directory(connSocket, dataPortNo, hostname, portNo)
    print("Directory successfully requested")

#Send request for file if applicable
if command == '-g':
    request_file(connSocket, dataPortNo, hostname, portNo, filename)
    print("File successfully requested")

#Accept/establish data connection for transfer
dataConnection, dataAddress = dataSocket.accept()

print("Data Connection established on port ", dataPortNo)

#Handle request for directory
if command == '-l':
    print("Now displaying requested directory contents.")
    #Get and display data from server
    while(1):
        directoryData = dataConnection.recv(BUFFER_LENGTH).decode('UTF-8')
        if len(directoryData) == 0:
            break
        print(directoryData)


#Handle request for file
if command == '-g':
    #Verify file exists
    checkfile = dataConnection.recv(BUFFER_LENGTH).decode('UTF-8')
    if len(checkfile) == 0:
        print("Error verifying filename\n");
        dataSocket.close()
        connSocket.close()
        sys.exit(0)
    if checkfile[0] == 'D' and checkfile[1] == 'N' and checkfile[2] == 'E':
        print("File does not exist\n");
        dataSocket.close()
        connSocket.close()
        sys.exit(0)
    #Handle case where file exists in same folder
    if(os.path.isfile(filename)):
        while(1):
            print("Filename requested exists in current folder. Overwrite?\n")
            choice = input("Y/N: ")
            if choice == "Y":
                print("Overwriting file\n")
                dataConnection.send(bytes("YES",'UTF-8'))
                break
            elif choice == "N":
                print("Exiting\n")
                dataSocket.close()
                connSocket.close()
                sys.exit(0)
            else:
                print("Please enter a valid selection.")
    else:
        dataConnection.send(bytes("YES",'UTF-8'))

    print("Now receiving file")
    fileText = ""
    #Loop until we get the entire file
    while(1):
        newText = dataConnection.recv(BUFFER_LENGTH).decode('UTF-8')
        if len(newText) == 0:
            break
        fileText += newText
    #Write received text to file
    newFile = open(filename, 'w')
    newFile.write(fileText)
    newFile.close()
    print("File successfully transferred.\n")


#Close data connection
dataSocket.close()

#Close connection to server
connSocket.close()

