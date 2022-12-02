/* Compile via 'gcc in-memory.c -pthread -l sqlite3' */
#include <stdio.h>
#include <pthread.h>
#include <sqlite3.h>

void* insertFn(void *arg) {
    char *err_msg = 0;
    int rc = 0;
    sqlite3 *conn = (sqlite3*)arg;
    while(1) {
        rc = sqlite3_exec(conn, "INSERT INTO logs(entry) VALUES('hello')", 0, 0, &err_msg);
        if (rc != SQLITE_OK) {
            fprintf(stderr, "SQL error: %s\n", err_msg);
        }
    }
}

/* Compile via 'gcc open-in-mem.c -l sqlite3' */
int main(void) {
    sqlite3 *rwConn;
    sqlite3 *roConn;
    char *err_msg = 0;
    int rc = 0;
    sqlite3_stmt *res;
    pthread_t thread_id;

    rc = sqlite3_open_v2("file:/mydb?mode=rw&vfs=memdb&_txlock=immediate&_fk=false", &rwConn, SQLITE_OPEN_CREATE | SQLITE_OPEN_READWRITE | SQLITE_OPEN_URI, NULL);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Can't open execute connection: %s\n", sqlite3_errmsg(rwConn));
    }
    rc = sqlite3_exec(rwConn, "CREATE TABLE logs (entry TEXT)", 0, 0, &err_msg);
     if (rc != SQLITE_OK) {  
        fprintf(stderr, "SQL error: %s\n", err_msg);
        sqlite3_free(err_msg);        
        return 1;
    }

    pthread_create(&thread_id, NULL, insertFn, (void*)rwConn);

    rc = sqlite3_open_v2("file:/mydb?mode=ro&vfs=memdb&_txlock=deferred&_fk=false", &roConn, SQLITE_OPEN_READONLY | SQLITE_OPEN_URI, NULL);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Can't open execute connection: %s\n", sqlite3_errmsg(roConn));
    }   
    rc = sqlite3_prepare_v2(roConn, "SELECT * FROM logs", -1, &res, 0);    
    if (rc != SQLITE_OK) {
        
        fprintf(stderr, "Failed to fetch data: %s\n", sqlite3_errmsg(roConn));
        return 1;
    }
    rc = sqlite3_step(res);
    if (rc == SQLITE_ROW) {
        printf("%s\n", sqlite3_column_text(res, 0));
    }
    sqlite3_finalize(res);

    return 0;
}
