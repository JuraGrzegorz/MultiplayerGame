#include<ncurses.h>
#include<stdlib.h>
#include<pthread.h>
#include <unistd.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#define MAX_BEAST_VAL 10

struct player{
    unsigned int x_position;
    unsigned int y_position;
    unsigned int start_x;
    unsigned int start_y;
    unsigned int PID;
    unsigned int deathes;
    unsigned int coin_carried;
    unsigned int coin_brought;
    char Type;
    unsigned int bush_x;
    unsigned int bush_y;
    unsigned int bush_try;
    char last_char_onmap;

    pthread_mutex_t player_move;
    pthread_t player_thread;
};
struct treasure{
    unsigned int x_position;
    unsigned int y_position;
    unsigned int coins;
    char last_char_onmap;
};
struct beast{
    unsigned int x_position;
    unsigned int y_position;
    char character;
    char last_char_onmap;

    pthread_t beast_thread;
    pthread_mutex_t beast_move;
};
struct server{
    int server_open;
    unsigned int PID;
    unsigned int Campsite_x;
    unsigned int Campsite_y;
    unsigned int round;
    char **map;
    unsigned int map_width;
    unsigned int map_height;
    struct player p[2];
    int val_beast;
    struct beast beast[MAX_BEAST_VAL];
    unsigned int val_dropped_treasure;
    struct treasure *dropped_treasure;
};


void display_map(int width,int height);
void rand_position(char **board,int width,int height,unsigned int *x,unsigned int *y);
char **load_map(char *filename,unsigned int *width,unsigned int *height);
int communication_first(struct server *s1,struct player *p2);
void start_key_getch(int *val);
void * key_getch(void *val);
void * beast_move(void *new);
void add_beast();
void server_close();
void add_coin(char coin);
void server_open();
void start_local_player(int *val);
void *local_player(void *val);
void player_move(struct player *p,int move);
void start_online_player();
void *online_player(void *val);

struct server server;

pthread_mutex_t display;

int main(void)
{
    server_open();
    int key_val=0;
    start_key_getch(&key_val);
    start_local_player(&key_val);
    start_online_player();
    while(1){
        if(key_val=='Q' || key_val=='q'){
            break;
        }
        for(int i=0;i<2;i++){
            pthread_mutex_unlock(&server.p[i].player_move);
        }
        for(int i=0;i<server.val_beast;i++){
            pthread_mutex_unlock(&((server.beast + i)->beast_move));
        }
        server.round+=1;
        clear();
        display_map(server.map_width,server.map_height);
        refresh();
        sleep(1);
    }
    server_close();
    return 0;
}

