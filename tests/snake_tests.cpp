#include "gtest/gtest.h"
#include "../snake.h"   // adjust path if necessary
#include <fstream>
#include <vector>
#include <algorithm>
#include <cstdio> // for remove()

using namespace std;

// Helper to read high_scores file into vector<int>
static vector<int> read_high_scores_file(const string &path = "high_scores.txt") {
    vector<int> res;
    ifstream in(path);
    if (!in.is_open()) return res;
    int v;
    while (in >> v) res.push_back(v);
    in.close();
    return res;
}

TEST(GetNextHead, RightWrap) {
    pair<int,int> cur = {0, 4};
    auto nxt = SnakeGame::getNextHead(cur, 'r', 5);
    // from col 4 on grid 5, right wraps to 0
    EXPECT_EQ(nxt.first, 0);
    EXPECT_EQ(nxt.second, 0);
}

TEST(GetNextHead, LeftWrap) {
    pair<int,int> cur = {0, 0};
    auto nxt = SnakeGame::getNextHead(cur, 'l', 5);
    EXPECT_EQ(nxt.first, 0);
    EXPECT_EQ(nxt.second, 4);
}

TEST(GetNextHead, UpWrap) {
    pair<int,int> cur = {0, 2};
    auto nxt = SnakeGame::getNextHead(cur, 'u', 5);
    EXPECT_EQ(nxt.first, 4);
    EXPECT_EQ(nxt.second, 2);
}

TEST(GetNextHead, DownWrap) {
    pair<int,int> cur = {4, 2};
    auto nxt = SnakeGame::getNextHead(cur, 'd', 5);
    EXPECT_EQ(nxt.first, 0);
    EXPECT_EQ(nxt.second, 2);
}

TEST(SpawnFood, NotOnSnakeOrPoison) {
    SnakeGame g(5);
    // Place snake occupying some cells
    g.snake.clear();
    g.snake.push_back({0,0});
    g.snake.push_back({0,1});
    g.snake.push_back({1,0});
    // place poison at a fixed place
    g.poisonous_food = {2,2};

    // Spawn food multiple times to increase chance it's valid
    for (int i = 0; i < 5; ++i) {
        g.spawn_food();
        // Should not be on snake
        auto it = find(g.snake.begin(), g.snake.end(), g.food);
        EXPECT_TRUE(it == g.snake.end());
        // Should not equal poison
        EXPECT_FALSE(g.food == g.poisonous_food);
    }
}

TEST(SpawnPoison, NotOnSnakeOrFood) {
    SnakeGame g(5);
    g.snake.clear();
    g.snake.push_back({3,3});
    g.snake.push_back({3,4});
    // set food at a fixed place
    g.food = {1,1};
    for (int i = 0; i < 5; ++i) {
        g.spawn_poison();
        auto it = find(g.snake.begin(), g.snake.end(), g.poisonous_food);
        EXPECT_TRUE(it == g.snake.end());
        EXPECT_FALSE(g.poisonous_food == g.food);
    }
}

TEST(UpdateHighScores, WritesTop10WithCurrentScore) {
    // prepare a temporary high_scores.txt
    const string path = "high_scores.txt";
    // Remove if exists
    remove(path.c_str());

    // Write some initial values
    {
        ofstream out(path);
        out << 50 << endl;
        out << 40 << endl;
        out << 30 << endl;
        out << 20 << endl;
        out << 10 << endl;
        out.close();
    }

    SnakeGame g(5);
    g.score = 999; // atomic<int> assignment

    g.update_high_scores();

    vector<int> hs = read_high_scores_file(path);
    // first element should be the newly inserted highest value
    ASSERT_FALSE(hs.empty());
    EXPECT_EQ(hs[0], 999);

    // file must not exceed 10 entries
    EXPECT_LE(hs.size(), 10u);

    // Clean up
    remove(path.c_str());
}
