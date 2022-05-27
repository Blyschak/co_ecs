#include <cobalt/core/application.hpp>

class my_app : public cobalt::application {
public:
    my_app(int argc, char** argv) : cobalt::application(argc, argv) {
    }
};

int main(int argc, char** argv) {
    my_app app{ argc, argv };
    app.run();
    return 0;
}