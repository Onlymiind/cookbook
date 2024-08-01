#ifndef COOKBOOK_DATABASE_HDR_
#define COOKBOOK_DATABASE_HDR_

#include <sqlite/sqlite3.h>

#include <optional>
#include <ostream>
#include <string_view>
#include <utility>
#include <vector>

namespace db {

    enum class Unit { COUNT, GRAM, MILLILITRE };

    enum class RecipieType { NONE };

    struct Ingredient {
        std::string name;
        Unit units{};
    };

    enum class Status { NONE, DONE, ERROR, ROW_AVAILABLE };

    class Statement {
      public:
        Statement(sqlite3_stmt *smt) : smt_(smt) {}
        ~Statement() { sqlite3_finalize(smt_); }
        Statement(const Statement &) = delete;
        Statement &operator=(const Statement &) = delete;

        Statement(Statement &&other) noexcept
            : smt_(std::exchange(other.smt_, nullptr)),
              status_(std::exchange(other.status_, Status::NONE)) {}
        Statement &operator=(Statement &&other) noexcept {
            if (this == &other) {
                return *this;
            }
            sqlite3_finalize(smt_);
            smt_ = std::exchange(other.smt_, nullptr);
            status_ = std::exchange(other.status_, Status::NONE);
            return *this;
        }

        bool reset();
        bool bind(int param, int pos);
        bool bind(std::string_view param, int pos);

        bool run();
        bool step();

        std::optional<std::string> get_text(int column);
        std::optional<int> get_int(int column);

        Status status() const noexcept { return status_; }

      private:
        sqlite3_stmt *smt_ = nullptr;
        Status status_ = Status::NONE;
    };

    struct Queries {
        Statement add_ingredient;
        Statement get_ingredients;
        Statement add_recipie;
        Statement search_recipies_by_name;
    };

    class Database {
      public:
        Database(sqlite3 *db, Queries &&queries)
            : db_(db), queries_(std::move(queries)) {}
        ~Database();
        Database(const Database &) = delete;
        Database &operator=(const Database &) = delete;

        Database(Database &&other) noexcept
            : db_(std::exchange(other.db_, nullptr)),
              queries_(std::move(other.queries_)) {}
        Database &operator=(Database &&other) noexcept {
            if (this == &other) {
                return *this;
            }
            sqlite3_close(db_);
            db_ = std::exchange(other.db_, nullptr);
            queries_ = std::move(other.queries_);
            return *this;
        }

        sqlite3 *operator*() const noexcept { return db_; }

        bool add_ingredient(const Ingredient &ingredient);
        std::vector<Ingredient> get_ingredients();

        bool add_recipie(std::string_view name, std::string_view directions,
                         RecipieType type);

        std::vector<std::string> find_recipies(std::string_view name,
                                               size_t max_count = 5);

      private:
        sqlite3 *db_ = nullptr;
        Queries queries_;
    };

    std::optional<Database> init_database(std::string_view db_file,
                                          std::ostream *err_out);
} // namespace db

#endif
