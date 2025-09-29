#ifndef SNAKE_H
#define SNAKE_H

#include <iostream>
#include <vector>
#include <chrono>
#include <thread>
#include <cstdlib>
#if defined(_WIN32) || defined(_WIN64)
    #include <conio.h>   // Windows: use _getch()
#else
    #include <termios.h> // Linux/Mac
    #include <unistd.h>
#endif
#include <map>
#include <deque>
#include <algorithm>
#include <fstream>
#include <atomic>
#include <mutex>
#include <random>

using namespace std;
using namespace std::chrono;
using namespace std::this_thread;

class SnakeGame {
public:
    const int grid_size;
    atomic<char> direction;
    atomic<bool> is_paused;
    atomic<int> score;
    pair<int,int> food;
    pair<int,int> poisonous_food;
    deque<pair<int,int>> snake;
    chrono::milliseconds sleep_duration;
    mutex io_mutex; // protect file IO / render where needed

    SnakeGame(int size = 10)
        : grid_size(size), direction('r'), is_paused(false), score(0),
          food({-1,-1}), poisonous_food({-1,-1}), sleep_duration(500)
    {
        snake.push_back(make_pair(0,0));
        rng.seed(random_device{}());
        spawn_food();
        spawn_poison();
    }

    // deterministic helper (also kept as free function wrapper below)
    static pair<int,int> getNextHead(pair<int,int> current, char dir, int grid = 10) {
        pair<int,int> next;
        if (dir == 'r') {
            next = make_pair(current.first, (current.second + 1) % grid);
        } else if (dir == 'l') {
            next = make_pair(current.first, current.second == 0 ? grid-1 : current.second - 1);
        } else if (dir == 'd') {
            next = make_pair((current.first + 1) % grid, current.second);
        } else { // 'u'
            next = make_pair(current.first == 0 ? grid-1 : current.first - 1, current.second);
        }
        return next;
    }

    void render_game() {
        // move cursor to top-left to redraw in-place
        cout << "\033[H";
        for (int i = 0; i < grid_size; ++i) {
            for (int j = 0; j < grid_size; ++j) {
                if (i == food.first && j == food.second) {
                    cout << "🍎";
                } else if (i == poisonous_food.first && j == poisonous_food.second) {
                    cout << "💀";
                } else if (find(snake.begin(), snake.end(), make_pair(i,j)) != snake.end()) {
                    cout << "🐍";
                } else {
                    cout << "⬜";
                }
            }
            cout << endl;
        }
    }

    void spawn_food() {
        uniform_int_distribution<int> dist(0, grid_size - 1);
        pair<int,int> pos;
        do {
            pos = make_pair(dist(rng), dist(rng));
        } while (find(snake.begin(), snake.end(), pos) != snake.end()
                 || pos == poisonous_food);
        food = pos;
    }

    void spawn_poison() {
        uniform_int_distribution<int> dist(0, grid_size - 1);
        pair<int,int> pos;
        do {
            pos = make_pair(dist(rng), dist(rng));
        } while (find(snake.begin(), snake.end(), pos) != snake.end()
                 || pos == food);
        poisonous_food = pos;
    }

    void update_high_scores() {
        lock_guard<mutex> lock(io_mutex);
        vector<int> high_scores;
        ifstream infile("high_scores.txt");
        int s;
        while (infile >> s) {
            high_scores.push_back(s);
        }
        infile.close();

        high_scores.push_back(score.load());
        sort(high_scores.rbegin(), high_scores.rend());

        ofstream outfile("high_scores.txt");
        for (size_t i = 0; i < high_scores.size() && i < 10; ++i) {
            outfile << high_scores[i] << endl;
        }
        outfile.close();

        cout << "--- Top 10 High Scores ---" << endl;
        for (size_t i = 0; i < high_scores.size() && i < 10; ++i) {
            cout << high_scores[i] << endl;
        }
    }

