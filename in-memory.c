/* Compile via 'gcc in-memory.c -pthread -l sqlite3' */

#include <stdio.h>
#include <pthread.h>
#include <sqlite3.h>

#define RW_DSN "file:/mydb?mode=rw&vfs=memdb&_txlock=immediate&_fk=false"
#define RO_DSW "file:/mydb?mode=ro&vfs=memdb&_txlock=deferred&_fk=false"

void* insertFn(void *arg) {
    char *err_msg = 0;
    int rc = 0;
    sqlite3 *conn = (sqlite3*)arg;
    for (int i = 0; i<10000000; ++i) {
        rc = sqlite3_exec(conn, "INSERT INTO logs(entry) VALUES('hello')", 0, 0, &err_msg);
        if (rc != SQLITE_OK) {
            fprintf(stderr, "SQL error: %s\n", err_msg);
        }
    }
}

int main(void) {
    sqlite3 *rwConn;
    sqlite3 *roConn;
    char *err_msg = 0;
    int rc = 0;
    sqlite3_stmt *res;
    pthread_t thread_id;

    rc = sqlite3_open_v2(RW_DSN, &rwConn, SQLITE_OPEN_CREATE | SQLITE_OPEN_READWRITE | SQLITE_OPEN_URI, NULL);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Can't open execute connection: %s\n", sqlite3_errmsg(rwConn));
    }
    rc = sqlite3_exec(rwConn, "CREATE TABLE logs (entry TEXT)", 0, 0, &err_msg);
     if (rc != SQLITE_OK) {  
        fprintf(stderr, "SQL error: %s\n", err_msg);
        sqlite3_free(err_msg);        
        return 1;
    }
    rc = sqlite3_exec(rwConn, "PRAGMA busy_timeout = 5000", 0, 0, &err_msg);
     if (rc != SQLITE_OK) {  
        fprintf(stderr, "SQL error setting busy timeout on RW connection: %s\n", err_msg);
        sqlite3_free(err_msg);        
        return 1;
    }
    pthread_create(&thread_id, NULL, insertFn, (void*)rwConn);

    rc = sqlite3_open_v2(RO_DSW, &roConn, SQLITE_OPEN_READONLY | SQLITE_OPEN_URI, NULL);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Can't open execute connection: %s\n", sqlite3_errmsg(roConn));
    }
    rc = sqlite3_prepare_v2(roConn, "PRAGMA busy_timeout = 5000", -1, &res, 0);    
    if (rc != SQLITE_OK) {
        fprintf(stderr, "SQL error setting busy timeout on RO connection: %s\n", sqlite3_errmsg(roConn));
        return 1;
    }

    int nSuccess = 0;
    for (int i = 0; i<10000000; ++i){
        rc = sqlite3_prepare_v2(roConn, "SELECT * FROM logs", -1, &res, 0);
        if (rc != SQLITE_OK) {
            fprintf(stderr, "Failed to fetch data: %s\n", sqlite3_errmsg(roConn));
            return 1;
        }
        sqlite3_finalize(res);

        if (i % 1000000 == 0) {
            rc = sqlite3_prepare_v2(roConn, "SELECT COUNT(*) FROM logs", -1, &res, 0);
            if (rc != SQLITE_OK) {
                fprintf(stderr, "Failed to fetch data: %s\n", sqlite3_errmsg(roConn));
                return 1;
            }
            rc = sqlite3_step(res);
            if (rc == SQLITE_ROW) {
                printf("Count during loop is: %s\n", sqlite3_column_text(res, 0));
            }
            sqlite3_finalize(res);
        }
        nSuccess++;
    }
    fprintf(stderr, "Fetched %d times without error\n", nSuccess);

    pthread_join(thread_id, NULL);
    rc = sqlite3_prepare_v2(roConn, "SELECT COUNT(*) FROM logs", -1, &res, 0);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Failed to fetch data: %s\n", sqlite3_errmsg(roConn));
        return 1;
    }
    rc = sqlite3_step(res);
    if (rc == SQLITE_ROW) {
        printf("Count is: %s\n", sqlite3_column_text(res, 0));
    }
    sqlite3_finalize(res);

    return 0;
}
