/**
 * @mainpage Документация проекта A-Wordle Solver
 *
 * @section description_sec Описание
 * Консольное приложение для автоматического решения головоломки Wordle 
 * с использованием теоретико-информационных эвристик.
 *
 * @author Шелонин Арсений (Б01-411)
 * @date 20.02.2026
 * @version 1.0
 */

/**
 * @file main.cpp
 * @brief Реализация решателя головоломки Wordle.

 * * Содержит класс WordleSolver, который реализует алгоритм поиска загаданного слова
 */

#include <cstdio>
#include <vector>
#include <string>
#include <cstdint>
#include <cassert>
#include <algorithm>
#include <stdint.h>
#include <cmath>

constexpr size_t MAX_WORD_LEN = 10;

/**
 * @class WordleSolver
 * @brief Класс - автоматическтй решатель игры Wordle.

 * * Класс предварительно вычисляет двумерную матрицу паттернов @ref pattern_matrix_ для всех возможных пар слов.
 * На каждом ходу он оценивает количество энтропии для каждого слова из отобранных, и посылает его на сервер.
 */
class WordleSolver {

private:
    const uint16_t total_words_;                                    ///< Общее количество доступных слов в словаре.
    const uint16_t rounds_;                                         ///< Количество раундов, которые нужно сыграть.
    const uint16_t word_length_;                                    ///< Длина каждого слова.
    const uint16_t possible_answers_count_;                         ///< Количество первых слов словаря, которые могут быть ответом (по условию ограничено 2100).

    uint16_t best_first_guess_idx_;                                 ///< Кэшированный индекс лучшего первого слова для ускорения последующих раундов.

    /**
     * @brief Состояния совпадения отдельной буквы.
     */
    enum LetterHitT {
        MISS      = 0,                                              ///< Буква отсутствует в загаданном слове.
        EXISTANCE = 1,                                              ///< Буква есть в загаданном слове, но на другой позиции.
        POSITION  = 2                                               ///< Буква находится на правильной позиции.
    };

    static const char MISS_SYM      = '-';                          ///< Символ ответа сервера: буквы нет.
    static const char EXISTANCE_SYM = '?';                          ///< Символ ответа сервера: буква есть, позиция неверна.
    static const char POSITION_SYM  = '#';                          ///< Символ ответа сервера: точное совпадение.

    /**
     * @brief Тип для хранения числового представления паттерна.
     * * Паттерн кодируется как число в троичной системе счисления.
     * Для слова из 5 букв максимальное значение: 3^5 - 1 = 242.
     * Значения разрядов: 0 - MISS, 1 - EXISTANCE, 2 - POSITION.
     */
    using pattern_t = uint8_t;

    std::vector<std::string> words_;                                ///< Словарь всех допустимых слов.

    /**
     * @brief Предвычисленная матрица паттернов @ref pattern_t.

     * * Матрица [@ref total_words_][@ref possible_answers_count_]
     * Для каждой пары слов содержит их паттерн (паттерн, который мы бы получили для засланного серверу слова из словаря (идут по вертикали), если бы загаданным было бы слово из списка возможных (по горизонтали))
     */
    std::vector<std::vector<pattern_t>> pattern_matrix_;

    std::vector<uint16_t> rem_indices_;                             ///< Буфер индексов слов, которые еще могут быть ответом.
    std::vector<uint16_t> next_rem_;                                ///< Вспомогательный буффер для отбора слов для следующего шага.

    /**
     * @brief Вычисляет паттерн совпадения слова guess со словом secret.
     * @param[in] guess Слово-догадка.
     * @param[in] secret Загаданное (целевое) слово.
     * @return Паттерн ответа в виде числа (троичная система).
     * @note Сложность O(L), L - максимальная длина слова (@ref word_length_).
     */
    pattern_t CalculatePattern_ (const std::string& guess, const std::string& secret) const;

    /**
     * @brief Преобразует строковый ответ сервера в числовой паттерн.
     * @param[in] server_ans Строка ответа (состоит из символов @ref MISS_SYM, @ref EXISTANCE_SYM, @ref POSITION_SYM).
     * @return Паттерн ответа в виде числа (троичная система).
     */
    pattern_t ParseServerPattern_(const std::string& server_ans) const;

    /**
     * @brief Предварительно вычисляет матрицу паттернов для всех пар слов.
     * @note Сложность O(N * M * L), где N - общее число слов, M - число возможных ответов, L - длина слова.
     */
    void PrecomputeMatrix_();

