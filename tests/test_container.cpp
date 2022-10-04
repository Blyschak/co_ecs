#include <catch2/catch_all.hpp>
#include <co_ecs/co_ecs.hpp>

using namespace co_ecs;
using namespace co_ecs::detail;

TEST_CASE("Hash map", "[hash_map]") {
    hash_map<std::string, std::string> ht;

    SECTION("Empty hash map") {
        REQUIRE(ht.size() == 0);
        REQUIRE(ht.empty());
        REQUIRE(ht.begin() == ht.end());
    }

    SECTION("Insertion") {
        auto [insert_iter, is_inserted] = ht.insert({ "hello", "world" });
        REQUIRE(is_inserted);
        REQUIRE(insert_iter->first == "hello");
        REQUIRE(insert_iter->second == "world");
        REQUIRE(ht["hello"] == "world");
        REQUIRE_FALSE(ht.empty());
        REQUIRE(ht.size() == 1);

        std::tie(insert_iter, is_inserted) = ht.insert({ "hello", "there" });
        REQUIRE_FALSE(is_inserted);
        REQUIRE(insert_iter->first == "hello");
        REQUIRE(insert_iter->second == "world");
        REQUIRE(ht["hello"] == "world");
        REQUIRE_FALSE(ht.empty());
        REQUIRE(ht.size() == 1);
    }

    SECTION("Emplacement") {
        auto [insert_iter, is_inserted] = ht.emplace("hello", "world");
        REQUIRE(is_inserted);
        REQUIRE(insert_iter->first == "hello");
        REQUIRE(insert_iter->second == "world");
        REQUIRE(ht["hello"] == "world");
        REQUIRE_FALSE(ht.empty());
        REQUIRE(ht.size() == 1);

        std::tie(insert_iter, is_inserted) = ht.emplace("hello", "there");
        REQUIRE_FALSE(is_inserted);
        REQUIRE(insert_iter->first == "hello");
        REQUIRE(insert_iter->second == "world");
        REQUIRE(ht["hello"] == "world");
        REQUIRE_FALSE(ht.empty());
        REQUIRE(ht.size() == 1);

        std::tie(insert_iter, is_inserted) =
            ht.emplace(std::piecewise_construct, std::forward_as_tuple("hey"), std::forward_as_tuple(5, 'c'));
        REQUIRE(is_inserted);
        REQUIRE(insert_iter->first == "hey");
        REQUIRE(insert_iter->second == "ccccc");
        REQUIRE(is_inserted);
        REQUIRE(ht["hey"] == "ccccc");
        REQUIRE_FALSE(ht.empty());
        REQUIRE(ht.size() == 2);
    }

    SECTION("Construct with an allocator") {
        hash_map<std::string, std::string> ht{ std::allocator<std::pair<std::string, std::string>>() };
        auto [insert_iter, is_inserted] = ht.emplace("hello", "world");

        REQUIRE(is_inserted);
        REQUIRE(insert_iter->first == "hello");
        REQUIRE(insert_iter->second == "world");
        REQUIRE(ht["hello"] == "world");
        REQUIRE_FALSE(ht.empty());
        REQUIRE(ht.size() == 1);
    }

    SECTION("Construct from initializer list") {
        hash_map<std::string, std::string> ht = { { "hello", "world" }, { "hey", "there" } };

        REQUIRE(ht["hey"] == "there");
        REQUIRE(ht["hello"] == "world");
        REQUIRE(ht.size() == 2);
    }

    SECTION("Construct from iterators") {
        std::vector<std::pair<std::string, std::string>> vec = { { "hello", "world" }, { "hey", "there" } };
        hash_map<std::string, std::string> ht{ vec.begin(), vec.end() };

        REQUIRE(ht.size() == 2);
        REQUIRE(ht["hey"] == "there");
        REQUIRE(ht["hello"] == "world");
    }

    SECTION("Copy construction") {
        hash_map<std::string, std::vector<std::string>> ht = { { "hello", { "world", "!" } },
            { "hey", { "there", "!" } } };
        decltype(ht) copy_ht(ht);

        REQUIRE(ht.size() == 2);
        REQUIRE(ht["hey"][0] == "there");
        REQUIRE(ht["hello"][0] == "world");

        REQUIRE(copy_ht.size() == 2);
        REQUIRE(copy_ht["hey"][0] == "there");
        REQUIRE(copy_ht["hello"][0] == "world");
    }

    SECTION("Copy assignment") {
        hash_map<std::string, std::vector<std::string>> ht = { { "hello", { "world", "!" } },
            { "hey", { "there", "!" } } };
        auto copy_ht = ht;

        REQUIRE(ht.size() == 2);
        REQUIRE(ht["hey"][0] == "there");
        REQUIRE(ht["hello"][0] == "world");

        REQUIRE(copy_ht.size() == 2);
        REQUIRE(copy_ht["hey"][0] == "there");
        REQUIRE(copy_ht["hello"][0] == "world");
    }

    SECTION("Move construction") {
        // unique_ptr can only be moved
        hash_map<std::string, std::unique_ptr<std::string>> ht;
        ht.emplace("hello", std::make_unique<std::string>("world"));
        ht.emplace("hey", std::make_unique<std::string>("there"));
        decltype(ht) move_ht(std::move(ht));

        REQUIRE(move_ht.size() == 2);
        REQUIRE(*move_ht["hey"] == "there");
        REQUIRE(*move_ht["hello"] == "world");
    }

    SECTION("Move assignment") {
        // unique_ptr can only be moved
        hash_map<std::string, std::unique_ptr<std::string>> ht;
        ht.emplace("hello", std::make_unique<std::string>("world"));
        ht.emplace("hey", std::make_unique<std::string>("there"));
        auto move_ht = std::move(ht);

        REQUIRE(move_ht.size() == 2);
        REQUIRE(*move_ht["hey"] == "there");
        REQUIRE(*move_ht["hello"] == "world");
    }

    SECTION("Emplace default constructed value") {
        ht.emplace(std::piecewise_construct, std::forward_as_tuple("hello"), std::forward_as_tuple());
        REQUIRE(ht.size() == 1);
        REQUIRE(ht["hello"] == "");
    }

    SECTION("Emplace default constructed value with operator[]") {
        ht["hello"];
        REQUIRE(ht.size() == 1);
        REQUIRE(ht["hello"] == "");
    }

    SECTION("Find value in hash map") {
        hash_map<int, int> ht;
        ht.emplace(1, 1);
        ht.emplace(2, 4);
        ht.emplace(4, 16);

        REQUIRE(ht.find(1) != ht.end());
        REQUIRE(ht.find(1)->second == 1);
        REQUIRE(ht.find(2) != ht.end());
        REQUIRE(ht.find(2)->second == 4);
        REQUIRE(ht.find(4) != ht.end());
        REQUIRE(ht.find(4)->second == 16);
        REQUIRE(ht.find(3) == ht.end());
    }

    SECTION("Iterate over empty hash map") {
        hash_map<int, int> ht{};
        auto res = std::accumulate(
            ht.begin(), ht.end(), 0, [](const auto res, const auto& entry) { return entry.second + res; });
        REQUIRE(res == 0);
    }

    SECTION("Iterate over hash map containing values") {
        hash_map<int, int> ht{
            { 1, 1 },
            { 2, 4 },
            { 3, 9 },
            { 4, 16 },
            { 5, 25 },
        };
        auto res = std::accumulate(
            ht.begin(), ht.end(), 0, [](const auto res, const auto& entry) { return entry.second + res; });
        REQUIRE(res == 55);
    }

    SECTION("Insertion and erasure") {
        hash_map<std::string, int> ht{
            { "1", 1 },
            { "2", 2 },
            { "3", 3 },
            { "4", 4 },
            { "5", 5 },
        };

        auto res = std::accumulate(
            ht.begin(), ht.end(), 0, [](const auto res, const auto& entry) { return entry.second + res; });
        REQUIRE(res == 15);

        for (int i = 6; i < 100; i++) {
            ht.emplace(std::to_string(i), i);
        }

        res = std::accumulate(
            ht.begin(), ht.end(), 0, [](const auto res, const auto& entry) { return entry.second + res; });
        REQUIRE(res == 4950);

        REQUIRE(ht.find("4") != ht.end());
        REQUIRE(ht.find("4")->second == 4);

        ht.erase("4");
        REQUIRE(ht.find("4") == ht.end());

        REQUIRE(ht.find("5") != ht.end());
        REQUIRE(ht.find("5")->second == 5);

        ht.erase(ht.find("5"));
        REQUIRE(ht.find("5") == ht.end());

        REQUIRE(ht.find("90") != ht.end());

        ht.erase(ht.find("90"));
        REQUIRE(ht.find("90") == ht.end());

        ht.erase(ht.find("1"));
        REQUIRE(ht.find("1") == ht.end());

        for (int i = 0; i < 100; i++) {
            ht.erase(std::to_string(i));
        }
        REQUIRE(ht.empty());
        REQUIRE(ht.size() == 0);

        res = std::accumulate(
            ht.begin(), ht.end(), 0, [](const auto res, const auto& entry) { return entry.second + res; });
        REQUIRE(res == 0);
    }

    SECTION("Clear the hash map") {
        ht.insert({ "hello", "world" });
        ht.insert({ "hey", "there" });
        REQUIRE(ht.size() == 2);

        ht.clear();
        REQUIRE(ht.size() == 0);
    }
}

