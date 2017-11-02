#ifdef FELIXDU_TEST
#include <iostream>
#include <stdexcept>
#include <cstdlib>
#include <sys/msg.h>

int main(int argc, char* argv[])
{
    try {
        if (argc < 2) {
            std::cout << "Usage: " << argv[0] << " [0-9]" << std::endl;
            return -1;
        }

        int id = msgget(3003, 0666|IPC_CREAT);

        struct _msg{
            long int type;
            long int data_type;
            int data;
        }msg;

        msg.type = 10;
        msg.data_type = 88;
        msg.data = atoi(argv[1]);
        msgsnd(id, (void*)&msg, sizeof(msg), 0);

    } catch (std::exception & e) {
        std::cout << e.what() << std::endl;
        return -1;
    }

    return 0;
}
#endif
