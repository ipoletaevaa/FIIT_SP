#include "gtest/gtest.h"

#include <list>
#include <random>
#include <vector>
#include <b_tree.h>

template<typename tvalue>
bool compare_obtain_results(
        std::vector<tvalue> const &expected,
        std::vector<tvalue> const &actual)
{
    if (expected.size() != actual.size()) {
        return false;
    }

    for (size_t i = 0; i < expected.size(); ++i) {
        if (expected[i] != actual[i]) {
            return false;
        }
    }

    return true;
}

template<typename tkey, typename tvalue>
struct test_data {
    tkey key;
    tvalue value;
    size_t depth, index;

    test_data(size_t d, size_t i, tkey k, tvalue v) : depth(d), index(i), key(k), value(v)
    {}
};

template<typename tkey, typename tvalue, typename comp, size_t t>
bool infix_const_iterator_test(
        B_tree<tkey, tvalue, comp, t> const &tree,
        std::vector<test_data<tkey, tvalue>> const &expected_result)
{
    auto end_infix = tree.cend();
    auto it = tree.cbegin();

    for (auto const &item: expected_result) {
        auto data = *it;

        if (it->first != item.key || it->second != item.value || it.depth() != item.depth || it.index() != item.index) {
            return false;
        }

        ++it;
    }

    return true;
}

TEST(bTreePositiveTests, test0)
{
    std::vector<test_data<int, std::string>> expected_result =
            {

            };

    B_tree<int, std::string, std::less<int>, 1024> tree(std::less<int>(), nullptr);

    EXPECT_TRUE(infix_const_iterator_test(tree, expected_result));
}

TEST(bTreePositiveTests, test1)
{
    // t=3: max_keys=5, mid=t=3 => separator at index 3
    // After inserting 1,2,15,3,4 root is full [1,2,3,4,15].
    // Split: left=[1,2,3], separator=4, right=[15].
    // Insert 27 into right => right=[15,27].
    // Tree: root={4}, left=[1,2,3], right=[15,27]
    std::vector<test_data<int, std::string>> expected_result =
            {
                    test_data<int, std::string>(1, 0, 1, "a"),
                    test_data<int, std::string>(1, 1, 2, "b"),
                    test_data<int, std::string>(1, 2, 3, "d"),
                    test_data<int, std::string>(0, 0, 4, "e"),
                    test_data<int, std::string>(1, 0, 15, "c"),
                    test_data<int, std::string>(1, 1, 27, "f")
            };

    B_tree<int, std::string, std::less<int>, 3> tree(std::less<int>(), nullptr);

    tree.emplace(1, std::string("a"));
    tree.emplace(2, std::string("b"));
    tree.emplace(15, std::string("c"));
    tree.emplace(3, std::string("d"));
    tree.emplace(4, std::string("e"));
    tree.emplace(27, std::string("f"));

    EXPECT_TRUE(infix_const_iterator_test(tree, expected_result));
}

TEST(bTreePositiveTests, test2)
{
    // t=5: max_keys=9, mid=t=5 => separator at index 5
    // After 9 inserts root is full [1,2,3,4,15,24,100,101,456].
    // Split: left=[1,2,3,4,15], separator=24, right=[100,101,456].
    // Insert 45,193,534 into right => right=[45,100,101,193,456,534].
    // Tree: root={24}, left=[1,2,3,4,15], right=[45,100,101,193,456,534]
    std::vector<test_data<int, std::string>> expected_result =
            {
                    test_data<int, std::string>(1, 0, 1, "a"),
                    test_data<int, std::string>(1, 1, 2, "b"),
                    test_data<int, std::string>(1, 2, 3, "d"),
                    test_data<int, std::string>(1, 3, 4, "e"),
                    test_data<int, std::string>(1, 4, 15, "c"),
                    test_data<int, std::string>(0, 0, 24, "g"),
                    test_data<int, std::string>(1, 0, 45, "k"),
                    test_data<int, std::string>(1, 1, 100, "f"),
                    test_data<int, std::string>(1, 2, 101, "j"),
                    test_data<int, std::string>(1, 3, 193, "l"),
                    test_data<int, std::string>(1, 4, 456, "h"),
                    test_data<int, std::string>(1, 5, 534, "m")
            };

    B_tree<int, std::string, std::less<int>, 5> tree(std::less<int>(), nullptr);

    tree.emplace(1, std::string("a"));
    tree.emplace(2, std::string("b"));
    tree.emplace(15, std::string("c"));
    tree.emplace(3, std::string("d"));
    tree.emplace(4, std::string("e"));
    tree.emplace(100, std::string("f"));
    tree.emplace(24, std::string("g"));
    tree.emplace(456, std::string("h"));
    tree.emplace(101, std::string("j"));
    tree.emplace(45, std::string("k"));
    tree.emplace(193, std::string("l"));
    tree.emplace(534, std::string("m"));

    EXPECT_TRUE(infix_const_iterator_test(tree, expected_result));
}

