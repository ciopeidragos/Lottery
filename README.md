# Lottery
A small lottery implementation

23.Aug.2023 - At this moment only server.cpp it's available, client.cpp will be added later. Testing functionality of the server can be done via netcat.

How To Run:

1. Compile the server.cpp : clang++ server.cpp -o server -levent
2. Run the server : ./server
3. Start a localhost client with netcat on port 40713 : nc localhost 40713
4. You can start registering tickets using the reutrned unique id for your connection following the format: n:1,2,3,4,5,6 where n is the unique id and the enumeration 1,2,3,4,5,6 is your subset of numbers.

CAVEAT: This is a POC and does not contain validations for inputed numbers, any type of auth/authz and most probably it's not memory safe at this stage of developemnt. 
