# Client-Server
Section 1 (Server)
 The server and an identical copy of the server called the mirror [see section 3] must
both run before any of the client (s) run and both of them must wait for request/s
from client/s
 Upon receiving a connection request from a client, the server forks a child process
that services the client request exclusively in a function called processclient() and (the
server) returns to listening to requests from other clients.
o The processclient() function enters an infinite loop waiting for the client to
send a command
Page 2 of 4
o Upon the receipt of a command from the client, processclient() performs the
action required to process the command as per the requirements listed in
section 2 and returns the result to the client
 Upon the receipt of quit from the client, processclient() exits.
 Note: for each client request, the server must fork a separate process with the
processclient() function to service the request and then go back to listening to
requests from other clients

Section 2 (Client)
The client process runs an infinite loop waiting for the user to enter one of the commands.
Note: The commands are not Linux commands and are defined(in this project) to denote the
action to be performed by the server.
Once the command is entered, the client verifies the syntax of the command and if it is okay,
sends the command to the server, else it prints an appropriate error message.
List of Client Commands:
 fgets file1 file2 file3 file4
o The server must search the files (file 1 ..up to file4) in its directory tree rooted
at ~ and return temp.tar.gz that contains at least one (or more of the listed
files) if they are present
o If none of the files are present, the server sends “No file found” to the client
(which is then printed on the client terminal by the client)
o Ex: C$ fgets new.txt ex1.c ex4.pdf
 tarfgetz size1 size2 <-u>
o The server must return to the client temp.tar.gz that contains all the files in
the directory tree rooted at its ~ whose file-size in bytes is >=size1 and <=size2
 size1 < = size2 (size1>= 0 and size2>=0)
o -u unzip temp.tar.gz in the pwd of the client
o Ex: C$ tarfgetz 1240 12450 -u
Page 3 of 4
 filesrch filename
o If the file filename is found in its file directory tree rooted at ~, the server must
return the filename, size(in bytes), and date created to the client and the
client prints the received information on its terminal.
 Note: if the file with the same name exists in multiple folders in the
directory tree rooted at ~, the server sends information pertaining to
the first successful search/match of filename
 Else the client prints “File not found”
o Ex: C$ filesrch sample.txt
 targzf <extension list> <-u> //up to 4 different file types
o the server must return temp.tar.gz that contains all the files in its directory tree
rooted at ~ belonging to the file type/s listed in the extension list, else the
server sends the message “No file found” to the client (which is printed on the
client terminal by the client)
o -u unzip temp.tar.gz in the pwd of client
o The extension list must have at least one file type and can have up to six
different file types
o Ex: C$ targzf c txt pdf
 getdirf date1 date2 <-u>
o The server must return to the client temp.tar.gz that contains all the files in the
directory tree rooted at ~ whose date of creation is <=date2 and >=date1
(date1<=date2)
o -u unzip temp.tar.gz in the pwd of the client
o Ex: C$ getdirf 2023-01-16 2023-03-04 -u
 quit The command is transferred to the server and the client process is terminated

Section 3 Alternating Between the Server and the Mirror
 The server and the mirror (the server’s copy possibly with a few
additions/changes) are to run on two different machines/terminals.
 The first 6 client connections are to be handled by the server.
 The next 6 client connections are to be handled by the mirror.
 The remaining client connections are to be handled by the server and the
mirror in an alternating manner- (ex: connection 13 is to be handled by the
server, connection 14 by the mirror, and so on)
