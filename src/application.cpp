#include "application.h"
#include "database.h"

#include <algorithm>
#include <cctype>
#include <iostream>
#include <stdexcept>
#include <utility>

namespace app {

    namespace commands {
        constexpr std::string_view g_add_ingredient = "add_ingr";
        constexpr std::string_view g_add_recipie = "add_recipie";
        constexpr std::string_view g_list_ingredients = "list_ingrs";
        constexpr std::string_view g_search_recipies = "search";
        constexpr std::string_view g_quit = "quit";
    } // namespace commands

    Application init(std::string_view db_name) {
        auto db = db::init_database(db_name, &std::cout);
        if (!db) {
            throw std::runtime_error("failed to init database");
        }
        return Application{std::move(*db)};
    }

    std::string get_word(std::string &str) {
        size_t end = std::find_if(str.begin(), str.end(),
                                  [](char c) { return std::isspace(c); }) -
                     str.begin();
        std::string result;
        if (end == std::string_view::npos) {
            result = std::exchange(str, "");
            return result;
        }
        result = str.substr(0, end);
        str = str.substr(std::find_if(str.begin() + end, str.end(),
                                      [](char c) { return !std::isspace(c); }) -
                         str.begin());
        return result;
    }

    void Application::run() {
        while (true) {
            std::string cmd;
            std::getline(std::cin, cmd);
            if (cmd.empty()) {
                continue;
            }
            cmd = cmd.substr(
                std::find_if(cmd.begin(), cmd.end(),
                             [](char c) { return !std::isspace(c); }) -
                cmd.begin());

            size_t last_space = cmd.size() - 1;
            for (; last_space > 0 && std::isspace(cmd[last_space]);
                 --last_space)
                ;
            // handle fully whitespace string
            if (last_space == 0 && std::isspace(cmd[last_space])) {
                cmd = "";
            } else {
                cmd.resize(last_space + 1);
            }
            if (cmd.empty()) {
                continue;
            }

            std::string cmd_name = get_word(cmd);
            if (cmd_name == commands::g_list_ingredients) {
                auto ingrs = db_.get_ingredients();
                std::cout << "Ingredients:\n";
                for (const auto &ingr : ingrs) {
                    std::cout << ingr.name << ' '
                              << static_cast<int>(ingr.units) << '\n';
                }

            } else if (cmd_name == commands::g_quit) {
                return;
            } else if (cmd_name == commands::g_search_recipies) {
                std::string search_name = get_word(cmd);
                auto result = db_.find_recipies(search_name);
                std::cout << "Query results:\n";
                for (const auto &recipie : result) {
                    std::cout << recipie << '\n';
                }
            } else if (cmd_name == commands::g_add_recipie) {
                std::cout << ">TODO\n";
            } else if (cmd_name == commands::g_add_ingredient) {
                std::string name = get_word(cmd);
                if (name.empty()) {
                    std::cout << ">error: empty ingredient name\n";
                }
                if (cmd.empty()) {
                    std::cout << ">error: no measure unit is given\n";
                }
                db::Unit unit{};
                switch (cmd[0]) {
                case 'c': unit = db::Unit::COUNT; break;
                case 'g': unit = db::Unit::GRAM; break;
                case 'm': unit = db::Unit::MILLILITRE; break;
                default:
                    std::cout << ">error: unknown measure unit\n";
                    continue;
                }
                db::Ingredient ingr{std::move(name), unit};
                if (!db_.add_ingredient(ingr)) {
                    std::cout << ">error: could not add ingredient\n";
                }
            } else {
                std::cout << ">error: unknown command\n";
            }
        }
    }

} // namespace app
