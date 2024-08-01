#include <iostream>
#include <sqlite/sqlite3.h>

int main() {
  sqlite3 *db = nullptr;
  if (sqlite3_open("db.db", &db) != SQLITE_OK) {
    std::cout << sqlite3_errmsg(db) << std::endl;
    return 1;
  }
  sqlite3_stmt *create_table = nullptr;
  if (sqlite3_prepare_v2(db,
                         "CREATE TABLE IF NOT EXISTS recipies (id INT"
                         "PRIMARY UNIQUE NOT NULL, name TEXT UNIQUE NOT NULL);",
                         -1, &create_table, nullptr) != SQLITE_OK) {
    std::cout << sqlite3_errmsg(db) << std::endl;
    return 1;
  }
  if (sqlite3_step(create_table) == SQLITE_ERROR) {
    std::cout << sqlite3_errmsg(db) << std::endl;
    return 0;
  }

  std::cout << "OK\n";
  sqlite3_close(db);
  return 0;
}
