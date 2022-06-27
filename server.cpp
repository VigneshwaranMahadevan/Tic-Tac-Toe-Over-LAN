#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <time.h> 
#include <bits/stdc++.h>
#include <pthread.h>
#include <string.h>
#include <iostream>
#include <fstream>

using std::cout; using std::fstream;
using std::endl; using std::string;


using namespace std;

fstream fp;


int player_count = 0;
pthread_mutex_t mutexcount;

void error(const char *msg)
{
    perror(msg);
    pthread_exit(NULL);
}

void write_client_msg(int cli_sockfd, char * msg)
{
    int n = write(cli_sockfd, msg, strlen(msg));
    if (n < 0)
        error("ERROR writing msg to client socket");
}

void write_client_int(int cli_sockfd, int msg)
{
    int n = write(cli_sockfd, &msg, sizeof(int));
    if (n < 0)
        error("ERROR writing int to client socket");
}


void write_clients_msg(int * cli_sockfd, char * msg)
{
    write_client_msg(cli_sockfd[0], msg);
    write_client_msg(cli_sockfd[1], msg);
}

void write_clients_int(int * cli_sockfd, int msg)
{
    write_client_int(cli_sockfd[0], msg);
    write_client_int(cli_sockfd[1], msg);
}

int recv_int_tout(int cli_sockfd)
{
    int msg = 0;

    fd_set readfds;
    FD_ZERO(&readfds);
    FD_SET(cli_sockfd, &readfds);
    

    struct timeval timeout;
    timeout.tv_sec = 15;
    timeout.tv_usec = 0;
    int rv = select(cli_sockfd + 1, &readfds, NULL, NULL, &timeout);
    int n=0;
    if(rv==0) {
        cout<<"Timeout\n";
        return 10;
    }
    else
     n = read(cli_sockfd, &msg, sizeof(int));
    
    if (n < 0 || n != sizeof(int))  return -1;

    //printf("[DEBUG] Received int: %d\n", msg);
    
    return msg;
}

int recv_int(int cli_sockfd)
{
    int msg = 0;

    int n = read(cli_sockfd, &msg, sizeof(int));
    
    if (n < 0 || n != sizeof(int))  return -1;

    //printf("[DEBUG] Received int: %d\n", msg);
    
    return msg;
}

int get_player_move(int cli_sockfd)
{
    
    
    //Send Turn Signal to the Client
    write_client_msg(cli_sockfd, "TRN");

    //Client will invoke the take_turn function

    //Recieve the box number from client and send to play_game function
    return recv_int_tout(cli_sockfd);
}

int check_move(char board[][3], int move, int player_id)
{
    if (move == 10 ||board[move/3][move%3] == ' ') 
        return 1;
    else 
        return 0;
}

void update_board(char board[][3], int move, int player_id)
{
    board[move/3][move%3] = player_id ? 'X' : 'O';
}

void show_game(char board[][3])
{
    printf(" %c | %c | %c \n", board[0][0], board[0][1], board[0][2]);
    printf("***********\n");
    printf(" %c | %c | %c \n", board[1][0], board[1][1], board[1][2]);
    printf("***********\n");
    printf(" %c | %c | %c \n", board[2][0], board[2][1], board[2][2]);
}

void send_update(int * cli_sockfd, int move, int player_id)
{

    
    //send update to both clients
    write_clients_msg(cli_sockfd, "UPD");

    //get_update function gets triggered
    // It needs player id for X or O calculation and move on board
    //Send to both clients
    write_clients_int(cli_sockfd, player_id);
    
    write_clients_int(cli_sockfd, move);
}

void send_player_count(int cli_sockfd)
{
    write_client_msg(cli_sockfd, "CNT");
    write_client_int(cli_sockfd, player_count);
}

int check_board(char board[][3], int last_move)
{

    int row = last_move/3;
    int col = last_move%3;

    if ( board[row][0] == board[row][1] && board[row][1] == board[row][2] ) { 
        
        return 1;
    }
    else if ( board[0][col] == board[1][col] && board[1][col] == board[2][col] ) { 
        
        return 1;
    }
    else if (!(last_move % 2)) {
        if ( (last_move == 0 || last_move == 4 || last_move == 8) && (board[1][1] == board[0][0] && board[1][1] == board[2][2]) ) {  
        
            return 1;
        }
        if ( (last_move == 2 || last_move == 4 || last_move == 6) && (board[1][1] == board[0][2] && board[1][1] == board[2][0]) ) { 
           
            return 1;
        }
    }
    
    return 0;
}

void accept_clients(int master_socket, int * cli_sockfd)
    //arguments are master socket and 
{
    socklen_t clilen;

    // Socket Address of master socket (server) and clients
    struct sockaddr_in serv_addr, cli_addr; 
    
    int num_conn = 0;
    while(num_conn < 2)
    {
	    // Listen on Master Socket
        listen(master_socket, 253 - player_count);
        
        //Set cli_addr , client socket to 0
        
        memset(&cli_addr, 0, sizeof(cli_addr));

        clilen = sizeof(cli_addr);
        
        //accept connection from client socket to master socket 
        cli_sockfd[num_conn] = accept(master_socket, (struct sockaddr *) &cli_addr, &clilen);

        //cli_sockfd[num_conn] is whats gonna be used while writing to the client

        if (cli_sockfd[num_conn] < 0)
            error("ERROR accepting a connection from a client.");
        
        write(cli_sockfd[num_conn], &num_conn, sizeof(int));
        // Send Client the Client Id
        // num_conn in server side = Client id in Client side
                
        pthread_mutex_lock(&mutexcount);
        player_count++;
        printf("Player Count at present is %d.\n", player_count);
        pthread_mutex_unlock(&mutexcount);

        if (num_conn == 0) 
            write_client_msg(cli_sockfd[0],"HLD"); // make the odd player hold

        num_conn++;
    }
}