TEST_CASE("Sparse Map", "[sparse_map]") {
    sparse_map<unsigned int, int> map{};

    SECTION("Emplacement and erasure") {
        REQUIRE(map.empty());
        REQUIRE_FALSE(map.contains(5));
        REQUIRE_FALSE(map.contains(6));
        REQUIRE_FALSE(map.contains(7));

        map.emplace(5, 25);
        map.emplace(6, 36);

        REQUIRE(map.contains(5));
        REQUIRE(map.contains(6));
        REQUIRE_FALSE(map.contains(7));
        REQUIRE_FALSE(map.empty());

        map.erase(5);

        REQUIRE_FALSE(map.contains(5));
        REQUIRE(map.contains(6));
        REQUIRE_FALSE(map.contains(7));
        REQUIRE_FALSE(map.empty());

        map.clear();

        REQUIRE(map.empty());
        REQUIRE_FALSE(map.contains(5));
        REQUIRE_FALSE(map.contains(6));
        REQUIRE_FALSE(map.contains(7));
    }

    SECTION("Sparse map iteration") {
        map = { { 1, 1 }, { 2, 4 }, { 3, 9 }, { 4, 16 }, { 5, 25 } };
        map.emplace(6, 36);
        map.erase(6);
        REQUIRE(std::accumulate(map.cbegin(), map.cend(), 0, [](auto res, const auto& kv) { return res + kv.first; })
                == 15);
        REQUIRE(std::accumulate(map.cbegin(), map.cend(), 0, [](auto res, const auto& kv) { return res + kv.second; })
                == 55);
    }

    SECTION("Insert non-copyable element") {
        sparse_map<unsigned int, std::unique_ptr<int>> map{};

        map.emplace(5, std::make_unique<int>(5));
        REQUIRE(map.contains(5));
    }
}

