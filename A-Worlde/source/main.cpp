#include <cstdio>
#include <vector>
#include <string>
#include <cstdint>
#include <cassert>
#include <algorithm>
#include <stdint.h>
#include <cmath>

class WordleSolver {

private:

    const uint16_t total_words_;
    const uint16_t rounds_;
    const uint16_t word_length_;
    const uint16_t possible_answers_count_;

          uint16_t best_first_guess_idx_;

    enum LetterHitT {
        MISS = 0,       // буквы нет
        EXISTANCE,      // угадали существование
        POSITION        // угадали позицию
    };

    static const char MISS_SYM      = '-';
    static const char EXISTANCE_SYM = '?';
    static const char POSITION_SYM  = '#';

    // троичная система: 0..242)
    // 0 - 'MISS', 1 - 'EXISTANCE', 2 - 'POSITION'
    using pattern_t = uint8_t;


    std::vector<std::string> words_;
    std::vector<std::vector<pattern_t>> pattern_matrix_;

    // 2 буфера для работы без аллокаций памяти в игровом цикле
    std::vector<uint16_t> rem_indices_;
    std::vector<uint16_t> next_rem_;


    pattern_t CalculatePattern_ (const std::string& guess, const std::string& secret) const;

    // преобразование ответа сервера в числовой паттерн
    pattern_t ParseServerPattern_(const std::string& server_ans) const;

    void PrecomputeMatrix_();

    // поиск оптимального слова на основе минимизации суммы квадратов размеров корзин
    int FindBestGuess_AverageSq_() const;
    int FindBestGuess_Entropy_  () const;

public:
    WordleSolver(int n, int m, int l) 
        : total_words_(n)
        , rounds_(m)
        , word_length_(l)
        , possible_answers_count_(std::min(total_words_, (uint16_t) 2100))
        , best_first_guess_idx_(-1)
           
    {
        assert(word_length_ <= 5);
        words_.resize(total_words_);
        
        // Резервирование памяти ровно 1 раз
        rem_indices_.reserve(possible_answers_count_);
        next_rem_   .reserve(possible_answers_count_);
    }

    void ReadWords()
    {
        char buf[16];
        for (uint16_t i = 0; i < total_words_; ++i)
        {
            if (scanf("%15s", buf) == 1) {
                words_[i] = buf;
            }
        }
    }

    void Run();
};



WordleSolver::pattern_t WordleSolver::CalculatePattern_ (const std::string& guess, const std::string& secret) const
{
    pattern_t pattern = 0;
    int secret_counts[26] = {};
    
    for (uint16_t i = 0; i < word_length_; ++i)
    {
        secret_counts[secret[i] - 'a']++;
    }

    int pattern_digits[5] = {}; 

    // поиск точных совпадений
    for (uint16_t i = 0; i < word_length_; ++i)
    {
        if (guess[i] == secret[i]) {
            pattern_digits[i] = POSITION;
            secret_counts[guess[i] - 'a']--;
        }
    }

    // поиск неточных совпадений
    for (uint16_t i = 0; i < word_length_; ++i)
    {
        if (pattern_digits[i] == 0 && secret_counts[guess[i] - 'a'] > 0)
        {
            pattern_digits[i] = EXISTANCE;
            secret_counts[guess[i] - 'a']--;
        }
    }

    // digits -> number
    int mult = 1;

    for (uint16_t i = 0; i < word_length_; ++i)
    {
        pattern += pattern_digits[i] * mult;
        mult *= 3;
    }
    
    return pattern;
}


WordleSolver::pattern_t WordleSolver::ParseServerPattern_(const std::string& server_ans) const
{
    pattern_t pattern = 0;

    int mult = 1;

    for (uint16_t i = 0; i < word_length_; ++i)
    {
        LetterHitT val = MISS;

        switch (server_ans[i])
        {
        case POSITION_SYM:  val = POSITION;     break;
        case EXISTANCE_SYM: val = EXISTANCE;    break;
        case MISS_SYM:      val = MISS;         break;

        default:            
            fprintf(stderr, "invalid server_ans[i], i = %u\n", i);
        }
        
        pattern += val * mult;
        mult *= 3;
    }

    return pattern;
}


void WordleSolver::PrecomputeMatrix_()
{
    pattern_matrix_.assign(total_words_, std::vector<pattern_t>(possible_answers_count_));

    for (uint16_t i = 0; i < total_words_; ++i)
    {
        for (uint16_t j = 0; j < possible_answers_count_; ++j)
        {
            pattern_matrix_[i][j] = CalculatePattern_(words_[i], words_[j]);
        }
    }
}


