//g++ -o $@ $^ -I ../inc -I /home/durd/dm8127/ipnc_rdk/ipnc_app/vd/lib/sqlite/include -lsqlite3
//#define FELIXDU_TEST
#ifdef FELIXDU_TEST
#include <iostream>

#define DEBUG printf
#define INFO  printf
#define ERROR printf

#include "park_status_db_api.cpp"

int main(int argc, char* argv[])
{
    park_state_db_open("./test.db");
    park_state_db_create_tb();

    park_history_t t = {0};
    strcpy(t.time, "2017-06-01 11:11:11");
    strcpy(t.plate, "鲁BY29F5");
    strcpy(t.pic_name[0], "1.jpg");
    strcpy(t.pic_name[1], "2.jpg");
    t.state = PARK_RECORD_STATE::DRIVE_IN;

    park_history_t t1 = {0};
    strcpy(t1.time, "2017-06-02 11:11:11");
    strcpy(t1.plate, "鲁BY29F5");
    strcpy(t1.pic_name[0], "3.jpg");
    strcpy(t1.pic_name[1], "4.jpg");
    t1.state = PARK_RECORD_STATE::DRIVE_OUT;

    park_history_t t2 = {0};
    strcpy(t2.time, "2017-06-03 11:11:11");
    strcpy(t2.plate, "鲁BY29F5");
    strcpy(t2.pic_name[0], "5.jpg");
    strcpy(t2.pic_name[1], "6.jpg");
    t2.state = PARK_RECORD_STATE::DRIVE_IN;

    park_history_t t3 = {0};
    strcpy(t3.time, "2017-06-04 11:11:11");
    strcpy(t3.plate, "鲁BY29F5");
    strcpy(t3.pic_name[0], "7.jpg");
    strcpy(t3.pic_name[1], "8.jpg");
    t3.state = PARK_RECORD_STATE::DRIVE_OUT;

    park_state_db_insert_parkhistory_tb(&t);
    park_state_db_insert_parkhistory_tb(&t1);
    park_state_db_insert_parkhistory_tb(&t2);
    park_state_db_insert_parkhistory_tb(&t3);

    //std::cout << "select * from parkhistory" << std::endl;
    park_state_db_select_parkhistory_tb(NULL, NULL, &t);

    //std::cout << "select * from parkhistory where datetime(time) > '2017-06-02 10:11:11'" << std::endl;
    park_state_db_select_parkhistory_tb("2017-06-02 10:11:11", NULL, &t);

    //std::cout << "select * from parkhistory where datetime(time) < '2017-06-02 10:11:11'" << std::endl;
    park_state_db_select_parkhistory_tb(NULL, "2017-06-02 10:11:11", &t);

    //std::cout << "select * from parkhistory where datetime(time) > '2017-06-02 10:11:11' and datetime(time) < '2017-06-03 23:59:59'"<< std::endl;
    park_state_db_select_parkhistory_tb("2017-06-02 10:11:11", "2017-06-03 23:59:59", &t);

    park_state_db_delete_parkhistory_old("2017-06-03");
    park_state_db_select_parkhistory_tb(NULL, NULL, &t);

    while(1);
    return 0;
}
#endif