void *play_game(void *thread_data) 
{
    //thread_data is argument passed in the thread
    //It has the socket file descriptor of opponet clients

    start: 
    int *cli_sockfd = (int*)thread_data; 
    char board[3][3] = { {' ', ' ', ' '},  {' ', ' ', ' '},   {' ', ' ', ' '} };

    printf("Game on!\n");

    //Give start signal to both clients
    // They will break of the HLD (hold) while loop
    write_clients_msg(cli_sockfd, "SRT"); // Sending start signal to both clients

    show_game(board);
    
    int prev_player_turn = 0;
    int player_turn = 1;
    int game_over = 0;
    int turn_count = 0;

    

    while(!game_over) {
        
        //Send wait signal to the 2nd player
        if (prev_player_turn != player_turn)
            write_client_msg(cli_sockfd[(player_turn + 1) % 2], "WAT");

        int valid = 0;
        int move = 0;

        //While player is making invalid move , keep on trying
        while(!valid) { 
            //player_turn is the current player
            //cli_sockfd[player_turn] is the socket file descriptor of that client

            //Move is the box number  from client

            move = get_player_move(cli_sockfd[player_turn]);
            if (move == -1) break; 
            
                
            printf("Player %d played position %d\n", player_turn, move);
            string str = to_string(player_turn) + " played " + to_string(move) + "\n";
            cout<<str<<endl;
            //cout<<player_turn<<" played "<<move<<endl;
            //fp<<"\n";
            fp<<str<<endl;
                
            valid = check_move(board, move, player_turn);
            if (!valid) { 
                printf("Move was invalid. Let's try this again...\n");
                write_client_msg(cli_sockfd[player_turn], "INV");
            }
        }
        
	    if (move == -1) { 
            printf("Player disconnected.\n");
            break;
        }
        
        else if( move == 10){
            write_clients_msg(cli_sockfd, "TOT");
            
        }
        else {
            //Local server board gets updated
            update_board(board, move, player_turn);
            //Sends X or O by player number and the cell to both clients
            send_update( cli_sockfd, move, player_turn );
                
         
            show_game(board);

            //Checks game over last move
            game_over = check_board(board, move);
            
            if (game_over == 1) {
                //current player wins
                write_client_msg(cli_sockfd[player_turn], "WIN");
                //next player looses
                write_client_msg(cli_sockfd[(player_turn + 1) % 2], "LSE");
                printf("Player %d won.\n", player_turn);


            }
            else if (turn_count == 8) {                
                printf("Draw.\n");
                //Send Draw to both clients
                write_clients_msg(cli_sockfd, "DRW");
                game_over = 1;
            }

            if(game_over == 1 || turn_count == 8 ){
                

                int replay1=0, replay2=0;

                // replay1 = get_player_move(cli_sockfd[player_turn]);
                // replay2 = get_player_move(cli_sockfd[1-player_turn]);

                replay1 = recv_int(cli_sockfd[player_turn]);
                replay2 = recv_int(cli_sockfd[(1 + player_turn)%2]);

                cout<<"replay"<<replay1<<replay2<<endl;

                write(cli_sockfd[(player_turn ) % 2], &replay2, sizeof(int));
                write(cli_sockfd[(player_turn +1 ) % 2], &replay1, sizeof(int));

                // write_client_msg(cli_sockfd[(player_turn ) % 2], (char*)&(to_string(replay2)));
                // write_client_msg(cli_sockfd[(player_turn + 1) % 2], (char*)&(to_string(replay1)));

                if(replay1 && replay2){
                    goto start;
                }
            }

            //cur player becomes prev player
            prev_player_turn = player_turn;
            //cur player is updated
            player_turn = (player_turn + 1) % 2;
            //turns updated
            turn_count++;
        }

        
    }

    printf("Game over.\n");

	close(cli_sockfd[0]);
    close(cli_sockfd[1]);

    pthread_mutex_lock(&mutexcount);
    player_count--;
    printf("Number of players is now %d.", player_count);
    player_count--;
    printf("Number of players is now %d.", player_count);
    pthread_mutex_unlock(&mutexcount);
    
    free(cli_sockfd);

    pthread_exit(NULL);
}



int main(int argc, char *argv[])
{
    
    ::fp.open("log.txt", ios::app|ios::out);
    
    int master_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (master_socket < 0) 
        cout<<"Master Socket ERROR";

    
    int opt = 1;  
    if( setsockopt(master_socket, SOL_SOCKET, SO_REUSEADDR, (char *)&opt, sizeof(opt)) < 0 )  
    {  
        perror("Set Socket Option Error");  
        exit(EXIT_FAILURE);  
    } 

    struct sockaddr_in serv_addr; 
    memset(&serv_addr, '0', sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(5000); 

    bind(master_socket, (struct sockaddr*)&serv_addr, sizeof(serv_addr)); 

    pthread_mutex_init(&mutexcount, NULL); // Initialising pThread
    //master_socket is master socket


    while(1)
    {
        if (player_count <= 252) {

            int *cli_sockfd = (int*)malloc(2*sizeof(int)); 
            memset(cli_sockfd, 0, 2*sizeof(int));
            
            accept_clients(master_socket, cli_sockfd);

            //Create thread to play_game
            pthread_t thread;

            int result = pthread_create(&thread, NULL, play_game, (void *)cli_sockfd);

            if (result){
                printf("Creating Thread failed with return code %d\n", result);
                exit(-1);
            }
            
            printf(" New thread for another game has been started.\n");
            
        }
    }

    close(master_socket);
    pthread_mutex_destroy(&mutexcount); // Destroying pThread
    pthread_exit(NULL);
    ::fp.close();
}