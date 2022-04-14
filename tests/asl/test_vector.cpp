#include <cobalt/asl/vector.hpp>

#include <gtest/gtest.h>

#include <numeric>

struct test_struct {
    test_struct(int a) : _a(a) {
        if (a == 5) {
            throw 1;
        }
    }

    test_struct operator+(test_struct rhs) {
        return test_struct(_a + rhs._a);
    }
    int _a;
};

TEST(vector, vector_exceptions) {
    cobalt::asl::vector<test_struct> v;
    v.emplace_back(1);
    v.emplace_back(2);
    v.emplace_back(3);
    EXPECT_EQ(v.size(), 3);
    EXPECT_EQ(std::accumulate(v.begin(), v.end(), test_struct(0))._a, 6);
    EXPECT_THROW(v.emplace_back(5), int);
    EXPECT_EQ(v.size(), 3);
    EXPECT_EQ(std::accumulate(v.begin(), v.end(), test_struct(0))._a, 6);
}

TEST(vector, vector_basic) {
    cobalt::asl::vector<int> v;
    EXPECT_EQ(v.size(), 0);
    EXPECT_EQ(std::accumulate(v.begin(), v.end(), 0), 0);
    v.resize(10);
    EXPECT_EQ(v.size(), 10);
    EXPECT_EQ(v.capacity(), 10);
    EXPECT_EQ(std::accumulate(v.begin(), v.end(), 0), 0);
    v.clear();
    v.resize(10, 1);
    EXPECT_EQ(v.size(), 10);
    EXPECT_EQ(v.capacity(), 10);
    EXPECT_EQ(std::accumulate(v.begin(), v.end(), 0), 10);
    v.clear();
    EXPECT_EQ(v.size(), 0);
    EXPECT_EQ(v.capacity(), 10);
    v.push_back(1);
    v.push_back(2);
    v.push_back(3);
    EXPECT_EQ(v.size(), 3);
    EXPECT_EQ(v.capacity(), 10);
    EXPECT_EQ(std::accumulate(v.begin(), v.end(), 0), 6);
    v.pop_back();
    EXPECT_EQ(v.size(), 2);
    EXPECT_EQ(v.capacity(), 10);
    EXPECT_EQ(std::accumulate(v.begin(), v.end(), 0), 3);
    v.emplace_back(3);
    v.emplace_back(4);
    EXPECT_EQ(v.size(), 4);
    EXPECT_EQ(v.capacity(), 10);
    EXPECT_EQ(std::accumulate(v.begin(), v.end(), 0), 10);
    v.emplace_back(5);
    v.emplace_back(6);
    v.emplace_back(7);
    v.emplace_back(8);
    v.emplace_back(9);
    v.emplace_back(10);
    EXPECT_EQ(std::accumulate(v.begin(), v.end(), 0), 55);
    v.emplace_back(11);
    EXPECT_EQ(v.size(), 11);
    EXPECT_EQ(v.capacity(), 20);
    EXPECT_EQ(std::accumulate(v.begin(), v.end(), 0), 66);
    v.clear();
    EXPECT_EQ(v.size(), 0);
    EXPECT_EQ(v.capacity(), 20);

    v = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10 };
    EXPECT_EQ(v.size(), 10);
    EXPECT_EQ(v.capacity(), 20);
    v.shrink_to_fit();
    EXPECT_EQ(std::accumulate(v.begin(), v.end(), 0), 55);
}

