# MyChat
Multi-user-chat between different terminals. Using Named Pipes and multiprocessing.

Necessary Libraries:
sudo apt-get install libncurses5-dev libncursesw5-dev

Short description of operation:
A program launched with the parameter "--start" creates a background server process responsible for:
- registering connected users
- forwarding messages to appropriate users and creates a named pipe (FIFO) with the default name for this purpose, from which it reads all messages from users
- upon receiving the SIGQUIT signal, it terminates its operation, sending a notification to all registered users about the server shutdown

A program launched with the parameter --login additionally accepts the username as a parameter and registers the user by sending information to the server FIFO. If sending the information fails, the program terminates. If successful, it creates a new process that will create and listen to the user's FIFO pipe to receive information from the server and print the received messages to the screen (standard output). The parent process retrieves information about to whom and what content the message should be sent from the user. After reading this information, it sends it to the server FIFO in the appropriate format. The server constantly listens to its FIFO to receive information and after reading it, it sends messages to the FIFOs of individual users. When receiving the SIGQUIT signal, the user process sends a message to the server to logout and terminates both processes (by sending a signal to the child process).

The respective programs save information in system logs about each user login and logout operation, problems with sending information to the user by the server, and the correct creation of the named pipe with its name.

The project includes files such as:
myChat.c - responsible for distinguishing the parameters with which the program was called

server.c - responsible for:
- creating the server FIFO
- informing about user login/logout
- listening to the server FIFO
- sending messages to a specific user's FIFO
- handling the SIQUIT interrupt and sending information to users about the server's termination
- sending informational logs

login.c - responsible for:
- creating the user's FIFO
- Console handling within the ncurses library
- User process handling
- sending messages to the server FIFO
- listening to the user's FIFO
- handling the SIQUIT interrupt from the child process and sending information to the server about the user's logout
- sending informational logs

login.h and server.h - copying functions from login.c and server.c to myChat.c

Makefile - responsible for faster command entry.

User Manual:

In the first console:
1. Execute the command make main or gcc server.c server.h myChat.c login.c login.h -o myChat -Wall -lncurses;
2. Execute the command make server or ./myChat --start

In subsequent consoles:
./myChat --login [Username]

After pressing 's' in the user's console:
To Whom: [Recipient]
Message: [Message content]