TEST_CASE("Dynamic bitset", "[dynamic_bitset]") {
    dynamic_bitset bitset;

    SECTION("Set and test bits") {
        REQUIRE_FALSE(bitset.test(5));
        REQUIRE_FALSE(bitset.test(3));
        REQUIRE_FALSE(bitset.test(100));
        REQUIRE_FALSE(bitset.test(50));

        bitset.set(5).set(3).set(100);
        REQUIRE(bitset.test(5));
        REQUIRE(bitset.test(3));
        REQUIRE(bitset.test(100));
        REQUIRE_FALSE(bitset.test(50));

        bitset.set(5, false);
        REQUIRE_FALSE(bitset.test(5));
        REQUIRE(bitset.test(3));
        REQUIRE(bitset.test(100));
        REQUIRE_FALSE(bitset.test(50));
    }

    SECTION("Hash") {
        dynamic_bitset bitset;
        bitset.set(5).set(3).set(100);

        std::hash<dynamic_bitset<>> hasher{};
        hasher(bitset);
    }
}

TEST_CASE("Dynamic bitset equality", "Test equality of bitsets") {
    dynamic_bitset bits1;
    dynamic_bitset bits2;
    std::hash<dynamic_bitset<>> hasher{};

    SECTION("Test equality and hash equality of {14, 500}") {
        bits1.set(100);
        bits1.set(500);
        bits1.set(14);
        bits1.set(100, false);

        REQUIRE(bits1 != bits2);

        bits2.set(14);

        REQUIRE(bits1 != bits2);

        bits2.set(500);

        REQUIRE(bits1 == bits2);

        REQUIRE(hasher(bits1) == hasher(bits2));

        bits1.set(14, false);

        REQUIRE(bits1 != bits2);

        bits1.set(500, false);

        REQUIRE(bits1 != bits2);

        bits2.set(14, false);

        REQUIRE(bits1 != bits2);

        bits2.set(500, false);

        REQUIRE(bits1 == bits2);
        REQUIRE(hasher(bits1) == hasher(bits2));
    }

    SECTION("Test equality and hash equality of {14}") {
        bits1.set(100);
        bits1.set(500);
        bits1.set(14);
        bits1.set(500, false);
        bits1.set(100, false);

        REQUIRE(bits1 != bits2);

        bits2.set(14);

        REQUIRE(bits1 == bits2);

        REQUIRE(hasher(bits1) == hasher(bits2));
    }
}