int WordleSolver::FindBestGuess_AverageSq_() const
{
    if (rem_indices_.size() <= 2)
    {
        return rem_indices_[0];
    }

    int       best_idx = -1;
    long long min_cost = -1;

    std::vector<bool> is_possible(possible_answers_count_, false);
    
    for (uint16_t j : rem_indices_)
    {
        is_possible[j] = true;
    }

    for (uint16_t i = 0; i < possible_answers_count_; ++i)
    {
        int counts[243] = {0};
        const std::vector<pattern_t>& row = pattern_matrix_[i];
        
        // распределение оставшихся возможных ответов по корзинам (паттернам)
        for (uint16_t j : rem_indices_)
        {
            counts[row[j]]++;
        }

        long long cost = 0;
        
        for (int c = 0; c < 243; ++c)
        {
            if (counts[c] > 0)
            {
                cost += 1LL * counts[c] * counts[c];
            }
        }

        // эвристика
        long long adjusted_cost = cost - ((is_possible[i]) ? 1 : 0);

        if (min_cost == -1 || adjusted_cost < min_cost)
        {
            min_cost = adjusted_cost;
            best_idx = i;
        }
    }

    return best_idx;
}


int WordleSolver::FindBestGuess_Entropy_() const
{
    if (rem_indices_.size() <= 2)
    {
        return rem_indices_[0];
    }

    int    best_idx    = -1;
    double max_entropy = -1.0;

    int remaining_count = rem_indices_.size();

    std::vector<bool> is_possible(possible_answers_count_, false);
    for (uint16_t j : rem_indices_)
    {
        is_possible[j] = true;
    }

    for (uint16_t i = 0; i < total_words_; ++i)
    {
        int counts[243] = {0};
        const std::vector<pattern_t>& row = pattern_matrix_[i];
        
        for (uint16_t j : rem_indices_)
        {
            counts[row[j]]++;
        }

        double expected_information = 0.0;

        for (int c = 0; c < 243; ++c)
        {
            if (counts[c] > 0)
            {
                // p(x) - вероятность получить паттерн c
                double p_x = static_cast<double>(counts[c]) / remaining_count;
                
                // E[I] = - p(x) * log2(p(x))
                expected_information -= p_x * std::log2(p_x);
            }
        }

        bool is_possible_answer = (i < possible_answers_count_ && is_possible[i]);
        
        const double EPSILON = 1e-9;
        
        if (expected_information > max_entropy + EPSILON)
        {
            max_entropy = expected_information;
            best_idx = i;
        }

        else if (std::abs(expected_information - max_entropy) <= EPSILON)
        {
             bool best_is_possible = (best_idx != -1 && best_idx < possible_answers_count_ && is_possible[best_idx]);

             if (is_possible_answer && !best_is_possible)
             {
                 max_entropy = expected_information;
                 best_idx = i;
             }
        }
    }

    return best_idx;
}


void WordleSolver::Run()
{
    PrecomputeMatrix_();

    const std::string win_string(word_length_, POSITION_SYM);
    char resp_buf[16];

    for (uint16_t round = 0; round < rounds_; ++round)
    {
        rem_indices_.resize(possible_answers_count_);
        for (uint16_t j = 0; j < possible_answers_count_; ++j)
        {
            rem_indices_[j] = j;
        }
        
        for (int attempt = 0; attempt < 6; ++attempt)
        {
            int current_best_idx = -1;
            
            if (attempt == 0 && best_first_guess_idx_ != uint16_t(-1))
            {
                current_best_idx = best_first_guess_idx_;
            }

            else
            {
                current_best_idx = FindBestGuess_AverageSq_();
                if (attempt == 0)
                {
                    best_first_guess_idx_ = current_best_idx;
                }
            }

            // Отправка догадки на сервер
            printf("%s\n", words_[current_best_idx].c_str());
            fflush(stdout);

            if (scanf("%15s", resp_buf) != 1)
            {
                break;
            }

            std::string response(resp_buf);

            if (response == win_string)
            {
                break;
            }

            uint8_t target_pattern = ParseServerPattern_(response);
            
            next_rem_.clear();
            
            const std::vector<pattern_t>& row = pattern_matrix_[current_best_idx];

            for (uint16_t j : rem_indices_)
            {
                if (row[j] == target_pattern)
                {
                    next_rem_.push_back(j);
                }
            }

            rem_indices_.swap(next_rem_);
        }
    }
}


//--------------------------------------------------------------------------------------------------------------------------------
int main()
{
    int n, m, l;
    if (scanf("%d %d %d", &n, &m, &l) == 3) {
        WordleSolver solver(n, m, l);
        solver.ReadWords();
        solver.Run();
    }

    return 0;
}