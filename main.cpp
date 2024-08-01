#include "application.h"

int main() {
    app::Application app = app::init("db.db");
    app.run();
}
