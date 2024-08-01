#ifndef COOKBOOK_APPLICATION_HDR_
#define COOKBOOK_APPLICATION_HDR_

#include "database.h"
namespace app {
    class Application {
      public:
        Application(db::Database db) : db_(std::move(db)) {}

        void run();

      private:
        db::Database db_;
    };

    Application init(std::string_view db_name);
} // namespace app

#endif
