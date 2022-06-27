For Q1

g++ server.cpp -o server 
If that doesnot work use,
g++ server.cpp -pthread -o server 

./server

Find server ip by 
hostname -I

g++ client.cpp -o client

In 2 terminals

./client <server-ip>


For Q2

g++ yapp.cpp -o yapp
sudo ./yapp <ip-to-ping>