void display_map(int width,int height)
{
    pthread_mutex_lock(&display);
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

            for(int j=0;j<height;j++){
                for(int i=0;i<width;i++){
                    move(j,i);
                    if(*(*(server.map+j)+i)==' '){
                        attrset(COLOR_PAIR(1));
                        addch(' ');
                        attroff(COLOR_PAIR(1));
                    }
                    if(*(*(server.map+j)+i)=='*'){
                        attrset(COLOR_PAIR(3));
                        addch(*(*(server.map+j)+i));
                        attroff(COLOR_PAIR(3));
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
            mvprintw(4,54,"Campsite X/Y: %i/%i",server.Campsite_x,server.Campsite_y);
            mvprintw(5 ,54,"Round number: %i",server.round);
            mvprintw(6,53,"Parameter:   Player1   Player2");
            mvprintw(7,54,"PID");
            mvprintw(8,54,"Type");
            mvprintw(9,54,"Curr X/Y");
            mvprintw(10,54,"Deaths");
            mvprintw(7,66,"%i",server.p[0].PID);
            mvprintw(7,76,"%i",server.p[1].PID);
            mvprintw(8,66,"HUMAN     HUMAN");
            mvprintw(9,66,"%i/%i      %i/%i",server.p[0].x_position,server.p[0].y_position,server.p[1].x_position,server.p[1].y_position);
            mvprintw(10,66,"%i",server.p[0].deathes);
            mvprintw(10,76,"%i",server.p[1].deathes);
            mvprintw(13,54,"Coins ");
            mvprintw(14,58,"carried");
            mvprintw(15,58,"brought");
            mvprintw(14,66,"%i",server.p[0].coin_carried);
            mvprintw(14,76,"%i",server.p[1].coin_carried);
            mvprintw(15,66,"%i",server.p[0].coin_brought);
            mvprintw(15,76,"%i",server.p[1].coin_brought);
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
    pthread_mutex_unlock(&display);
}
void add_coin(char coin){
    int x,y;
    rand_position(server.map,server.map_width,server.map_height,&x,&y);
    pthread_mutex_lock(&display);
    *(*(server.map+y)+x)=coin;
    pthread_mutex_unlock(&display);
}
char **load_map(char *filename,unsigned int *width,unsigned int *height){
    if(filename==0){
        return 0;
    }
    FILE *f= fopen(filename,"r");
    if(f==0){
        return 0;
    }

    if(fscanf(f,"%d\n",height)==0){
        fclose(f);
        return 0;
    }
    if(fscanf(f,"%d\n",width)==0){
        fclose(f);
        return 0;
    }
    char **res= calloc(*height,sizeof(char *));
    if(res==0){
        fclose(f);
        return 0;
    }

    for(int x=0;x<*height;x++){
        *(res+x)= calloc(*width,sizeof(char));
        if(*(res+x)==0){
            fclose(f);
            for(int y=0;y<x;y++){
                free(*(res+y));
            }
            return 0;
        }
    }
    char tmp;
    for(int x=0;x<*height;x++){
        for(int y=0;y<*width;y++){
            fscanf(f,"%c",&tmp);
            *(*(res+x)+y)=tmp;
        }
        getc(f);
    }
    fclose(f);
    return res;
}
void rand_position(char **board,int width,int height,unsigned int *x,unsigned int *y){
    if(board==0 || x==0 ||  y==0){
        return;
    }
    srand( time( NULL ) );
    *x=0,*y=0;
    while (1){
        if(*(*(board+*y)+*x)==' '){
            break;
        }
        *x=rand()%width;
        *y=rand()%height;
    }

    return;
}
int communication_first(struct server *s1,struct player *p2){
    if(s1==0 || p2==0){
        return 1;
    }
    mkfifo("/tmp/to_connect",0777);
    mkfifo("/tmp/feedback",0777);
    int id=open("/tmp/to_connect",O_RDONLY);
    read(id,&p2->PID,sizeof(p2->PID));
    close(id);

    id=open("/tmp/feedback",O_WRONLY);
    write(id,&s1->PID,sizeof(s1->PID));
    close(id);
    mkfifo("/tmp/c_to_server",0777);
    mkfifo("/tmp/c_from_server",0777);
    remove("/tmp/to_connect");
    remove("/tmp/feedback");
    return 0;
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
        if(*(char *)val=='c' || *(char *)val=='t' || *(char *)val=='T'){
            add_coin(*(char *)val);
            *(char *)val=0;
        }
        if(*(char *)val=='B' || *(char *)val=='b'){
            add_beast();
            *(char *)val=0;
        }
    }
    pthread_exit(NULL);
}
void add_beast() {
    if(server.val_beast>=MAX_BEAST_VAL){
        return;
    }
    (server.beast + server.val_beast)->character='*';
    (server.beast + server.val_beast)->last_char_onmap=' ';
    rand_position(server.map, server.map_width, server.map_height, &((server.beast + server.val_beast)->x_position), &((server.beast + server.val_beast)->y_position));
    *(*(server.map+(server.beast + server.val_beast)->y_position) + (server.beast + server.val_beast)->x_position)=(server.beast + server.val_beast)->character;
    pthread_create(&((server.beast + server.val_beast)->beast_thread), NULL, beast_move,
                   (server.beast + server.val_beast));
    pthread_mutex_init(&((server.beast + server.val_beast)->beast_move), NULL);
    pthread_mutex_lock(&((server.beast + server.val_beast)->beast_move));
    pthread_detach(((server.beast + server.val_beast)->beast_thread));
    server.val_beast=server.val_beast+1;
}
void * beast_move(void *new) {
    struct beast *beast=(struct beast *)new;
    while (1) {
        pthread_mutex_lock(&(*beast).beast_move);
        if(server.server_open==1){
            pthread_mutex_lock(&display);
            int x=0,y=0;
            srand( time( NULL ) );
            int val=-1;

            if(*(*(server.map+beast->y_position)+beast->x_position+1)=='1' || *(*(server.map+beast->y_position)+beast->x_position+1)=='2'){
                val=0;
            }
            if(*(*(server.map+beast->y_position+1)+beast->x_position)=='1' || *(*(server.map+beast->y_position+1)+beast->x_position)=='2'){
                val=1;
            }
            if(*(*(server.map+beast->y_position)+beast->x_position-1)=='1' || *(*(server.map+beast->y_position)+beast->x_position-1)=='2'){
                val=2;
            }
            if(*(*(server.map+beast->y_position-1)+beast->x_position)=='1' || *(*(server.map+beast->y_position-1)+beast->x_position)=='2'){
                val=3;
            }
            if(val==0){
                x=1;
                y=0;
            }
            if(val==1){
                x=0;
                y=1;
            }
            if(val==2){
                x=-1;
                y=0;
            }
            if(val==3){
                x=0;
                y=-1;
            }
            struct player *p;
            if(val!=-1){
                if(server.p[0].x_position==beast->x_position+x && server.p[0].y_position==beast->y_position+y){
                    p=&server.p[0];
                }else{
                    p=&server.p[1];
                }
                struct treasure *tmp_d=realloc(server.dropped_treasure,sizeof(struct treasure)*(server.val_dropped_treasure+1));
                if(tmp_d==0){
                    pthread_mutex_unlock(&display);
                    break;
                }
                server.dropped_treasure=tmp_d;
                (server.dropped_treasure+server.val_dropped_treasure)->last_char_onmap=p->last_char_onmap;
                (server.dropped_treasure+server.val_dropped_treasure)->x_position=p->x_position;
                (server.dropped_treasure+server.val_dropped_treasure)->y_position=p->y_position;
                (server.dropped_treasure+server.val_dropped_treasure)->coins=p->coin_carried;
                *(*(server.map+p->y_position)+p->x_position)='*';
                p->x_position=p->start_x;
                p->y_position=p->start_y;
                *(*(server.map+p->y_position)+p->x_position)=p->Type;
                p->coin_carried=0;
                (p->deathes)++;
                p->last_char_onmap=' ';
                *(*(server.map+beast->y_position)+beast->x_position)=beast->last_char_onmap;
                beast->last_char_onmap='D';
                beast->x_position+=x;
                beast->y_position+=y;
            }else{
                for(int i=0;i<2;i++){
                    char tmp=*(*(server.map+beast->y_position-i)+beast->x_position);
                    if(tmp==' ' || tmp=='c' || tmp=='t' || tmp=='T' || tmp=='D'){
                        if(*(*(server.map+beast->y_position-i-1)+beast->x_position)=='1' || *(*(server.map+beast->y_position-i-1)+beast->x_position)=='2'){
                            val=3;
                            break;
                        }
                    }
                    tmp=*(*(server.map+beast->y_position+i)+beast->x_position);
                    if(tmp==' ' || tmp=='c' || tmp=='t' || tmp=='T' || tmp=='D'){
                        if(*(*(server.map+beast->y_position+i+1)+beast->x_position)=='1' || *(*(server.map+beast->y_position+i+1)+beast->x_position)=='2'){
                            val=1;
                            break;
                        }
                    }
                    tmp=*(*(server.map+beast->y_position)+beast->x_position+i);
                    if(tmp==' ' || tmp=='c' || tmp=='t' || tmp=='T' || tmp=='D'){
                        if(*(*(server.map+beast->y_position)+beast->x_position+x+1)=='1' || *(*(server.map+beast->y_position)+beast->x_position+x+1)=='2'){
                            val=0;
                            break;
                        }
                    }
                    tmp=*(*(server.map+beast->y_position-1)+beast->x_position-i);
                    if(tmp==' ' || tmp=='c' || tmp=='t' || tmp=='T' || tmp=='D'){
                        if(*(*(server.map+beast->y_position)+beast->x_position-i-1)=='1' || *(*(server.map+beast->y_position)+beast->x_position-i-1)=='2'){
                            val=3;
                            break;
                        }
                    }
                }
            }
            if(val==-1) {
                while (1) {
                    val = rand() % 4;
                    if (val == 0) {
                        x = 1;
                        y = 0;
                    }
                    if (val == 1) {
                        x = 0;
                        y = 1;
                    }
                    if (val == 2) {
                        x = -1;
                        y = 0;
                    }
                    if (val == 3) {
                        x = 0;
                        y = -1;
                    }
                    if (*(*(server.map + (beast->y_position) + y) + (beast->x_position) + x) == ' ' ||
                        *(*(server.map + (beast->y_position) + y) + (beast->x_position) + x) == 'c' ||
                        *(*(server.map + (beast->y_position) + y) + (beast->x_position) + x) == 't' ||
                        *(*(server.map + (beast->y_position) + y) + (beast->x_position) + x) == 'T' ||
                        *(*(server.map + (beast->y_position) + y) + (beast->x_position) + x) == '#' ||
                        *(*(server.map + (beast->y_position) + y) + (beast->x_position) + x) == 'D') {
                        break;
                    }
                }
            }else{
                if (val == 0) {
                    x = 1;
                    y = 0;
                }
                if (val == 1) {
                    x = 0;
                    y = 1;
                }
                if (val == 2) {
                    x = -1;
                    y = 0;
                }
                if (val == 3) {
                    x = 0;
                    y = -1;
                }
            }
                *(*(server.map+beast->y_position)+beast->x_position)=beast->last_char_onmap;
                beast->x_position+=x;
                beast->y_position+=y;
                beast->last_char_onmap=*(*(server.map+beast->y_position)+beast->x_position);
                *(*(server.map+beast->y_position)+beast->x_position)='*';
                server.val_dropped_treasure++;
            pthread_mutex_unlock(&display);
        }else{
            break;
        }
    }
    pthread_exit(NULL);
}
void start_local_player(int *val){
    pthread_create(&server.p[0].player_thread, NULL, local_player, val);
    pthread_mutex_init(&server.p[0].player_move,NULL);
    pthread_mutex_lock(&server.p[0].player_move);
    pthread_detach(server.p[0].player_thread);
}
void *local_player(void *val){
    int *k_move=(int *)val;
    while (1) {
        pthread_mutex_lock(&server.p[0].player_move);
        if (server.server_open==1) {
            pthread_mutex_lock(&display);
            if(*k_move==259 || *k_move== 258 || *k_move==260 || *k_move==261){
                player_move(&server.p[0],*((int *)val));
                *((int *)val)=0;
            }
            pthread_mutex_unlock(&display);
        }else{
            break;
        }
    }
    pthread_exit(NULL);
}
void start_online_player(){
    pthread_create(&server.p[1].player_thread, NULL, online_player, NULL);
    pthread_mutex_init(&server.p[1].player_move,NULL);
    pthread_mutex_lock(&server.p[1].player_move);
    pthread_detach(server.p[1].player_thread);
}
void *online_player(void *val){
    while(1){
        pthread_mutex_lock(&server.p[1].player_move);
        if (server.server_open==1) {
            pthread_mutex_lock(&display);
            int id_w=open("/tmp/c_from_server",O_WRONLY);
            write(id_w,&server.p[1].x_position,sizeof(unsigned int));
            write(id_w,&server.p[1].y_position,sizeof(unsigned int));
            write(id_w,&server.p[1].deathes,sizeof(unsigned int));
            write(id_w,&server.p[1].coin_carried,sizeof(unsigned int));
            write(id_w,&server.p[1].coin_brought,sizeof(unsigned int));
            write(id_w,&server.round,sizeof(unsigned int));
            char space=' ';
            for(int i=0;i<5;i++){
                for(int j=0;j<5;j++){
                    if(server.p[1].y_position-2+i<0 || server.p[1].x_position-2+j<0 || server.p[1].y_position+i-2>=server.map_height){
                        write(id_w,&space,sizeof(char));
                    }else{
                        write(id_w,(*(server.map+server.p[1].y_position+i-2)+server.p[1].x_position+j-2),sizeof(char));
                    }
                }
            }
            close(id_w);
            int id_r=open("/tmp/c_to_server",O_RDONLY);
            int move=0;
            read(id_r,&move,sizeof(int));
            if(move== 259 || move==258 || move==260 || move==261){
                player_move(&server.p[1],move);
            }
            close(id_r);
            pthread_mutex_unlock(&display);
        }else{
            break;
        }
    }
    pthread_exit(NULL);
}
void player_move(struct player *p,int move){

    int x,y;
    if(move==259){
        x=0;
        y=-1;
    }
    if(move==258){
        x=0;
        y=1;
    }
    if(move==260){
        x=-1;
        y=0;
    }
    if(move==261){
        x=1;
        y=0;
    }
    if(p->last_char_onmap=='c' || p->last_char_onmap=='t' || p->last_char_onmap=='T'){
        p->last_char_onmap=' ';
    }
    if(*(*(server.map+(p->y_position)+y)+(p->x_position)+x)=='w'){
        return;
    }
    if(*(*(server.map+(p->y_position)+y)+(p->x_position)+x)=='#'){
        if(p->y_position+y==p->bush_y && p->x_position+x==p->bush_x && p->bush_try==1){
            *(*(server.map+(p->y_position))+(p->x_position))=p->last_char_onmap;
            p->x_position+=x;
            p->y_position+=y;
            p->last_char_onmap=*(*(server.map+(p->y_position))+(p->x_position));
            *(*(server.map+(p->y_position))+(p->x_position))=p->Type;
            p->bush_try=0;
        }else{
            p->bush_y=(p->y_position)+y;
            p->bush_x=(p->x_position)+x;
            p->bush_try=1;
        }
        return;
    }
    if(*(*(server.map+(p->y_position)+y)+(p->x_position)+x)==' '){
        *(*(server.map+(p->y_position))+(p->x_position))=p->last_char_onmap;
        p->x_position+=x;
        p->y_position+=y;
        p->last_char_onmap=*(*(server.map+(p->y_position))+(p->x_position));
        *(*(server.map+(p->y_position))+(p->x_position))=p->Type;
        return;
    }
    if(*(*(server.map+(p->y_position)+y)+(p->x_position)+x)=='A'){
        *(*(server.map+(p->y_position))+(p->x_position))=p->last_char_onmap;
        p->x_position+=x;
        p->y_position+=y;
        p->last_char_onmap=*(*(server.map+(p->y_position))+(p->x_position));
        *(*(server.map+(p->y_position))+(p->x_position))=p->Type;
        p->coin_brought+=p->coin_carried;
        p->coin_carried=0;
        return;
    }
    if(*(*(server.map+(p->y_position)+y)+(p->x_position)+x)=='c'){
        *(*(server.map+(p->y_position))+(p->x_position))=p->last_char_onmap;;
        p->x_position+=x;
        p->y_position+=y;
        p->last_char_onmap=*(*(server.map+(p->y_position))+(p->x_position));
        *(*(server.map+(p->y_position))+(p->x_position))=p->Type;
        p->coin_carried+=1;
        return;
    }

    if(*(*(server.map+(p->y_position)+y)+(p->x_position)+x)=='t'){
        *(*(server.map+(p->y_position))+(p->x_position))=p->last_char_onmap;;
        p->x_position+=x;
        p->y_position+=y;
        p->last_char_onmap=*(*(server.map+(p->y_position))+(p->x_position));
        *(*(server.map+(p->y_position))+(p->x_position))=p->Type;
        p->coin_carried+=10;
        return;
    }
    if(*(*(server.map+(p->y_position)+y)+(p->x_position)+x)=='T'){
        *(*(server.map+(p->y_position))+(p->x_position))=p->last_char_onmap;;
        p->x_position+=x;
        p->y_position+=y;
        p->last_char_onmap=*(*(server.map+(p->y_position))+(p->x_position));
        *(*(server.map+(p->y_position))+(p->x_position))=p->Type;
        p->coin_carried+=50;
        return;
    }
    if(*(*(server.map+(p->y_position)+y)+(p->x_position)+x)=='1' || *(*(server.map+(p->y_position)+y)+(p->x_position)+x)=='2'){
        struct treasure *tmp= realloc(server.dropped_treasure,sizeof(struct treasure)*(server.val_dropped_treasure+1));
        if(tmp==0){
            return;
        }
        server.dropped_treasure=tmp;
        (server.dropped_treasure+server.val_dropped_treasure)->coins=server.p[0].coin_carried+server.p[1].coin_carried;
        (server.dropped_treasure+server.val_dropped_treasure)->x_position=(p->x_position)+x;
        (server.dropped_treasure+server.val_dropped_treasure)->y_position=(p->y_position)+y;
        *(*(server.map+(p->y_position)+y)+(p->x_position)+x)='D';
        *(*(server.map+(p->y_position))+(p->x_position))=p->last_char_onmap;
        if(p==&server.p[0]){
            (server.dropped_treasure+server.val_dropped_treasure)->last_char_onmap=server.p[1].last_char_onmap;
        }else{
            (server.dropped_treasure+server.val_dropped_treasure)->last_char_onmap=server.p[0].last_char_onmap;
        }
        server.p[0].x_position=server.p[0].start_x;
        server.p[0].y_position=server.p[0].start_y;
        server.p[1].x_position=server.p[1].start_x;
        server.p[1].y_position=server.p[1].start_y;
        server.p[0].last_char_onmap=*(*(server.map+(server.p[0].start_y))+(server.p[0].start_x));
        server.p[1].last_char_onmap=*(*(server.map+(server.p[1].start_y))+(server.p[1].start_x));
        *(*(server.map+(server.p[0].start_y))+(server.p[0].start_x))='1';
        *(*(server.map+(server.p[1].start_y))+(server.p[1].start_x))='2';
        server.val_dropped_treasure++;
        server.p[0].coin_carried=0;
        server.p[1].coin_carried=0;
        server.p[0].deathes++;
        server.p[1].deathes++;
        return;
    }
    if(*(*(server.map+(p->y_position)+y)+(p->x_position)+x)=='D'){
        int i=0;
        while(i<server.val_dropped_treasure){
            if((server.dropped_treasure+i)->x_position==(p->x_position)+x &&  (server.dropped_treasure+i)->y_position==(p->y_position)+y){
                break;
            }
            i++;
        }
        p->coin_carried+=(server.dropped_treasure+i)->coins;
        *(*(server.map+(p->y_position))+(p->x_position))=p->last_char_onmap;
        *(*(server.map+(p->y_position)+y)+(p->x_position)+x)=p->Type;
        p->last_char_onmap=(server.dropped_treasure+i)->last_char_onmap;
        p->x_position+=x;
        p->y_position+=y;
        if(i+1!=server.val_dropped_treasure){
            (server.dropped_treasure+i)->last_char_onmap=(server.dropped_treasure+server.val_dropped_treasure-1)->last_char_onmap;
            (server.dropped_treasure+i)->coins=(server.dropped_treasure+server.val_dropped_treasure-1)->coins;
            (server.dropped_treasure+i)->x_position=(server.dropped_treasure+server.val_dropped_treasure-1)->x_position;
            (server.dropped_treasure+i)->y_position=(server.dropped_treasure+server.val_dropped_treasure-1)->y_position;
        }
        (server.dropped_treasure+server.val_dropped_treasure-1)->y_position=0;
        (server.dropped_treasure+server.val_dropped_treasure-1)->x_position=0;
        (server.dropped_treasure+server.val_dropped_treasure-1)->coins=0;
        (server.dropped_treasure+server.val_dropped_treasure-1)->last_char_onmap=0;
        struct treasure *tmp=realloc(server.dropped_treasure,sizeof(struct treasure)*(server.val_dropped_treasure-1));
        if(tmp==0){
            return;
        }
        server.dropped_treasure=tmp;
        server.val_dropped_treasure--;
        return;
    }
}
void server_close(){
    endwin();
    int val=0;
    int id_w=open("/tmp/c_from_server",O_WRONLY);
    write(id_w,&val,sizeof(char));
    close(id_w);
    pthread_mutex_unlock(&server.p[0].player_move);
    pthread_mutex_unlock(&server.p[1].player_move);
    for(int i=0;i<server.val_beast;i++){
        pthread_mutex_unlock(&((server.beast + i)->beast_move));
    }
    for(int i=0;i<server.val_beast;i++){
        pthread_mutex_destroy(&((server.beast + i)->beast_move));
    }

    for(int i=0;i<server.map_height;i++){
        free(*(server.map+i));
    }
    free(server.map);
    free(server.dropped_treasure);
    pthread_mutex_destroy(&server.p[0].player_move);
    pthread_mutex_destroy(&server.p[1].player_move);
    pthread_mutex_destroy(&display);
    remove("/tmp/c_to_server");
    remove("/tmp/c_from_server");
}
void server_open(){
    initscr();
    noecho();
    keypad(stdscr,true);
    pthread_mutex_init(&display,NULL);
    server.map= load_map("map.txt",&server.map_width,&server.map_height);
    server.val_beast=0;
    server.round=0;
    server.server_open=1;
    server.PID=getpid();
    server.Campsite_x=35;
    server.Campsite_y=11;
    server.p[0].PID=server.PID;
    server.p[0].coin_brought=0;
    server.p[0].coin_carried=0;
    server.p[0].deathes=0;
    server.p[0].Type='1';
    server.p[0].bush_try=0;
    server.p[0].bush_x=0;
    server.p[0].bush_y=0;
    server.p[0].last_char_onmap=' ';
    server.p[1].coin_brought=0;
    server.p[1].coin_carried=0;
    server.p[1].deathes=0;
    server.p[1].Type='2';
    server.p[1].bush_try=0;
    server.p[1].bush_x=0;
    server.p[1].bush_y=0;
    server.p[1].last_char_onmap=' ';
    server.dropped_treasure=0;
    server.val_dropped_treasure=0;
    rand_position(server.map,server.map_width,server.map_height,&(server.p[0].start_x),&(server.p[0].start_y));
    server.p[0].x_position=server.p[0].start_x;
    server.p[0].y_position=server.p[0].start_y;
    *(*(server.map+server.p[0].start_y)+server.p[0].start_x)=server.p[0].Type;
    rand_position(server.map,server.map_width,server.map_height,&(server.p[1].start_x),&(server.p[1].start_y));
    server.p[1].x_position=server.p[1].start_x;
    server.p[1].y_position=server.p[1].start_y;
    *(*(server.map+server.p[1].start_y)+server.p[1].start_x)=server.p[1].Type;

    for(int i=0;i<15;i++){
        add_coin('c');
    }
    for(int i=0;i<5;i++){
        add_coin('t');
    }
    for(int i=0;i<2;i++){
        add_coin('T');
    }
    printw("waiting for player!!");
    refresh();
    communication_first(&server,&server.p[1]);
    add_beast();
}
