# Chat-Server-with-select

Implement a simple chat server using TCP and select().<br/>
When the server reads a message from the client, it reads it till a new line appears.<br/>

The server can talk with many clients, each on a different socket.<br/>
The server gets a message from the client and send it to all clients, for simplicity, also to the one who
sends it. The server assigns names to each client, the name is ‘guest<sd>’ where sd is the socket
descriptor assigned to this client after ‘accept()’ returns.<br/>
For example:<br/>
If client guest3 writes the message: “hello everyone\r\n”, The print will be like this:<br/>
guest3: hello everyone\r\n <br/>

Whenever your socket is ready to read or write it's print:<br/>
“server is ready to read from socket <sd>\n” or<br/>
“Server is ready to write to socket <sd>\n”<br/>

