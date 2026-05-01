#include "selfplay/selfplay.hpp"
#include "core/uci.hpp"
#include "evaluation/eval.hpp"
#include "openings/opening_book.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <random>
#include <string>
#include <thread>
#include <vector>

namespace chess {

// =========================
// SELF-PLAY CONFIG
// =========================

static constexpr int MAX_PLIES = 300;

// Use book positions for this percentage of games.
// 0.70 = 70% book-start games, 30% normal startpos games.
static constexpr double BOOK_START_PROBABILITY = 0.70;

// Early random exploration.
static constexpr int RANDOM_PLIES = 8;
static constexpr float RANDOM_TEMPERATURE = 1.0f;

// Opening book path.
static const std::string BOOK_PATH = "/Users/cahree/dev/chess_training_platform/services/chess-engine/src/openings/popularpos_lichess_v3.epd";


// =========================
// MOVE SAMPLING
// =========================

static Move sample_move(BitboardPosition& pos, float temperature, std::mt19937& rng) {
    MoveList moves;
    pos.generate_legal(moves);

    std::vector<float> scores(moves.size);

    for (int i = 0; i < moves.size; ++i) {
        pos.make_move(moves.moves[i]);

        int eval = Eval::evaluate(pos);

        // If your Eval is positive for White, then White wants max and Black wants min.
        // After make_move(), side_to_move has changed, so evaluate from the original mover's perspective.
        scores[i] = static_cast<float>(
            pos.side_to_move() == Color::Black ? eval : -eval
        );

        pos.undo_move();
    }

    if (temperature < 0.01f) {
        int best = 0;
        for (int i = 1; i < moves.size; ++i) {
            if (scores[i] > scores[best]) {
                best = i;
            }
        }
        return moves.moves[best];
    }

    std::vector<float> probs(moves.size);
    float max_score = *std::max_element(scores.begin(), scores.end());

    float sum = 0.0f;
    for (int i = 0; i < moves.size; ++i) {
        probs[i] = std::exp((scores[i] - max_score) / temperature);
        sum += probs[i];
    }

    for (auto& p : probs) {
        p /= sum;
    }

    std::discrete_distribution<int> dist(probs.begin(), probs.end());
    return moves.moves[dist(rng)];
}


// =========================
// OPENING BOOK
// =========================

PositionBook load_book() {
    PositionBook book = loadEPD(BOOK_PATH);

    if (book.empty()) {
        std::cerr << "[book] Failed to load book. Will use startpos only.\n";
    } else {
        std::cout << "[book] Loaded " << book.size() << " book positions.\n";
    }

    return book;
}

static BitboardPosition choose_start_position(
    const PositionBook& book,
    std::mt19937& rng
) {
    std::bernoulli_distribution use_book_dist(BOOK_START_PROBABILITY);

    bool use_book = !book.empty() && use_book_dist(rng);

    if (use_book) {
        std::string fen = pickRandomPosition(book);

        if (!fen.empty()) {
            return BitboardPosition::from_fen(fen);
        }
    }

    return BitboardPosition::startpos();
}


// =========================
// PLAY ONE GAME
// =========================

std::vector<PositionSample> play_one_game(int depth, const PositionBook& book) {
    std::mt19937 rng(std::random_device{}());

    BitboardPosition pos = choose_start_position(book, rng);

    std::vector<PositionSample> samples;
    samples.reserve(120);

    for (int ply = 0; ply < MAX_PLIES; ++ply) {
        if (pos.is_draw_by_fifty() || pos.is_draw_by_repetition()) {
            for (auto& s : samples) {
                s.outcome = 0.5f;
            }
            return samples;
        }

        MoveList moves;
        pos.generate_legal(moves);

        if (moves.size == 0) {
            float outcome = 0.5f;

            if (pos.in_check(pos.side_to_move())) {
                outcome = (pos.side_to_move() == Color::White) ? 0.0f : 1.0f;
            }

            for (auto& s : samples) {
                s.outcome = outcome;
            }

            return samples;
        }

        SearchResult result = SearchBB::minimax(pos, depth);

        Move chosen = (ply < RANDOM_PLIES)
            ? sample_move(pos, RANDOM_TEMPERATURE, rng)
            : result.best;

        PositionSample sample;
        sample.fen = pos.to_fen();
        sample.move_uci = move_to_uci(chosen);
        sample.score = result.score;
        sample.outcome = -1.0f;

        samples.push_back(sample);

        pos.make_move(chosen);
    }

    for (auto& s : samples) {
        s.outcome = 0.5f;
    }

    return samples;
}


// =========================
// WRITE DATA
// =========================

static void write_samples(const std::vector<PositionSample>& samples, std::ofstream& file) {
    for (const auto& s : samples) {
        file << s.fen << ","
             << s.move_uci << ","
             << s.score << ","
             << s.outcome << "\n";
    }
}

static void worker_file(
    int thread_id,
    int start_game,
    int end_game,
    int depth,
    const PositionBook& book,
    const std::string& output_path
) {
    std::string part_path = output_path + ".part" + std::to_string(thread_id);

    std::ofstream out(part_path);
    if (!out) {
        std::cerr << "[thread " << thread_id << "] failed to open " << part_path << "\n";
        return;
    }

    out << "fen,move,my_engine_score,outcome\n";

    for (int game = start_game; game < end_game; ++game) {
        auto samples = play_one_game(depth, book);
        write_samples(samples, out);

        if ((game - start_game + 1) % 10 == 0) {
            std::cout << "[thread " << thread_id << "] completed "
                      << (game - start_game + 1)
                      << " games\n";
        }
    }
}


// =========================
// RUN SELF-PLAY
// =========================

void run_selfplay(int num_games, int depth, const std::string& output_path) {
    PositionBook book = load_book();

    unsigned hw = std::thread::hardware_concurrency();
    int num_threads = hw == 0 ? 4 : static_cast<int>(hw);

    num_threads = std::min(num_threads, num_games);
    num_threads = std::max(1, num_threads - 2);
    num_threads = std::min(num_threads, 8);

    std::cout << "[selfplay] Using " << num_threads << " worker threads\n";

    std::vector<std::thread> threads;

    int base = num_games / num_threads;
    int extra = num_games % num_threads;

    int start = 0;

    for (int t = 0; t < num_threads; ++t) {
        int count = base + (t < extra ? 1 : 0);
        int end = start + count;

        threads.emplace_back(
            worker_file,
            t,
            start,
            end,
            depth,
            std::cref(book),
            std::cref(output_path)
        );

        start = end;
    }

    for (auto& t : threads) {
        t.join();
    }

    std::ofstream final_out(output_path);
    final_out << "fen,move,my_engine_score,outcome\n";

    for (int t = 0; t < num_threads; ++t) {
        std::string part_path = output_path + ".part" + std::to_string(t);
        std::ifstream part(part_path);

        std::string line;
        bool first_line = true;

        while (std::getline(part, line)) {
            if (first_line) {
                first_line = false;
                continue;
            }

            if (!line.empty()) {
                final_out << line << "\n";
            }
        }

        std::remove(part_path.c_str());
    }

    std::cout << "[selfplay] Finished writing " << output_path << "\n";
}

} // namespace chess