/*
 * Test D-Bus Socket Abstraction
 */

#include <c-macro.h>
#include <stdlib.h>
#include <sys/epoll.h>
#include <sys/types.h>
#include <sys/socket.h>
#include "dbus/message.h"
#include "dbus/socket.h"

static void test_setup(void) {
        _c_cleanup_(socket_deinit) Socket server = SOCKET_NULL(server), client = SOCKET_NULL(client);
        int r;

        r = socket_init(&server, -1, true);
        assert(r == 0);

        r = socket_init(&client, -1, false);
        assert(r == 0);
}

static void test_line(void) {
        _c_cleanup_(socket_deinit) Socket client = SOCKET_NULL(client), server = SOCKET_NULL(server);
        const char *test = "TEST", *line;
        size_t n_bytes;
        int pair[2], r;

        r = socketpair(AF_UNIX, SOCK_STREAM, 0, pair);
        assert(r >= 0);

        r = socket_init(&client, pair[0], false);
        assert(r == 0);

        r = socket_init(&server, pair[1], true);
        assert(r == 0);

        r = socket_dequeue_line(&server, &line, &n_bytes);
        assert(!r && !line);

        r = socket_queue_line(&client, test, strlen(test));
        assert(r == 0);

        r = socket_queue_line(&client, test, strlen(test));
        assert(r == 0);

        r = socket_dispatch(&client, EPOLLOUT);
        assert(r == SOCKET_E_LOST_INTEREST);
        r = socket_dispatch(&server, EPOLLIN);
        assert(!r);

        r = socket_dequeue_line(&server, &line, &n_bytes);
        assert(!r && line);
        assert(n_bytes == strlen(test));
        assert(memcmp(test, line, n_bytes) == 0);

        r = socket_dequeue_line(&server, &line, &n_bytes);
        assert(!r && line);
        assert(n_bytes == strlen(test));
        assert(memcmp(test, line, n_bytes) == 0);

        r = socket_dequeue_line(&server, &line, &n_bytes);
        assert(!r && !line);
}

static void test_message(void) {
        _c_cleanup_(socket_deinit) Socket client = SOCKET_NULL(client), server = SOCKET_NULL(server);
        _c_cleanup_(message_unrefp) Message *message1 = NULL, *message2 = NULL;
        SocketBuffer *skb;
        MessageHeader header = {
                .endian = 'l',
        };
        int pair[2], r;

        r = socketpair(AF_UNIX, SOCK_STREAM, 0, pair);
        assert(r >= 0);

        r = socket_init(&client, pair[0], false);
        assert(r == 0);

        r = socket_init(&server, pair[1], true);
        assert(r == 0);

        r = socket_dequeue(&server, &message2);
        assert(!r && !message2);

        r = message_new_incoming(&message1, header);
        assert(r == 0);

        r = socket_buffer_new_message(&skb, message1);
        assert(!r);

        socket_queue(&client, skb);

        r = socket_dispatch(&client, EPOLLOUT);
        assert(r == SOCKET_E_LOST_INTEREST);
        r = socket_dispatch(&server, EPOLLIN);
        assert(!r);

        r = socket_dequeue(&server, &message2);
        assert(!r && message2);

        assert(memcmp(message1->header, message2->header, sizeof(header)) == 0);
}

int main(int argc, char **argv) {
        test_setup();
        test_line();
        test_message();
        return 0;
}
