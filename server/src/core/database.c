#include "server.h"
#include <stdlib.h>

extern sqlite3 *db;

void init_db() {
    int rc = sqlite3_open(DB_FILE, &db);

    if (rc != SQLITE_OK) {
        fprintf(stderr, "Cannot open database: %s\n", sqlite3_errmsg(db));
        sqlite3_close(db);
        exit(1);
    }

    // Read and execute schema.sql
    FILE *schema_file = fopen("schema.sql", "r");
    if (schema_file) {
        fseek(schema_file, 0, SEEK_END);
        long file_size = ftell(schema_file);
        fseek(schema_file, 0, SEEK_SET);
        
        char *schema_sql = malloc(file_size + 1);
        if (schema_sql) {
            fread(schema_sql, 1, file_size, schema_file);
            schema_sql[file_size] = '\0';
            
            char *err_msg = NULL;
            rc = sqlite3_exec(db, schema_sql, NULL, NULL, &err_msg);
            if (rc != SQLITE_OK) {
                fprintf(stderr, "Schema execution error: %s\n", err_msg);
                sqlite3_free(err_msg);  
            } else {
                printf("Database schema initialized.\n");
            }
            free(schema_sql);
        }
        fclose(schema_file);
    } else {
        printf("Warning: schema.sql not found, using existing database.\n");
    }
    
    printf("Database opened successfully.\n");
}