TEST(bTreePositiveTests, test3)
{
    // t=7: max_keys=13. Only 12 elements inserted, root never splits.
    // All 12 keys in root at depth=0, indices 0..11.
    std::vector<test_data<int, std::string>> expected_result =
            {
                    test_data<int, std::string>(0, 0, 1, "a"),
                    test_data<int, std::string>(0, 1, 2, "b"),
                    test_data<int, std::string>(0, 2, 3, "d"),
                    test_data<int, std::string>(0, 3, 4, "e"),
                    test_data<int, std::string>(0, 4, 15, "c"),
                    test_data<int, std::string>(0, 5, 24, "g"),
                    test_data<int, std::string>(0, 6, 45, "k"),
                    test_data<int, std::string>(0, 7, 100, "f"),
                    test_data<int, std::string>(0, 8, 101, "j"),
                    test_data<int, std::string>(0, 9, 193, "l"),
                    test_data<int, std::string>(0, 10, 456, "h"),
                    test_data<int, std::string>(0, 11, 534, "m")
            };

    B_tree<int, std::string, std::less<int>, 7> tree(std::less<int>(), nullptr);

    tree.emplace(1, std::string("a"));
    tree.emplace(2, std::string("b"));
    tree.emplace(15, std::string("c"));
    tree.emplace(3, std::string("d"));
    tree.emplace(4, std::string("e"));
    tree.emplace(100, std::string("f"));
    tree.emplace(24, std::string("g"));
    tree.emplace(456, std::string("h"));
    tree.emplace(101, std::string("j"));
    tree.emplace(45, std::string("k"));
    tree.emplace(193, std::string("l"));
    tree.emplace(534, std::string("m"));

    EXPECT_TRUE(infix_const_iterator_test(tree, expected_result));
}

TEST(bTreePositiveTests, test4)
{
    // t=3: max_keys=5, mid=t=3.
    // After 5 inserts root=[1,2,3,4,15] is full. Insert 100 triggers split:
    //   left=[1,2,3], sep=4, right=[15]. root={4}.
    //   Insert 100 -> right=[15,100].
    // After 7 inserts root has {4}, left=[1,2,3], right=[15,100].
    // Insert 24: right=[15,24,100]. Insert 456: right=[15,24,100,456].
    // Insert 101: right=[15,24,100,101,456] -- full (5 keys).
    // Insert 45: right is full, split right at mid=3:
    //   right_left=[15,24,100], sep=101, right_right=[456].
    //   root={4,101}, pointers=[left,right_left,right_right].
    //   Insert 45 -> pos in root: 45>4, 45<101 => child=right_left=[15,24,100].
    //   right_left=[15,24,45,100].
    // Insert 193: root={4,101}, 193>101 => child=right_right=[456] => [193,456].
    // Insert 534: right_right=[193,456,534].
    // Tree: root={4,101}
    //   left=[1,2,3]      depth=1, indices 0,1,2
    //   mid=[15,24,45,100] depth=1, indices 0,1,2,3
    //   right=[193,456,534] depth=1, indices 0,1,2
    std::vector<test_data<int, std::string>> expected_result =
            {
                    test_data<int, std::string>(1, 0, 1, "a"),
                    test_data<int, std::string>(1, 1, 2, "b"),
                    test_data<int, std::string>(1, 2, 3, "d"),
                    test_data<int, std::string>(0, 0, 4, "e"),
                    test_data<int, std::string>(1, 0, 15, "c"),
                    test_data<int, std::string>(1, 1, 24, "g"),
                    test_data<int, std::string>(1, 2, 45, "k"),
                    test_data<int, std::string>(1, 3, 100, "f"),
                    test_data<int, std::string>(0, 1, 101, "j"),
                    test_data<int, std::string>(1, 0, 193, "l"),
                    test_data<int, std::string>(1, 1, 456, "h"),
                    test_data<int, std::string>(1, 2, 534, "m")
            };

    B_tree<int, std::string, std::less<int>, 3> tree(std::less<int>(), nullptr);

    tree.emplace(1, std::string("a"));
    tree.emplace(2, std::string("b"));
    tree.emplace(15, std::string("c"));
    tree.emplace(3, std::string("d"));
    tree.emplace(4, std::string("e"));
    tree.emplace(100, std::string("f"));
    tree.emplace(24, std::string("g"));
    tree.emplace(456, std::string("h"));
    tree.emplace(101, std::string("j"));
    tree.emplace(45, std::string("k"));
    tree.emplace(193, std::string("l"));
    tree.emplace(534, std::string("m"));

    EXPECT_TRUE(infix_const_iterator_test(tree, expected_result));
}