    /**
     * @brief Ищет оптимальное слово на основе минимизации суммы квадратов размеров баскетов.
     * * Оценивает каждое слово по тому, насколько равномерно оно разбивает оставшиеся 
     * возможные ответы по паттернам-баскетам.
     * @return Индекс оптимального слова в массиве `words_`.
     */
    int FindBestGuess_AverageSq_() const;

    /**
     * @brief Ищет оптимальное слово на основе максимизации информационной энтропии.
     * * Альтернативная эвристика. Попытка честно считать энтропию. На практике не выигрывает по точности выбора оптимального слова с учетом сложности его расчёта.
     * @return Индекс оптимального слова в массиве `words_`.
     */
    int FindBestGuess_Entropy_() const;

public:
    /**
     * @brief Конструктор инициализации решателя.
     * @param[in] n Общее количество слов в словаре.
     * @param[in] m Количество раундов.
     * @param[in] l Длина одного слова.
     */
    WordleSolver(int n, int m, int l) 
        : total_words_(n)
        , rounds_(m)
        , word_length_(l)
        , possible_answers_count_(std::min(total_words_, (uint16_t) 2100))
        , best_first_guess_idx_(-1)
    {
        assert(word_length_ <= 5);
        words_.resize(total_words_);
        
        // резервирование памяти ровно 1 раз для избежания реаллокаций в цикле
        rem_indices_.reserve(possible_answers_count_);
        next_rem_   .reserve(possible_answers_count_);
    }

    /**
     * @brief Считывает словарь слов из стандартного потока ввода (stdin).
     */
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

    /**
     * @brief Запускает основной цикл решения для всех раундов.
     * * Метод взаимодействует с сервером (или тестирующей системой) через stdin/stdout.
     */
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

    // Перевод массива цифр в троичное число
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

    // is_possible[k] = может ли быть слово с индексом k (среди возможных ответов) ответом или точно нет
    std::vector<bool> is_possible(possible_answers_count_, false);
    
    for (uint16_t j : rem_indices_)
    {
        is_possible[j] = true;
    }

    for (uint16_t i = 0; i < possible_answers_count_; ++i)
    {
        // 243 паттерна для каждого слова. Для каждого слова считаем, на какие группы оно бьет оставшиеся слова по паттернам
        int counts[243] = {0};
        const std::vector<pattern_t>& row = pattern_matrix_[i];
        
        // распределение оставшихся возможных ответов по баскетам (паттернам)
        for (uint16_t j : rem_indices_)
        {
            counts[row[j]]++;
        }

        // Будем минимизировать стоимость cost, тк при её минимизации получается оценочно наиболее "ровное" распределение слов по баскетам,
        // что дает наиболее информативно.
        long long cost = 0;
        
        for (int c = 0; c < 243; ++c)
        {
            if (counts[c] > 0)
            {
                cost += 1LL * counts[c] * counts[c];
            }
        }

        // эвристика: бонус для слов, которые сами могут быть ответом.
        // По сути означает что между двумя словами, одинаково разбивающими оставшиеся слова по баскетам, выбираем то, которое может быть ответом.
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
    // eдиножды предвычисляем матрицу всех возможных паттернов
    PrecomputeMatrix_();

    const std::string win_string(word_length_, POSITION_SYM);
    char resp_buf[MAX_WORD_LEN];

    for (uint16_t round = 0; round < rounds_; ++round)
    {
        // Сброс буфера возможных ответов для нового раунда
        rem_indices_.resize(possible_answers_count_);
        for (uint16_t j = 0; j < possible_answers_count_; ++j)
        {
            rem_indices_[j] = j;
        }
        
        for (int attempt = 0; attempt < 6; ++attempt)
        {
            int current_best_idx = -1;
            
            // оптимальное первое слово всегда одно и то же
            // мы вычисляем его только в первой игре и кэшируем.

            // первой попыткой отправляем уже найденное (если нашли) лучшее слово
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

            // отправка догадки на сервер
            printf("%s\n", words_[current_best_idx].c_str());
            fflush(stdout);

            if (scanf("%9s", resp_buf) != 1)
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

            // фильтрация оставшихся вариантов
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

/**
 * @brief Точка входа в программу.
 * * Читает входные параметры, инициализирует класс WordleSolver и запускает игровой цикл.
 * @return 0 при успешном завершении.
 */
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