#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <arpa/inet.h> 
#include <bits/stdc++.h>

using namespace std;



void error(const char *msg)
{
    perror(msg);
    printf("Either the server shut down or the other player disconnected.\nGame over.\n");
    
    exit(0);
}

void recv_msg(int sockfd, char * msg)
{
    memset(msg, 0, 4);
    int n = read(sockfd, msg, 3);
    
    if (n < 0 || n != 3)
     error("ERROR reading message from server socket.");

    //printf("[DEBUG] Received message: %s\n", msg);
   }

int recv_int(int sockfd)
{
    int msg = 0;
    int n = read(sockfd, &msg, sizeof(int));
    
    if (n < 0 || n != sizeof(int)) 
        error("ERROR reading int from server socket");
    
   // printf("[DEBUG] Received int: %d\n", msg);
    
    return msg;
}

void draw_board(char board[][3])
{
    printf(" %c | %c | %c \n", board[0][0], board[0][1], board[0][2]);
    printf("***********\n");
    printf(" %c | %c | %c \n", board[1][0], board[1][1], board[1][2]);
    printf("***********\n");
    printf(" %c | %c | %c \n", board[2][0], board[2][1], board[2][2]);
}

void write_server_int(int sockfd, int msg)
{
    int n = write(sockfd, &msg, sizeof(int));
    if (n < 0)
        error("ERROR writing int to server socket");
    
    //printf("[DEBUG] Wrote int to server: %d\n", msg);
}

int take_turn(int sockfd)
{
    char buffer[10];
    int move=9;
    
    while (1) { 
        printf("Enter 0-8 to make a move: ");
	    
        fgets(buffer, 10, stdin);
        
        
	    move = buffer[0] - '0';
        
        if (move < 9 && move >= 0){
            printf("\n");
            //Send the box number to client
            write_server_int(sockfd, move);   
            break;
        } 
        else
            printf("\nInvalid input. Try again.\n");
    }
    return move;
}

int replay(int sockfd)
{
    char buffer[10];
    int move;
    
    while (1) { 
        printf("Enter 0 to discontinue and 1 to replay ");
	    fgets(buffer, 10, stdin);
	    move = buffer[0] - '0';
        if (move < 9 && move >= 0){
            printf("\n");
            //Send the box number to client
            write_server_int(sockfd, move);   
            break;
        } 
        else
            printf("\nInvalid input. Try again.\n");
    }

    return move;
}

int get_update(int sockfd, char board[][3])
{
    
    int player_id = recv_int(sockfd);
    int move = recv_int(sockfd);
    
    board[move/3][move%3] = player_id ? 'X' : 'O';  
}

int main(int argc, char *argv[])
{


    if(argc != 2)
    {
        printf("\n Usage: %s <ip of server> \n",argv[0]);
        return 1;
    } 

    

    //socket file descriptor - master socket
    int sockfd = 0;
    if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        printf("\n Error : Could not create socket \n");
        return 1;
    } 

    struct timeval timeout;      
    timeout.tv_sec = 15;
    timeout.tv_usec = 0;

    if (setsockopt (sockfd, SOL_SOCKET, SO_SNDTIMEO, &timeout,
                sizeof timeout) < 0)
        error("setsockopt failed\n");

    //server sockaddr structure
    struct sockaddr_in serv_addr;
    memset(&serv_addr, '0', sizeof(serv_addr)); 

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(5000); 


    //argv[1] is ip address
    if(inet_pton(AF_INET, argv[1], &serv_addr.sin_addr)<=0)
    {
        printf("\n inet_pton error occured\n");
        return 1;
    } 
    cout<<"Inet done\n";
    // connect with server address and master socket file descriptor
    if( connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
       printf("\n Error : Connect Failed \n");
       return 1;
    } 
    cout<<"Connect done\n";
    
    //Get Client id from server
    int id = recv_int(sockfd);

    bool play =1;
    //bool tot = 0;


    while(play ){

    
        char msg[4]; // Using a 3 character signalling procedure from server to clients
        char board[3][3] = { {' ', ' ', ' '}, 
                            {' ', ' ', ' '}, 
                            {' ', ' ', ' '} };

        printf("Tic-Tac-Toe\n------------\n");

        //Wait till msg says to start
        // Until then hold
        do {
            recv_msg(sockfd, msg);
            if (!strcmp(msg, "HLD"))
                printf("Waiting for a second player...\n");
        } while ( strcmp(msg, "SRT") );

        /* The game has begun. */
        printf("Game on!\n");

        printf(" %c | %c | %c \n", '0', '1', '2');
        printf("***********\n");
        printf(" %c | %c | %c \n", '3', '4', '5');
        printf("***********\n");
        printf(" %c | %c | %c \n", '6', '7', '8');

        printf("Your are %c's\n", id ? 'X' : 'O');

        draw_board(board);
        
        bool tot = 0;
        while(1) {
            recv_msg(sockfd, msg);

            if (!strcmp(msg, "TRN")) { 
                printf("Your move...\n");
                take_turn(sockfd);
               
            }
            else if (!strcmp(msg, "INV")) { 
                printf("That position has already been played. Try again.\n"); 
            }
            else if (!strcmp(msg, "TOT")) { 
                printf("TimeOut \n"); 
                exit(0);
               
            }
            else if (!strcmp(msg, "CNT")) { 
                int num_players = recv_int(sockfd);
                printf("There are currently %d active players.\n", num_players); 
            }
            else if (!strcmp(msg, "UPD")) { 
                get_update(sockfd, board);            
                   
                draw_board(board);
            }
            else if (!strcmp(msg, "WAT")) { 
                printf("Waiting for other players move...\n");
            }
            else if (!strcmp(msg, "WIN")) { 
                printf("You win!\n");

                int playAgain = replay(sockfd);
                int replayOppo = recv_int(sockfd);
                cout<<replayOppo<<endl;
                

                if(replayOppo == 0){
                    cout<<'opponent left\n';
                    play=0;
                    
                }
                else if(playAgain == 0 )
                play=0;
                break;
            }
            else if (!strcmp(msg, "LSE")) { 
                printf("You lost.\n");

                int playAgain = replay(sockfd);
                int replayOppo = recv_int(sockfd);

                cout<<replayOppo<<endl;
                if(replayOppo == 0){
                    cout<<'opponent left\n';
                    play=0;
                    
                }
                else if(playAgain == 0 )
                play=0;
                break;
                
            }
            else if (!strcmp(msg, "DRW")) { 
                printf("Draw.\n");
                int playAgain = replay(sockfd);
                int replayOppo = recv_int(sockfd);
                cout<<replayOppo<<endl;
                if(replayOppo == 0){
                    cout<<'opponent left\n';
                    play=0;
                    
                }
                else if(playAgain == 0 )
                play=0;
                break;
            }
            else 
                error("Unknown message.");
        }

    }
    
    printf("Game over.\n");
    close(sockfd);
    return 0;

}