TEST(vector, vector_construction) {
    auto val = 1;
    std::size_t size = 5;
    cobalt::asl::vector<int> v(size, val);
    EXPECT_EQ(v.size(), 5);
    EXPECT_EQ(v.capacity(), 5);
    EXPECT_EQ(std::accumulate(v.begin(), v.end(), 0), 5);

    cobalt::asl::vector<int> v1(5, 6);
    EXPECT_EQ(v1.size(), 5);
    EXPECT_EQ(v1.capacity(), 5);
    EXPECT_EQ(std::accumulate(v1.begin(), v1.end(), 0), 30);

    cobalt::asl::vector<int> v2(v1);
    EXPECT_EQ(v2.size(), 5);
    EXPECT_EQ(v2.capacity(), 5);
    EXPECT_NE(v1.data(), nullptr);
    EXPECT_EQ(std::accumulate(v2.begin(), v2.end(), 0), 30);

    cobalt::asl::vector<int> v3(std::move(v2));
    EXPECT_EQ(v3.size(), 5);
    EXPECT_EQ(v3.capacity(), 5);
    EXPECT_EQ(v2.data(), nullptr);
    EXPECT_EQ(std::accumulate(v3.begin(), v3.end(), 0), 30);

    v3.assign({ 1, 2, 3 });
    EXPECT_EQ(v3.size(), 3);
    EXPECT_EQ(v3.capacity(), 5);
    EXPECT_EQ(std::accumulate(v3.begin(), v3.end(), 0), 6);

    v3.assign(10, 1);
    EXPECT_EQ(v3.size(), 10);
    EXPECT_EQ(v3.capacity(), 10);
    EXPECT_EQ(std::accumulate(v3.begin(), v3.end(), 0), 10);

    v3.insert(v3.begin() + 2, 3);
    EXPECT_EQ(v3.size(), 11);
    EXPECT_EQ(v3.capacity(), 20);
    EXPECT_EQ(v3[2], 3);
    EXPECT_EQ(std::accumulate(v3.begin(), v3.end(), 0), 13);

    v3.insert(v3.begin(), 5);
    EXPECT_EQ(v3.size(), 12);
    EXPECT_EQ(v3.capacity(), 20);
    EXPECT_EQ(v3[0], 5);
    EXPECT_EQ(std::accumulate(v3.begin(), v3.end(), 0), 18);

    v3.insert(v3.end(), 2);
    EXPECT_EQ(v3.size(), 13);
    EXPECT_EQ(v3.capacity(), 20);
    EXPECT_EQ(v3[v3.size() - 1], 2);
    EXPECT_EQ(std::accumulate(v3.begin(), v3.end(), 0), 20);

    v3.emplace(v3.begin() + 2, 15);
    EXPECT_EQ(v3.size(), 14);
    EXPECT_EQ(v3.capacity(), 20);
    EXPECT_EQ(v3[v3.size() - 1], 2);
    EXPECT_EQ(v3[2], 15);
    EXPECT_EQ(std::accumulate(v3.begin(), v3.end(), 0), 35);
}

TEST(vector, swap_erase) {
    cobalt::asl::vector<int> v{ 1, 2, 3, 4, 5 };
    EXPECT_EQ(v.size(), 5);
    EXPECT_EQ(v.capacity(), 5);
    EXPECT_EQ(v[2], 3);
    EXPECT_EQ(std::accumulate(v.begin(), v.end(), 0), 15);

    v.swap_erase(v.begin() + 2);
    EXPECT_EQ(v.size(), 4);
    EXPECT_EQ(v.capacity(), 5);
    EXPECT_EQ(std::accumulate(v.begin(), v.end(), 0), 12);
    EXPECT_EQ(v[2], 5);
}

TEST(vector, shrink_to_fit) {
    cobalt::asl::vector<std::size_t> v{ 1, 2, 3, 4, 5, 6, 7, 8, 9, 10 };
    EXPECT_EQ(v.size(), 10);
    EXPECT_EQ(v.capacity(), 10);
    EXPECT_EQ(std::accumulate(v.begin(), v.end(), 0), 55);

    v.emplace_back(11);
    EXPECT_EQ(v.size(), 11);
    EXPECT_EQ(v.capacity(), 20);

    v.shrink_to_fit();
    EXPECT_EQ(v.size(), 11);
    EXPECT_EQ(v.capacity(), 11);
    EXPECT_EQ(std::accumulate(v.begin(), v.end(), 0), 66);
}

TEST(vector, assignment_operator) {
    cobalt::asl::vector<int> v1{ 1, 2, 3, 4, 5 };
    cobalt::asl::vector<int> v2;
    v2 = v1;
    EXPECT_EQ(v2.size(), 5);
    EXPECT_EQ(v2.capacity(), 5);
    EXPECT_EQ(v2[2], 3);
    EXPECT_EQ(std::accumulate(v2.begin(), v2.end(), 0), 15);

    cobalt::asl::vector<int> v3{ 1, 2 };
    v2 = v3;
    EXPECT_EQ(v2.size(), 2);
    EXPECT_EQ(v2.capacity(), 5);
    EXPECT_EQ(v2[1], 2);
    EXPECT_EQ(std::accumulate(v2.begin(), v2.end(), 0), 3);

    cobalt::asl::vector<int> v4{ 1, 2, 3 };
    v2 = v4;
    EXPECT_EQ(v2.size(), 3);
    EXPECT_EQ(v2.capacity(), 5);
    EXPECT_EQ(v2[1], 2);
    EXPECT_EQ(std::accumulate(v2.begin(), v2.end(), 0), 6);
}

TEST(vector, initializer_list_assignment) {
    cobalt::asl::vector<int> v1{ 1, 2, 3, 4, 5 };
    v1 = { 5, 6, 7, 8 };
    EXPECT_EQ(v1.size(), 4);
    EXPECT_EQ(v1.capacity(), 5);
    EXPECT_EQ(v1[2], 7);
    EXPECT_EQ(std::accumulate(v1.begin(), v1.end(), 0), 26);
}

