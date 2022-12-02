#include <stdio.h>
#include <sqlite3.h>

/* Compile via 'gcc open-in-mem.c -l sqlite3' */
int main(void) {
    sqlite3 *conn;
    char *err_msg = 0;
    int rc;
    sqlite3_stmt *res;

    //rc = sqlite3_open("file::memory:", &conn);
    rc = sqlite3_open_v2("file::memory:", &conn, SQLITE_OPEN_CREATE | SQLITE_OPEN_READWRITE | SQLITE_OPEN_URI, NULL);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Can't open execute connection: %s\n", sqlite3_errmsg(conn));
    }

    rc = sqlite3_exec(conn, "CREATE TABLE logs (entry TEXT)", 0, 0, &err_msg);
     if (rc != SQLITE_OK) {  
        fprintf(stderr, "SQL error: %s\n", err_msg);
        sqlite3_free(err_msg);        
        return 1;
    } 
    rc = sqlite3_exec(conn, "INSERT INTO logs(entry) VALUES('hello')", 0, 0, &err_msg);
     if (rc != SQLITE_OK) {  
        fprintf(stderr, "SQL error: %s\n", err_msg);
        sqlite3_free(err_msg);        
        return 1;
    }

    rc = sqlite3_prepare_v2(conn, "SELECT * FROM logs", -1, &res, 0);    
    if (rc != SQLITE_OK) {
        
        fprintf(stderr, "Failed to fetch data: %s\n", sqlite3_errmsg(conn));        
        return 1;
    }    
    
    rc = sqlite3_step(res);
    if (rc == SQLITE_ROW) {
        printf("%s\n", sqlite3_column_text(res, 0));
    }
    sqlite3_finalize(res);
    sqlite3_close(conn);

    return 0;
}
