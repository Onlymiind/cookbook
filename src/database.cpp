#include "database.h"

#include <cstddef>
#include <memory>
#include <optional>
#include <ostream>
#include <string_view>
#include <vector>

namespace db {

    // TODO: implement fuzzy search (it seems that a combination of spellfix1
    // and fts5 extensions should do the job)

    namespace ingredients {
        constexpr std::string_view g_create_table =
            "CREATE TABLE IF NOT EXISTS ingredients (id INTEGER PRIMARY KEY, "
            "name TEXT UNIQUE NOT NULL, units INTEGER NOT NULL CHECK (units >= "
            "0 AND units < 3));";
        constexpr std::string_view
            g_add = "INSERT INTO ingredients (name, units) VALUES(?, ?);";
        constexpr std::string_view
            g_get_all = "SELECT name, units FROM ingredients;";

        constexpr int g_name_column = 0;
        constexpr int g_units_column = 1;
    } // namespace ingredients

    namespace recipies {
        constexpr std::string_view g_create_table =
            "CREATE TABLE IF NOT EXISTS recipies (id INTEGER "
            "PRIMARY KEY, name TEXT UNIQUE NOT NULL, directions TEXT NOT NULL, "
            "type INTEGER NOT NULL);";
        constexpr std::string_view g_add = "INSERT INTO recipies (name, "
                                           "directions, type) VALUES(?, ?, ?)";
        constexpr std::string_view g_search_by_name =
            "SELECT name FROM recipies WHERE name LIKE ? LIMIT ?;";

    } // namespace recipies

    void destroy_db(sqlite3 *db) { sqlite3_close(db); }
    void destroy_smt(sqlite3_stmt *smt) { sqlite3_finalize(smt); }

    std::optional<Statement> prepare_statement(sqlite3 *db,
                                               std::string_view smt) {
        sqlite3_stmt *prepared = nullptr;
        if (sqlite3_prepare_v2(db, smt.data(), static_cast<size_t>(smt.size()),
                               &prepared, nullptr) != SQLITE_OK) {
            return {};
        }
        return Statement{prepared};
    }

    std::optional<Database> init_database(std::string_view db_file,
                                          std::ostream *err_out) {
        sqlite3 *db_ptr = nullptr;
        if (sqlite3_open(db_file.data(), &db_ptr) != SQLITE_OK) {
            if (err_out) {
                *err_out << sqlite3_errmsg(db_ptr) << '\n';
            }
            return {};
        }
        std::unique_ptr<sqlite3, void (*)(sqlite3 *)> db{db_ptr, destroy_db};
        if (sqlite3_exec(db.get(), recipies::g_create_table.data(), nullptr,
                         nullptr, nullptr) != SQLITE_OK) {
            if (err_out) {
                *err_out << sqlite3_errmsg(db.get()) << '\n';
            }
            return {};
        }
        if (sqlite3_exec(db.get(), ingredients::g_create_table.data(), nullptr,
                         nullptr, nullptr) != SQLITE_OK) {
            if (err_out) {
                *err_out << sqlite3_errmsg(db.get()) << '\n';
            }
            return {};
        }

        auto add_ingredient = prepare_statement(db.get(), ingredients::g_add);
        if (!add_ingredient) {
            if (err_out) {
                *err_out << sqlite3_errmsg(db.get()) << '\n';
            }
            return {};
        }
        auto get_ingredients = prepare_statement(db.get(),
                                                 ingredients::g_get_all);
        if (!get_ingredients) {
            if (err_out) {
                *err_out << sqlite3_errmsg(db.get()) << '\n';
            }
            return {};
        }
        auto add_recipie = prepare_statement(db.get(), recipies::g_add);
        if (!add_recipie) {
            if (err_out) {
                *err_out << sqlite3_errmsg(db.get()) << '\n';
            }
            return {};
        }
        auto search_recipies_by_name = prepare_statement(db.get(),
                                                         recipies::
                                                             g_search_by_name);
        if (!search_recipies_by_name) {
            if (err_out) {
                *err_out << sqlite3_errmsg(db.get()) << '\n';
            }
            return {};
        }
        return Database{db.release(),
                        Queries{.add_ingredient{std::move(*add_ingredient)},
                                .get_ingredients{std::move(*get_ingredients)},
                                .add_recipie{std::move(*add_recipie)},
                                .search_recipies_by_name{
                                    std::move(*search_recipies_by_name)}}};
    }

    bool Database::add_ingredient(const Ingredient &ingredient) {
        if (!queries_.add_ingredient.reset()) {
            return false;
        }
        if (!queries_.add_ingredient.bind(ingredient.name, 1)) {
            return false;
        }
        if (!queries_.add_ingredient.bind(static_cast<int>(ingredient.units),
                                          2)) {
            return false;
        }
        if (!queries_.add_ingredient.run()) {
            return false;
        }
        return true;
    }

