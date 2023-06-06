#include<ncurses.h>
#include<stdlib.h>
#include<pthread.h>
#include <unistd.h>
#include <sys/types.h>
#include <fcntl.h>


struct player{
    unsigned int x_position;
    unsigned int y_position;
    unsigned int PID;
    char Type;
    unsigned int deaths;
    unsigned int coin_carried;
    unsigned int coin_brought;

    pthread_mutex_t player_move;
    pthread_t player_thread;
};

struct server{
    char **map;
    int server_open;
    unsigned int map_width;
    unsigned int map_height;
    unsigned int rounds;
    struct player p;
    int PID;
};
void server_connect(struct player *p2);
void start_key_getch(int *val);
void * key_getch(void *val);
void client_start();
void display_map();
void client_close();

struct server server;

int main(void)
{
    client_start();
    int key_val=0;
    start_key_getch(&key_val);
    while(1){
        if(key_val=='Q' || key_val=='q'){
            break;
        }
        int id_r= open("/tmp/c_from_server",O_RDWR);
        read(id_r,&server.p.x_position,sizeof(unsigned int));
        if(server.p.x_position==0){
            server.server_open=0;
            break;
        }
        read(id_r,&server.p.y_position,sizeof(unsigned int));
        read(id_r,&server.p.deaths,sizeof(unsigned int));
        read(id_r,&server.p.coin_carried,sizeof(unsigned int));
        read(id_r,&server.p.coin_brought,sizeof(unsigned int));
        read(id_r,&server.rounds,sizeof(unsigned int));
        for(int i=0;i<5;i++){
            for(int j=0;j<5;j++){
                read(id_r,(*(server.map+i)+j),sizeof(char));
            }
        }
        close(id_r);
        int id_w= open("/tmp/c_to_server",O_WRONLY);
        write(id_w,&key_val,sizeof(int));
        key_val=0;
        close(id_w);
        clear();
        display_map();
        refresh();
        sleep(1);
    }
    client_close();
    return 0;
}
void client_start(){
    initscr();
    server.server_open=1;
    keypad(stdscr,true);
    server_connect(&server.p);
    server.rounds=0;
    server.map_height=5;
    server.map_width=5;
    server.map= calloc(5,sizeof(char *));
    for(int i=0;i<5;i++){
        *(server.map+i)= calloc(5,sizeof(char));
    }
}
void client_close(){
    endwin();
    pthread_mutex_destroy(&server.p.player_move);
    for(int i=0;i<server.map_height;i++){
        free(*(server.map+i));
    }
    free(server.map);
}
void server_connect(struct player *p){
    if(p==0){
        return;
    }
    int id=open("/tmp/to_connect",O_WRONLY);
    int pid=getpid();
    server.p.PID=pid;
    write(id,&pid,sizeof(pid));
    close(id);
    id=open("/tmp/feedback",O_RDONLY);
    read(id,&(server.PID),sizeof(server.PID));
    close(id);
    p->Type='2';
}
void start_key_getch(int *val) {
    pthread_t start_key_getch;
    pthread_create(&start_key_getch, NULL, key_getch, val);
    pthread_detach(start_key_getch);
}
void * key_getch(void *val) {
    while (1) {
        *(int *)val = getch();
        if (*(char *)val == 'q' || *(char *)val == 'Q'){
            server.server_open=0;
            break;
        }
    }
    pthread_exit(NULL);
}
void display_map(){
    if(has_colors())
    {
        if(start_color() == OK)
        {
            init_pair(1, COLOR_WHITE, COLOR_WHITE);
            init_pair(2, COLOR_BLACK, COLOR_BLACK);
            init_pair(3, COLOR_BLACK, COLOR_WHITE);
            init_pair(4, COLOR_YELLOW, COLOR_BLUE);
            init_pair(5, COLOR_BLACK, COLOR_GREEN);
            init_pair(6, COLOR_YELLOW, COLOR_RED);
            init_pair(10, COLOR_BLACK,COLOR_GREEN);

            for(int j=0;j<server.map_height;j++){
                for(int i=0;i<server.map_width;i++){
                    move(j,i);
                    if(*(*(server.map+j)+i)==' '){
                        attrset(COLOR_PAIR(1));
                        addch(' ');
                        attroff(COLOR_PAIR(1));
                    }
                    if(*(*(server.map+j)+i)=='*'){
                        attrset(COLOR_PAIR(6));
                        addch(*(*(server.map+j)+i));
                        attroff(COLOR_PAIR(6));
                    }
                    if(*(*(server.map+j)+i)=='w'){
                        attrset(COLOR_PAIR(2));
                        addch(*(*(server.map+j)+i));
                        attroff(COLOR_PAIR(2));
                    }
                    if(*(*(server.map+j)+i)=='#'){
                        attrset(COLOR_PAIR(3));
                        addch(*(*(server.map+j)+i));
                        attroff(COLOR_PAIR(3));
                    }
                    if(*(*(server.map+j)+i)=='c' || *(*(server.map+j)+i)=='D' || *(*(server.map+j)+i)=='t' || *(*(server.map+j)+i)=='T'){
                        attrset(COLOR_PAIR(4));
                        addch(*(*(server.map+j)+i));
                        attroff(COLOR_PAIR(4));
                    }
                    if(*(*(server.map+j)+i)=='A'){
                        attrset(COLOR_PAIR(5));
                        addch(*(*(server.map+j)+i));
                        attroff(COLOR_PAIR(5));
                    }
                    if(*(*(server.map+j)+i)=='1' || *(*(server.map+j)+i)=='2' || *(*(server.map+j)+i)=='3' || *(*(server.map+j)+i)=='4'){
                        attrset(COLOR_PAIR(6));
                        addch(*(*(server.map+j)+i));
                        attroff(COLOR_PAIR(6));
                    }
                }
            }
            mvprintw(3 ,53,"Server's PID: %i",server.PID);
            mvprintw(4,54,"Campsite X/Y: unknow");
            mvprintw(5 ,54,"Round number: %i",server.rounds);
            mvprintw(6,53,"Parameter:   Player");
            mvprintw(8,54,"Type");
            mvprintw(9,54,"Curr X/Y");
            mvprintw(10,54,"Deaths");
            mvprintw(8,66,"HUMAN");
            mvprintw(9,66,"%i/%i",server.p.x_position,server.p.y_position);
            mvprintw(10,66,"%i",server.p.deaths);
            mvprintw(17,53,"legend:");
            attrset(COLOR_PAIR(6));
            mvprintw(18,54,"12");
            attroff(COLOR_PAIR(6));
            mvprintw(18,57,"- players");
            attrset(COLOR_PAIR(2));
            mvprintw(19,54,"w");
            attroff(COLOR_PAIR(2));
            mvprintw(19,57,"- wall");
            attrset(COLOR_PAIR(3));
            mvprintw(19,54,"#");
            attroff(COLOR_PAIR(3));
            mvprintw(19,57,"- bushes (slow down)");
            attrset(COLOR_PAIR(3));
            mvprintw(20,54,"*");
            attroff(COLOR_PAIR(3));
            mvprintw(20,57,"- wild beast");
            attrset(COLOR_PAIR(4));
            mvprintw(21,54,"c");
            attroff(COLOR_PAIR(4));
            mvprintw(21,57,"- one coin");
            attrset(COLOR_PAIR(4));
            mvprintw(22,54,"t");
            attroff(COLOR_PAIR(4));
            mvprintw(22,57,"- treasure (10 coins)");
            attrset(COLOR_PAIR(4));
            mvprintw(23,54,"T");
            attroff(COLOR_PAIR(4));
            mvprintw(23,57,"- large treasure (50 coins)");
            attrset(COLOR_PAIR(5));
            mvprintw(24,54,"A");
            attroff(COLOR_PAIR(5));
            mvprintw(24,57,"- campsite");
            attrset(COLOR_PAIR(4));
            mvprintw(24,54,"D");
            attroff(COLOR_PAIR(4));
            mvprintw(24,57,"- dropped treasure ");
        }
    }
}


