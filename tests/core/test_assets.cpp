#include <cobalt/core/assets.hpp>

#include <gtest/gtest.h>

struct some_asset {
    std::string name{};
};

struct some_asset_loader {
    cobalt::owned<some_asset> load_from_file(const std::string& filename) {
        return std::make_unique<some_asset>(filename);
    }
};

namespace cobalt {

template<>
struct asset_traits<some_asset> {
    using loader_type = some_asset_loader;
};

} // namespace cobalt

TEST(assets, basic) {
    using some_asset_storage = cobalt::asset_storage<some_asset>;

    some_asset_storage storage;

    auto handle = storage.from_path("hello");

    auto* asset = storage.get(handle);

    ASSERT_TRUE(asset);
    EXPECT_EQ(asset->name, "hello");
}