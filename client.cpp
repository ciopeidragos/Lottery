
#include <event2/event.h>
#include <event2/buffer.h>
#include <event2/bufferevent.h>
#include <string>

#define MAX_LINE 16384

void on_read(struct bufferevent *bev, void *arg)
{
  char *line;
  struct evbuffer *input;
  input = bufferevent_get_input(bev);
  size_t n;

  line = evbuffer_readln(input, &n, EVBUFFER_EOL_LF);
  printf("%s \n", line);
}

void on_write(struct bufferevent *bev, void *arg)
{
  struct evbuffer *output;
  char *line;
  output = bufferevent_get_output(bev);
  size_t n;
  char str[MAX_LINE];

  scanf("%s", str);
  evbuffer_add(output, std::string(str).c_str(), std::string(str).size());
}

int main(int argc, char **argv)
{
  evutil_socket_t client_socket;
  struct sockaddr_in sin;
  struct event_base *base;
  struct bufferevent *bev;
  struct evbuffer *input, *output;

  base = event_base_new();

  sin.sin_family = AF_INET;
  sin.sin_addr.s_addr = 0;
  sin.sin_port = htons(40713);

  client_socket = socket(AF_INET, SOCK_STREAM, 0);
  bev = bufferevent_socket_new(base, client_socket, BEV_OPT_CLOSE_ON_FREE);
  input = bufferevent_get_input(bev);
  output = bufferevent_get_output(bev);

  bufferevent_setcb(bev, on_read, on_write, NULL, NULL);
  bufferevent_setwatermark(bev, EV_READ, 0, MAX_LINE);
  bufferevent_setwatermark(bev, EV_WRITE, 0, 0);
  bufferevent_enable(bev, EV_READ | EV_WRITE);

  connect(client_socket, (struct sockaddr *)&sin, sizeof(sin));

  event_base_dispatch(base);
  return 0;
}