// TEST(sparse_map, iteration) {
//     co_ecs::detail::sparse_map<unsigned int, unsigned int> map = { { 1, 1 }, { 2, 4 }, { 3, 9 }, { 4, 16 }, { 5, 25 }
//     }; map.emplace(6, 36); map.erase(6); EXPECT_EQ(
//         std::accumulate(map.cbegin(), map.cend(), 0, [](auto res, const auto& kv) { return res + kv.first; }), 15);
//     EXPECT_EQ(
//         std::accumulate(map.cbegin(), map.cend(), 0, [](auto res, const auto& kv) { return res + kv.second; }), 55);
// }

// TEST(sparse_map, insert_non_copiable) {
//     co_ecs::detail::sparse_map<unsigned int, std::unique_ptr<int>> map{};

//     map.emplace(5, std::make_unique<int>(5));
//     EXPECT_TRUE(map.contains(5));
// }

// TEST_CASE("Hash map", "benchmarks") {
//     hash_map<int, int> table;
//     int i = 0;

//     BENCHMARK("Emplace key") {
//         table.emplace(i++, 100);
//     };

//     BENCHMARK("Emplace key that already exists") {
//         table.emplace(0, 100);
//     };

//     BENCHMARK("call operator[] with key that already exists") {
//         return table[0];
//     };

//     table = hash_map<int, int>{};
//     table.clear();
//     for (int i = 0; i < 100; i++) {
//         table.emplace(i, i*i);
//     }

//     BENCHMARK("Iterate over hash map containing 100 elements") {
//         return std::accumulate(table.begin(), table.end(), 0,
//             [](const auto& sum, const auto& entry) { return sum + entry.second; });
//     };
// }