TEST(bTreePositiveTests, test5)
{
    std::vector<test_data<int, std::string>> expected_result =
            {

            };

    B_tree<int, std::string, std::less<int>, 2> tree(std::less<int>(), nullptr);

    tree.emplace(1, std::string("a"));
    tree.emplace(2, std::string("b"));
    tree.emplace(15, std::string("c"));
    tree.emplace(3, std::string("d"));
    tree.emplace(4, std::string("e"));

    auto first_disposed = tree.at(2);
    auto second_disposed = tree.at(4);

    tree.erase(2);
    tree.erase(4);

    EXPECT_TRUE(infix_const_iterator_test(tree, expected_result));
}

TEST(bTreePositiveTests, test6)
{
    std::vector<test_data<int, std::string>> expected_result =
            {
                    test_data<int, std::string>(1, 0, 2, "b"),
                    test_data<int, std::string>(1, 1, 3, "d"),
                    test_data<int, std::string>(1, 2, 4, "e"),
                    test_data<int, std::string>(0, 0, 15, "c"),
                    test_data<int, std::string>(1, 0, 45, "k"),
                    test_data<int, std::string>(1, 1, 101, "j"),
                    test_data<int, std::string>(1, 2, 456, "h"),
                    test_data<int, std::string>(1, 3, 534, "m")
            };

    B_tree<int, std::string, std::less<int>, 4> tree(std::less<int>(), nullptr);

    tree.emplace(1, std::string("a"));
    tree.emplace(2, std::string("b"));
    tree.emplace(15, std::string("c"));
    tree.emplace(3, std::string("d"));
    tree.emplace(4, std::string("e"));
    tree.emplace(100, std::string("f"));
    tree.emplace(24, std::string("g"));
    tree.emplace(456, std::string("h"));
    tree.emplace(101, std::string("j"));
    tree.emplace(45, std::string("k"));
    tree.emplace(193, std::string("l"));
    tree.emplace(534, std::string("m"));

    auto first_disposed = std::move(tree.at(1));
    auto second_disposed = std::move(tree.at(100));
    auto third_disposed = std::move(tree.at(193));
    auto fourth_disposed = std::move(tree.at(24));

    tree.erase(1);
    tree.erase(100);
    tree.erase(193);
    tree.erase(24);

    EXPECT_TRUE(infix_const_iterator_test(tree, expected_result));

    EXPECT_TRUE(first_disposed == "a");
    EXPECT_TRUE(second_disposed == "f");
    EXPECT_TRUE(third_disposed == "l");
    EXPECT_TRUE(fourth_disposed == "g");
}

TEST(bTreePositiveTests, test7)
{
    std::vector<std::string> expected_result =
            {
                    "g",
                    "d",
                    "e",
                    " ",
                    "l",
                    "a",
                    "b",
                    "y"
            };

    B_tree<int, std::string, std::less<int>, 5> tree(std::less<int>(), nullptr);

    tree.emplace(1, std::move(std::string("a")));
    tree.emplace(2, std::move(std::string("b")));
    tree.emplace(15, std::move(std::string("c")));
    tree.emplace(3, std::move(std::string("d")));
    tree.emplace(4, std::move(std::string("e")));
    tree.emplace(100, std::move(std::string(" ")));
    tree.emplace(24, std::move(std::string("g")));
    tree.emplace(-456, std::move(std::string("h")));
    tree.emplace(101, std::move(std::string("j")));
    tree.emplace(-45, std::move(std::string("k")));
    tree.emplace(-193, std::move(std::string("l")));
    tree.emplace(534, std::move(std::string("m")));
    tree.emplace(1000, std::move(std::string("y")));

    std::vector<std::string> actual_result =
            {
                    tree.at(24),
                    tree.at(3),
                    tree.at(4),
                    tree.at(100),
                    tree.at(-193),
                    tree.at(1),
                    tree.at(2),
                    tree.at(1000)
            };

    EXPECT_TRUE(compare_obtain_results(expected_result, actual_result));
}

