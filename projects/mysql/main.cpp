#include <windows.h>
#include <stdio.h>

#include "../../support/mysql5.6/include/mysql.h"
#pragma comment(lib, "../../support/mysql5.6/lib/vs11/mysqlclient.lib")

int main(int, char **)
{
    const char user[] = "root";
    const char psw[] = "";
    const char host[] = "localhost";
    //const char table[] = "testdb";
    unsigned short port = 3306;

    MYSQL *mysql = mysql_init(nullptr);
    if (mysql_real_connect(mysql, host, user, psw, nullptr, port, nullptr, 0) != nullptr)
    {
        if (1 || mysql_query(mysql, "create database newdatabase") == 0)
        {
            if (mysql_select_db(mysql, "newdatabase") == 0)
            {
                if (1 || mysql_query(mysql, "create table player(uuid int unsigned not null, username char(16) not null, password char(16) not null)") == 0)
                {
                    if (0)
                    {
                        char str[128];
                        for (int i = 0; i < 10; ++i)
                        {
                            _snprintf(str, 127, "insert player values(%d, %d, %d)", i, i, i);
                            if (mysql_query(mysql, str) == 0)
                            {

                            }
                            else
                            {
                                printf("%s\n", mysql_error(mysql));
                            }
                        }
                    }
                    else
                    {
                        char str[128];
                        for (int i = 0; i < 10; ++i)
                        {
                            _snprintf(str, 127, "update player set uuid = %d, username = %d, password = %d", i, i, i);
                            if (mysql_query(mysql, str) == 0)
                            {

                            }
                            else
                            {
                                printf("%s\n", mysql_error(mysql));
                            }
                        }
                    }

                    if (mysql_query(mysql, "select * from player") == 0)
                    {
                        MYSQL_RES *result = mysql_store_result(mysql);
                        if (result != nullptr)
                        {
                            my_ulonglong rowCnt = mysql_num_rows(result);

                            MYSQL_FIELD *fd = nullptr;
                            while ((fd = mysql_fetch_field(result)) != nullptr)
                            {
                                printf("%s\t", fd->name);
                            }
                            printf("\n");

                            unsigned fdCnt = mysql_num_fields(result);

                            MYSQL_ROW row = nullptr;
                            while ((row = mysql_fetch_row(result)) != nullptr)
                            {
                                for (unsigned i = 0; i < fdCnt; ++i)
                                {
                                    printf("%s\t", row[i]);
                                }
                                printf("\n");
                            }

                            mysql_free_result(result);
                        }
                    }
                    else
                    {
                        printf("%s\n", mysql_error(mysql));
                    }
                }
                else
                {
                    printf("%s\n", mysql_error(mysql));
                }
            }
            else
            {
                printf("%s\n", mysql_error(mysql));
            }
        }
        else
        {
            printf("%s\n", mysql_error(mysql));
        }
    }
    else
    {
        printf("%s\n", mysql_error(mysql));
    }
    mysql_close(mysql);
    return 0;
}