    void input_handler() {
#if defined(_WIN32) || defined(_WIN64)
    map<char, char> keymap = {{'d', 'r'}, {'a', 'l'}, {'w', 'u'}, {'s', 'd'}, {'q', 'q'}, {'p', 'p'}};
    while (true) {
        if (_kbhit()) {
            char input = _getch();
            if (keymap.find(input) != keymap.end()) {
                if (input == 'p') {
                    is_paused = !is_paused.load();
                } else if (input == 'q') {
                    exit(0);
                } else {
                    char cur = direction.load();
                    char requested = keymap[input];
                    if (!is_opposite(cur, requested)) {
                        direction = requested;
                    }
                }
            }
        }
        this_thread::sleep_for(50ms); // avoid busy loop
    }
#else
    // Linux/Mac version (your original code)
    struct termios oldt, newt;
    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);

    map<char, char> keymap = {{'d', 'r'}, {'a', 'l'}, {'w', 'u'}, {'s', 'd'}, {'q', 'q'}, {'p', 'p'}};
    while (true) {
        char input = getchar();
        if (keymap.find(input) != keymap.end()) {
            if (input == 'p') {
                is_paused = !is_paused.load();
            } else if (input == 'q') {
                tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
                exit(0);
            } else {
                char cur = direction.load();
                char requested = keymap[input];
                if (!is_opposite(cur, requested)) {
                    direction = requested;
                }
            }
        }
    }

    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
#endif
}


    void game_play() {
        system("clear");
        // ensure initial food/poison exist
        if (food.first == -1) spawn_food();
        if (poisonous_food.first == -1) spawn_poison();

        const chrono::milliseconds min_sleep(50);

        while (true) {
            // send cursor to home for redrawing
            cout << "\033[H";
            if (is_paused.load()) {
                cout << "Game is Paused! Press 'p' to resume." << endl;
                sleep_for(100ms);
                continue;
            }

            pair<int,int> head = getNextHead(snake.back(), direction.load(), grid_size);

            // check self collision
            if (find(snake.begin(), snake.end(), head) != snake.end()) {
                system("clear");
                cout << "Game Over" << endl;
                update_high_scores();
                exit(0);
            } else if (head.first == poisonous_food.first && head.second == poisonous_food.second) {
                system("clear");
                cout << "Game Over - You ate poisonous food!" << endl;
                update_high_scores();
                exit(0);
            } else if (head.first == food.first && head.second == food.second) {
                // eaten food: grow snake
                snake.push_back(head);
                // regenerate food ensuring it's not on snake or poison
                spawn_food();
                score += 10;
                if (sleep_duration.count() - 10 >= min_sleep.count()) {
                    sleep_duration = chrono::milliseconds(sleep_duration.count() - 10);
                }
            } else {
                // normal move
                snake.push_back(head);
                snake.pop_front();
            }

            render_game();
            cout << "length of snake: " << snake.size() << endl;
            cout << "Score: " << score.load() << endl;

            sleep_for(sleep_duration);
        }
    }

    static bool is_opposite(char a, char b) {
        // pairs: r-l, u-d
        if ((a == 'r' && b == 'l') || (a == 'l' && b == 'r')) return true;
        if ((a == 'u' && b == 'd') || (a == 'd' && b == 'u')) return true;
        return false;
    }

private:
    mt19937 rng;
};

//////////////////////////////////////////////////////////////////
// Backwards-compatible free functions (tests and original main.cpp)
//////////////////////////////////////////////////////////////////

static SnakeGame GLOBAL_GAME;

pair<int,int> get_next_head(pair<int,int> current, char dir) {
    return SnakeGame::getNextHead(current, dir, GLOBAL_GAME.grid_size);
}

void render_game(int size, deque<pair<int,int>> &snake, pair<int,int> food) {
    // For compatibility only: delegate to GLOBAL_GAME.render_game()
    // Note: this wrapper ignores passed parameters and uses the global game state.
    GLOBAL_GAME.render_game();
}

void input_handler() {
    GLOBAL_GAME.input_handler();
}

void game_play() {
    GLOBAL_GAME.game_play();
}

#endif // SNAKE_H
