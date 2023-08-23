#include <netinet/in.h>
#include <sys/socket.h>
#include <fcntl.h>

#include <event2/event.h>
#include <event2/buffer.h>
#include <event2/bufferevent.h>

#include <assert.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <stdbool.h>
#include <string.h>
#include <iostream>
#include <vector>
#include <string>

#define MAX_LINE 16384

struct Ticket
{
    int fd;
    int ticketId;
    int gameId;
    int choices[6];
};

struct Game
{
    struct event_base *base;
    bool acceptConnections;
    std::vector<Ticket> tickets;
    int lastAssignedTicketId;
    int gameId;
};


//format of input expected is  : n:1,2,3,4,5,6 where n is the descriptor number of accepted socket and returned to client 
void on_read(struct bufferevent *bev, void *arg)
{
    struct evbuffer *input, *output;
    char *line;
    size_t n;
    int i;

    input = bufferevent_get_input(bev);
    output = bufferevent_get_output(bev);

    struct Game *game = (struct Game *)arg;

    line = evbuffer_readln(input, &n, EVBUFFER_EOL_LF);

    std::vector<int> choices;
    char *pt;

    pt = strtok(line, ":");
    int fd = atoi(pt);
    pt = strtok(NULL, ":");

    pt = strtok(pt, ",");
    while (pt != NULL)
    {
        int a = atoi(pt);
        choices.push_back(a);
        pt = strtok(NULL, ",");
    }

    if (choices.size() == 6)
    {
        struct Ticket ticket;
        memcpy(ticket.choices, &choices[0], sizeof(choices));
        ticket.gameId = game->gameId;
        ticket.fd = fd;
        ticket.ticketId = ++game->lastAssignedTicketId;
        game->tickets.push_back(ticket);

        std::string s = "Congrats on registering ticket with id " + std::to_string(ticket.ticketId) + "  and choices: ";
        evbuffer_add(output, s.c_str(), s.size());
        for(int i = 0; i < choices.size(); i++){
            
            evbuffer_add(output, (" " + std::to_string(choices[i]) + " ").c_str(), (" " + std::to_string(choices[i]) + " ").size());
        }
        evbuffer_add(output, "\n", 1);
    }
}

void on_accept(evutil_socket_t listener, short event, void *arg)
{
    struct Game *game = (struct Game *)arg;

    if (game->acceptConnections)
    {
        struct sockaddr_storage ss;
        socklen_t slen = sizeof(ss);
        int fd = accept(listener, (struct sockaddr *)&ss, &slen);
        struct bufferevent *bev;
        bev = bufferevent_socket_new(game->base, fd, BEV_OPT_CLOSE_ON_FREE);

        struct evbuffer *output;
        output = bufferevent_get_output(bev);
        evbuffer_add(output, "Your assigned id is: ", std::string("Your assigned id is: ").size());
        evbuffer_add(output, std::to_string(fd).c_str(), std::to_string(fd).size());
        evbuffer_add(output, "\n", 1);

        bufferevent_setcb(bev, on_read, NULL, NULL, (void *)game);
        bufferevent_setwatermark(bev, EV_READ, 0, MAX_LINE);
        bufferevent_enable(bev, EV_READ);
    }
    else
    {
        printf("Connection refused\n");
    }
}

void cb_func(evutil_socket_t fd, short what, void *arg)
{
    struct Game *game = (struct Game *)arg;

    game->acceptConnections = !game->acceptConnections;
    if (!game->acceptConnections)
    {
        srand(time(NULL));

        // Comment if you want to generate random solution
        std::vector<int> solution;
        solution.push_back(1);
        solution.push_back(2);
        solution.push_back(3);
        solution.push_back(4);
        solution.push_back(5);
        solution.push_back(6);

        // Uncomment to generate random solution
        // for (int i = 0; i < 6; i++)
        // {
        //     int x = rand() % 49 + 1;
        //     solution.push_back(x);
        // }
        
        for (int i = 0; i < game->tickets.size(); i++)
        {
            std::vector<int> mv(std::begin(solution), std::end(solution));
            std::vector<int> ma(std::begin(game->tickets[i].choices), std::end(game->tickets[i].choices));
            std::sort(mv.begin(), mv.end());
            std::sort(ma.begin(), ma.end());

            if (mv == ma)
            {
                struct evbuffer *output;
                struct bufferevent *bev;
                bev = bufferevent_socket_new(game->base, game->tickets[i].fd, BEV_OPT_CLOSE_ON_FREE);
                output = bufferevent_get_output(bev);
                evbuffer_add(output, "You won!\n", std::string("You won!\n").size());
                evbuffer_add(output, "\n", 1);
            }
        }
        game->tickets.clear();
        game->lastAssignedTicketId = 0;
        game->gameId++;
    }
}

int main(int argc, char **argv)
{
    setvbuf(stdout, NULL, _IONBF, 0);

    evutil_socket_t listener;
    struct sockaddr_in sin;
    struct event_base *base;
    struct event *listener_event;

    base = event_base_new();

    sin.sin_family = AF_INET;
    sin.sin_addr.s_addr = 0;
    sin.sin_port = htons(40713);

    listener = socket(AF_INET, SOCK_STREAM, 0);
    evutil_make_socket_nonblocking(listener);

    bind(listener, (struct sockaddr *)&sin, sizeof(sin));
    listen(listener, 16);

    Game game;
    game.base = base;
    game.acceptConnections = true;
    game.gameId = 0;
    game.lastAssignedTicketId = 0;

    listener_event = event_new(base, listener, EV_READ | EV_PERSIST, on_accept, &game);
    event_add(listener_event, NULL);

    //new game every 3 seconds, for testing purpose, change the value for real use-case
    struct timeval one_sec = {3, 0};
    struct event *ev;
    ev = event_new(base, -1, EV_PERSIST, cb_func, &game);
    event_add(ev, &one_sec);

    event_base_dispatch(base);
    return 0;
}