TEST(vactor, insert_multiple) {
    cobalt::asl::vector<int> v{ { 1, 2, 3 } };
    EXPECT_EQ(v.size(), 3);
    EXPECT_EQ(v.capacity(), 3);
    EXPECT_EQ(std::accumulate(v.begin(), v.end(), 0), 6);

    v.insert(v.begin(), 2, 3);
    EXPECT_EQ(v.size(), 5);
    EXPECT_EQ(v.capacity(), 5);
    EXPECT_EQ(std::accumulate(v.begin(), v.end(), 0), 12);
    EXPECT_EQ(v[0], 3);
    EXPECT_EQ(v[1], 3);
    EXPECT_EQ(v[2], 1);

    v.insert(v.begin() + 3, 5, 10);
    EXPECT_EQ(v.size(), 10);
    EXPECT_EQ(v.capacity(), 10);
    EXPECT_EQ(std::accumulate(v.begin(), v.end(), 0), 62);
    EXPECT_EQ(v[0], 3);
    EXPECT_EQ(v[1], 3);
    EXPECT_EQ(v[2], 1);
    EXPECT_EQ(v[3], 10);
    EXPECT_EQ(v[4], 10);
    EXPECT_EQ(v[5], 10);
    EXPECT_EQ(v[6], 10);
    EXPECT_EQ(v[7], 10);
    EXPECT_EQ(v[8], 2);
    EXPECT_EQ(v[9], 3);
}

TEST(vactor, insert_multiple_initializer_list) {
    cobalt::asl::vector<int> v{ { 1, 2, 3 } };
    EXPECT_EQ(v.size(), 3);
    EXPECT_EQ(v.capacity(), 3);
    EXPECT_EQ(std::accumulate(v.begin(), v.end(), 0), 6);

    v.insert(v.begin(), { 3, 3 });
    EXPECT_EQ(v.size(), 5);
    EXPECT_EQ(v.capacity(), 5);
    EXPECT_EQ(std::accumulate(v.begin(), v.end(), 0), 12);
    EXPECT_EQ(v[0], 3);
    EXPECT_EQ(v[1], 3);
    EXPECT_EQ(v[2], 1);

    v.insert(v.begin() + 3, { 10, 10, 10, 10, 10 });
    EXPECT_EQ(v.size(), 10);
    EXPECT_EQ(v.capacity(), 10);
    EXPECT_EQ(std::accumulate(v.begin(), v.end(), 0), 62);
    EXPECT_EQ(v[0], 3);
    EXPECT_EQ(v[1], 3);
    EXPECT_EQ(v[2], 1);
    EXPECT_EQ(v[3], 10);
    EXPECT_EQ(v[4], 10);
    EXPECT_EQ(v[5], 10);
    EXPECT_EQ(v[6], 10);
    EXPECT_EQ(v[7], 10);
    EXPECT_EQ(v[8], 2);
    EXPECT_EQ(v[9], 3);
}

TEST(vector, erase) {
    cobalt::asl::vector<int> v{ { 1, 2, 3, 4, 5, 6 } };
    v.erase(v.begin() + 2, v.begin() + 4);
    EXPECT_EQ(v.size(), 4);
    EXPECT_EQ(v.capacity(), 6);
    EXPECT_EQ(std::accumulate(v.begin(), v.end(), 0), 14);

    v.erase(v.begin());
    EXPECT_EQ(v.size(), 3);
    EXPECT_EQ(v.capacity(), 6);
    EXPECT_EQ(std::accumulate(v.begin(), v.end(), 0), 13);
}

TEST(vector, vector_with_vector) {
    cobalt::asl::vector<cobalt::asl::vector<int>> v{ { 1, 2 }, { 3, 4 } };
    v.emplace_back(5, 6);
    v.erase(v.begin());
}

TEST(vector, vector_with_unique_ptr) {
    cobalt::asl::vector<std::unique_ptr<int>> v;
    v.emplace_back(std::make_unique<int>(1));
    v.emplace_back(std::make_unique<int>(2));
    v.emplace_back(std::make_unique<int>(3));
    v.erase(v.begin());
}

TEST(vector, pair_with_const) {
    cobalt::asl::vector<std::pair<int, int>> v;
    v.emplace_back(1, 2);
    v.emplace_back(2, 3);
    v.swap_erase(v.begin());
    EXPECT_EQ(v.size(), 1);
    EXPECT_EQ(v.capacity(), 8);
    EXPECT_EQ(v[0].first, 2);
    EXPECT_EQ(v[0].second, 3);
}

TEST(vector, riters) {
    cobalt::asl::vector<int> v = { 4, 3, 2, 1 };
    auto it = v.rbegin();
    EXPECT_EQ(*it++, 1);
    EXPECT_EQ(*it++, 2);
    EXPECT_EQ(*it++, 3);
    EXPECT_EQ(*it++, 4);
    EXPECT_EQ(it, v.rend());
}