TEST(bTreePositiveTests, test8)
{
    std::vector<std::string> expected_result =
            {
                    "y",
                    "l",
                    "a",
                    "g",
                    "k",
                    "b",
                    "c",
                    "h"
            };

    B_tree<int, std::string, std::less<int>, 4> tree(std::less<int>(), nullptr);

    tree.emplace(1, std::string("a"));
    tree.emplace(2, std::string("b"));
    tree.emplace(15, std::string("c"));
    tree.emplace(3, std::string("d"));
    tree.emplace(4, std::string("e"));
    tree.emplace(100, std::string(" "));
    tree.emplace(24, std::string("g"));
    tree.emplace(-456, std::string("h"));
    tree.emplace(101, std::string("j"));
    tree.emplace(-45, std::string("k"));
    tree.emplace(-193, std::string("l"));
    tree.emplace(534, std::string("m"));
    tree.emplace(1000, std::string("y"));

    std::vector<std::string> actual_result =
            {
                    tree.at(1000),
                    tree.at(-193),
                    tree.at(1),
                    tree.at(24),
                    tree.at(-45),
                    tree.at(2),
                    tree.at(15),
                    tree.at(-456)
            };

    EXPECT_TRUE(compare_obtain_results(expected_result, actual_result));
}

TEST(bTreePositiveTests, test9)
{
    // t=5: max_keys=9, mid=t=5.
    // After 9 inserts root=[1,2,3,4,15,24,100,101,456] is full.
    // 10th insert (45) triggers split: left=[1,2,3,4,15], sep=24, right=[100,101,456].
    // Insert 45 -> right=[45,100,101,456].
    // Insert 193 -> right=[45,100,101,193,456].
    // Insert 534 -> right=[45,100,101,193,456,534].
    // Full iteration must return all 12 elements in sorted order.
    std::vector<B_tree<int, std::string>::value_type> expected_result =
            {
                    {1,   "a"},
                    {2,   "b"},
                    {3,   "d"},
                    {4,   "e"},
                    {15,  "c"},
                    {24,  "g"},
                    {45,  "k"},
                    {100, "f"},
                    {101, "j"},
                    {193, "l"},
                    {456, "h"},
                    {534, "m"},
            };

    B_tree<int, std::string, std::less<int>, 5> tree(std::less<int>(), nullptr);

    tree.emplace(1, std::string("a"));
    tree.emplace(2, std::string("b"));
    tree.emplace(15, std::string("c"));
    tree.emplace(3, std::string("d"));
    tree.emplace(4, std::string("e"));
    tree.emplace(100, std::string("f"));
    tree.emplace(24, std::string("g"));
    tree.emplace(456, std::string("h"));
    tree.emplace(101, std::string("j"));
    tree.emplace(45, std::string("k"));
    tree.emplace(193, std::string("l"));
    tree.emplace(534, std::string("m"));

    auto b = tree.begin();
    auto e = tree.end();
    std::vector<decltype(tree)::value_type> actual_result(b, e);

    EXPECT_TRUE(compare_obtain_results(expected_result, actual_result));
}

TEST(bTreeNegativeTests, test1)
{
    B_tree<int, std::string, std::less<int>, 3> tree(std::less<int>(), nullptr);

    tree.emplace(1, std::string("a"));
    tree.emplace(2, std::string("b"));
    tree.emplace(15, std::string("c"));
    tree.emplace(3, std::string("d"));
    tree.emplace(4, std::string("e"));

    EXPECT_EQ(tree.erase(45), tree.end());
}

TEST(bTreeNegativeTests, test3)
{
    B_tree<int, std::string, std::less<int>, 4> tree(std::less<int>(), nullptr);

    tree.emplace(1, std::string("a"));
    tree.emplace(2, std::string("b"));
    tree.emplace(15, std::string("c"));
    tree.emplace(3, std::string("d"));
    tree.emplace(4, std::string("e"));
    tree.emplace(100, std::string(" "));
    tree.emplace(24, std::string("g"));
    tree.emplace(-456, std::string("h"));
    tree.emplace(101, std::string("j"));
    tree.emplace(-45, std::string("k"));
    tree.emplace(-193, std::string("l"));
    tree.emplace(534, std::string("m"));
    tree.emplace(1000, std::string("y"));

    EXPECT_EQ(tree.erase(1001), tree.end());
}

int main(
        int argc,
        char **argv)
{
    testing::InitGoogleTest(&argc, argv);

    return RUN_ALL_TESTS();
}
