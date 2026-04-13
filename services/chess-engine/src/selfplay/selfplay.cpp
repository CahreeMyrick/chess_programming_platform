#include "selfplay/selfplay.hpp"
#include "core/uci.hpp"
#include "evaluation/eval.hpp"
#include "openings/opening_book.h"
#include <fstream>
#include <thread>
#include <mutex>
#include <random>
#include <iostream>

namespace chess {

static std::mutex file_mutex;

static Move sample_move(BitboardPosition& pos, float temperature, std::mt19937& rng) {
    MoveList moves;
    pos.generate_legal(moves);

    std::vector<float> scores(moves.size);
    for (int i = 0; i < moves.size; ++i) {
        pos.make_move(moves.moves[i]);
        scores[i] = (float)Eval::evaluate(pos);
        pos.undo_move();
    }

    if (temperature < 0.01f) {
        int best = 0;
        for (int i = 1; i < moves.size; ++i)
            if (scores[i] > scores[best]) best = i;
        return moves.moves[best];
    }

    std::vector<float> probs(moves.size);
    float sum = 0.0f;
    for (int i = 0; i < moves.size; ++i) {
        probs[i] = std::exp(scores[i] / temperature);
        sum += probs[i];
    }
    for (auto& p : probs) p /= sum;

    std::discrete_distribution<int> dist(probs.begin(), probs.end());
    return moves.moves[dist(rng)];
}


PositionBook load_book() {
    PositionBook book = loadEPD("/Users/cahree/Downloads/noob_3moves.epd");
    if (book.empty())
        std::cerr << "[book] Failed to load — will use startpos.\n";
    else
        std::cout << "[book] Loaded " << book.size() << " positions.\n";
    return book;
}

static BitboardPosition position_from_book(const PositionBook& book, std::mt19937& rng) {
    if (!book.empty()) {
        std::string fen = pickRandomPosition(book);
        if (!fen.empty()) {
            BitboardPosition pos = BitboardPosition::from_fen(fen);
            return pos;
        }
    }
    return BitboardPosition::startpos();
}

std::vector<PositionSample> play_one_game(int depth, const PositionBook& book) {
    std::mt19937 rng(std::random_device{}());
    BitboardPosition pos = position_from_book(book, rng);
    std::vector<PositionSample> samples;
    int ply = 0;

    for (; ply < 500; ++ply) {
        if (pos.is_draw_by_fifty()) {
            std::cout << "[DRAW] fifty move rule at ply " << ply << "\n";
            for (auto& s : samples) s.outcome = 0.5f;
            return samples;
        }
        if (pos.is_draw_by_repetition()) {
            std::cout << "[DRAW] repetition at ply " << ply << "\n";
            for (auto& s : samples) s.outcome = 0.5f;
            return samples;
        }

        MoveList moves;
        pos.generate_legal(moves);

        if (moves.size == 0) {
            if (pos.in_check(pos.side_to_move())) {
                float outcome = (pos.side_to_move() == Color::White) ? 0.0f : 1.0f;
                std::cout << "[MATE] "
                          << (pos.side_to_move() == Color::White ? "Black wins" : "White wins")
                          << " at ply " << ply << "\n";
                for (auto& s : samples) s.outcome = outcome;
                return samples;
            } else {
                std::cout << "[DRAW] stalemate at ply " << ply << "\n";
                for (auto& s : samples) s.outcome = 0.5f;
                return samples;
            }
        }

        SearchResult result = SearchBB::minimax(pos, depth);

        // Log suspiciously high scores that suggest mate was found
        if (std::abs(result.score) > 50000)
            std::cout << "[SCORE] ply=" << ply
                      << " stm=" << (pos.side_to_move() == Color::White ? "W" : "B")
                      << " score=" << result.score
                      << " move=" << move_to_uci(result.best) << "\n";

        Move chosen = (ply < 8)
            ? sample_move(pos, 1.0f, rng)
            : result.best;

        PositionSample sample;
        sample.fen      = pos.to_fen();
        sample.move_uci = move_to_uci(chosen);
        sample.score    = result.score;
        sample.outcome  = -1.0f;
        samples.push_back(sample);

        pos.make_move(chosen);
    }

    std::cout << "[DRAW] hit 500 ply limit\n";
    for (auto& s : samples) s.outcome = 0.5f;
    return samples;
}

/*
std::vector<PositionSample> play_one_game(int depth, const PositionBook& book) {
    std::mt19937 rng(std::random_device{}());
    BitboardPosition pos = position_from_book(book, rng);
    std::vector<PositionSample> samples;

    for (int ply = 0; ply < 500; ++ply) {
        if (pos.is_draw_by_fifty() || pos.is_draw_by_repetition()) {
            for (auto& s : samples) s.outcome = 0.5f;
            return samples;
        }

        MoveList moves;
        pos.generate_legal(moves);

        if (moves.size == 0) {
            float outcome;
            if (pos.in_check(pos.side_to_move()))
                outcome = (pos.side_to_move() == Color::White) ? 0.0f : 1.0f;
            else
                outcome = 0.5f;
            for (auto& s : samples) s.outcome = outcome;
            return samples;
        }

        // Temperature: still use noise for the first few plies after the
        // book position, then go greedy. Since we're already past the opening
        // the window is smaller than before (8 plies instead of 20).
        float temp = (ply < 8) ? 1.0f : 0.0f;
        Move chosen = sample_move(pos, temp, rng);

        // Full search to get the score for this position
        SearchResult result = SearchBB::minimax(pos, depth);

        PositionSample sample;
        sample.fen      = pos.to_fen();
        sample.move_uci = move_to_uci(chosen);
        sample.score    = result.score;
        sample.outcome  = -1.0f;
        samples.push_back(sample);

        pos.make_move(chosen);
    }

    for (auto& s : samples) s.outcome = 0.5f;
    return samples;
}
    */

static void write_samples(const std::vector<PositionSample>& samples,
                           std::ofstream& file) {
    for (const auto& s : samples)
        file << s.fen << ","
             << s.move_uci << ","
             << s.score << ","
             << s.outcome << "\n";
}

static void worker(int num_games, int depth,
                   const PositionBook& book, std::ofstream& out) {
    for (int i = 0; i < num_games; ++i) {
        auto samples = play_one_game(depth, book);
        std::lock_guard<std::mutex> lock(file_mutex);
        write_samples(samples, out);
        std::cout << "Game done: " << samples.size() << " positions\n";
    }
}

void run_selfplay(int num_games, int depth, const std::string& output_path) {
    PositionBook book = load_book();

    std::ofstream out(output_path, std::ios::app);
    if (out.tellp() == 0)
        out << "fen,move,score,outcome\n";

    int num_threads = std::thread::hardware_concurrency();
    int games_per_thread = num_games / num_threads;

    std::vector<std::thread> threads;
    for (int t = 0; t < num_threads; ++t)
        threads.emplace_back(worker, games_per_thread, depth,
                             std::cref(book), std::ref(out));
    for (auto& t : threads) t.join();
}

} // namespace chess