    std::vector<Ingredient> Database::get_ingredients() {
        std::vector<Ingredient> result;

        Ingredient buffer;
        queries_.get_ingredients.reset();
        queries_.get_ingredients.step();
        Status status = queries_.get_ingredients.status();
        while (status != Status::DONE && status != Status::ERROR) {
            if (status == Status::ROW_AVAILABLE) {
                buffer.name = *queries_.get_ingredients.get_text(
                    ingredients::g_name_column);
                buffer.units = static_cast<Unit>(
                    *queries_.get_ingredients.get_int(
                        ingredients::g_units_column));
                result.push_back(std::move(buffer));
            }
            queries_.get_ingredients.step();
            status = queries_.get_ingredients.status();
        }
        return result;
    }

    bool Database::add_recipie(std::string_view name,
                               std::string_view directions, RecipieType type) {
        if (!queries_.add_recipie.reset()) {
            return false;
        }
        if (!queries_.add_recipie.bind(name, 1)) {
            return false;
        }
        if (!queries_.add_recipie.bind(directions, 2)) {
            return false;
        }
        if (!queries_.add_recipie.bind(static_cast<int>(type), 3)) {
            return false;
        }
        if (!queries_.add_recipie.run()) {
            return false;
        }
        return true;
    }

    std::vector<std::string> Database::find_recipies(std::string_view name,
                                                     size_t max_count) {
        if (!queries_.search_recipies_by_name.reset()) {
            return {};
        }
        std::string name_token;
        name_token.reserve(name.size() + 2);
        name_token.push_back('%');
        name_token += name;
        name_token.push_back('%');

        if (!queries_.search_recipies_by_name.bind(name_token, 1)) {
            return {};
        }
        if (!queries_.search_recipies_by_name.bind(static_cast<int>(max_count),
                                                   2)) {
            return {};
        }
        std::vector<std::string> result;
        queries_.search_recipies_by_name.step();
        Status status = queries_.search_recipies_by_name.status();
        while (status != Status::DONE && status != Status::ERROR) {
            if (status == Status::ROW_AVAILABLE) {
                result.push_back(*queries_.search_recipies_by_name.get_text(0));
            }
            queries_.search_recipies_by_name.step();
            status = queries_.search_recipies_by_name.status();
        }

        return result;
    }

    Database::~Database() { sqlite3_close(db_); }

    bool Statement::reset() { return sqlite3_reset(smt_) == SQLITE_OK; }

    bool Statement::run() {
        status_ = Status::NONE;
        int status = sqlite3_step(smt_);
        while (status != SQLITE_DONE && status != SQLITE_ERROR) {
            status = sqlite3_step(smt_);
        }
        if (status == SQLITE_ERROR) {
            status_ = Status::ERROR;
        } else if (status == SQLITE_DONE) {
            status_ = Status::DONE;
        }
        return status == SQLITE_DONE;
    }

    bool Statement::step() {
        status_ = Status::NONE;
        int status = sqlite3_step(smt_);
        if (status == SQLITE_ERROR) {
            status_ = Status::ERROR;
            return false;
        }
        if (status == SQLITE_ROW) {
            status_ = Status::ROW_AVAILABLE;
        } else if (status == SQLITE_DONE) {
            status_ = Status::DONE;
        }
        return true;
    }

    std::optional<std::string> Statement::get_text(int column) {
        if (status_ != Status::ROW_AVAILABLE) {
            return std::optional<std::string>{};
        } else if (sqlite3_column_type(smt_, column) != SQLITE_TEXT) {
            return std::optional<std::string>{};
        }

        const unsigned char *text = sqlite3_column_text(smt_, column);
        int size = sqlite3_column_bytes(smt_, column);
        return std::string{reinterpret_cast<const char *>(text),
                           static_cast<size_t>(size)};
    }

    std::optional<int> Statement::get_int(int column) {
        if (status_ != Status::ROW_AVAILABLE) {
            return std::optional<int>{};
        } else if (sqlite3_column_type(smt_, column) != SQLITE_INTEGER) {
            return std::optional<int>{};
        }

        return sqlite3_column_int(smt_, column);
    }

    bool Statement::bind(int param, int pos) {
        return sqlite3_bind_int(smt_, pos, param) == SQLITE_OK;
    }

    bool Statement::bind(std::string_view param, int pos) {
        return sqlite3_bind_text(smt_, pos, param.data(),
                                 static_cast<int>(param.size()),
                                 SQLITE_TRANSIENT) == SQLITE_OK;
    }

} // namespace db
