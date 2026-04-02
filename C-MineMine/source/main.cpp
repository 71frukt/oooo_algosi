#include <cmath>
#include <iostream>
#include <vector>

//---------------------------------------------------------------------------------------------------------
template <typename T, typename Compare = std::less<T>>
class PriorityQueue
{
private:
    std::vector<T> data_;
    Compare comp_;

    void sift_up(std::size_t i)
    {
        while (i > 0)
        {
            std::size_t parent = (i - 1) / 2;

            if (comp_(data_[parent], data_[i]))
            {
                std::swap(data_[parent], data_[i]);
                i = parent;
            }
            else
            {
                break;
            }
        }
    }

    void sift_down(std::size_t i)
    {
        std::size_t size = data_.size();
        while (true)
        {
            std::size_t left = 2 * i + 1;
            std::size_t right = 2 * i + 2;
            std::size_t best = i;

            if (left < size && comp_(data_[best], data_[left]))
            {
                best = left;
            }
            if (right < size && comp_(data_[best], data_[right]))
            {
                best = right;
            }

            if (best != i)
            {
                std::swap(data_[i], data_[best]);
                i = best;
            }
            else
            {
                break;
            }
        }
    }

public:
    PriorityQueue() = default;
    explicit PriorityQueue(const Compare& comp) : comp_(comp) {}

    bool empty() const
    {
        return data_.empty();
    }

    std::size_t size() const
    {
        return data_.size();
    }

    const T& top() const
    {
        return data_.front();
    }

    void push(const T& val)
    {
        data_.push_back(val);
        sift_up(data_.size() - 1);
    }

    void push(T&& val)
    {
        data_.push_back(std::move(val));
        sift_up(data_.size() - 1);
    }

    void pop()
    {
        if (data_.empty()) return;

        if (data_.size() == 1)
        {
            data_.pop_back();
            return;
        }

        data_.front() = std::move(data_.back());
        data_.pop_back();
        
        sift_down(0);
    }
};

//---------------------------------------------------------------------------------------------------------
template <typename Key, typename Hash = std::hash<Key>, typename KeyEqual = std::equal_to<Key>>
class HashSet
{
private:
    struct Bucket
    {
        Key key;
        bool occupied = false;
    };

    std::vector<Bucket> table_;
    std::size_t size_;
    Hash hasher_;
    KeyEqual equals_;

    void rehash()
    {
        std::vector<Bucket> old_table = std::move(table_);
        table_.assign(old_table.size() * 2, Bucket{});
        size_ = 0;

        for (auto& bucket : old_table)
        {
            if (bucket.occupied)
            {
                insert(std::move(bucket.key));
            }
        }
    }

public:
    explicit HashSet(std::size_t initial_capacity = 100003) : size_(0)
    {
        table_.resize(initial_capacity);
    }

    bool contains(const Key& key) const
    {
        if (table_.empty()) return false;

        std::size_t idx = hasher_(key) % table_.size();
        std::size_t start_idx = idx;

        while (table_[idx].occupied)
        {
            if (equals_(table_[idx].key, key)) return true;

            idx = (idx + 1) % table_.size();
            if (idx == start_idx) break;
        }
        return false;
    }

    void insert(const Key& key)
    {
        if (size_ * 2 >= table_.size())
        {
            rehash();
        }

        std::size_t idx = hasher_(key) % table_.size();

        while (table_[idx].occupied)
        {
            if (equals_(table_[idx].key, key)) return;
            idx = (idx + 1) % table_.size();
        }

        table_[idx].key = key;
        table_[idx].occupied = true;
        size_++;
    }

    void insert(Key&& key)
    {
        if (size_ * 2 >= table_.size())
        {
            rehash();
        }

        std::size_t idx = hasher_(key) % table_.size();

        while (table_[idx].occupied)
        {
            if (equals_(table_[idx].key, key)) return;
            idx = (idx + 1) % table_.size();
        }

        table_[idx].key = std::move(key);
        table_[idx].occupied = true;
        size_++;
    }
};

//---------------------------------------------------------------------------------------------------------
template <typename Key, typename Value, typename Hash = std::hash<Key>, typename KeyEqual = std::equal_to<Key>>
class HashMap
{
private:
    struct Bucket
    {
        Key key;
        Value value;
        bool occupied = false;
    };

    std::vector<Bucket> table_;
    std::size_t size_;
    Hash hasher_;
    KeyEqual equals_;

    void rehash()
    {
        std::vector<Bucket> old_table = std::move(table_);
        table_.assign(old_table.size() * 2, Bucket{});
        size_ = 0;

        for (auto& bucket : old_table)
        {
            if (bucket.occupied)
            {
                insert(std::move(bucket.key), std::move(bucket.value));
            }
        }
    }

public:
    explicit HashMap(std::size_t initial_capacity = 100003) : size_(0)
    {
        table_.resize(initial_capacity);
    }

    bool contains(const Key& key) const
    {
        if (table_.empty()) return false;

        std::size_t idx = hasher_(key) % table_.size();
        std::size_t start_idx = idx;

        while (table_[idx].occupied)
        {
            if (equals_(table_[idx].key, key)) return true;

            idx = (idx + 1) % table_.size();
            if (idx == start_idx) break;
        }
        return false;
    }

    void insert(const Key& key, const Value& value)
    {
        if (size_ * 2 >= table_.size())
        {
            rehash();
        }

        std::size_t idx = hasher_(key) % table_.size();

        while (table_[idx].occupied)
        {
            if (equals_(table_[idx].key, key)) 
            {
                table_[idx].value = value;
                return;
            }
            idx = (idx + 1) % table_.size();
        }

        table_[idx].key = key;
        table_[idx].value = value;
        table_[idx].occupied = true;
        size_++;
    }

    Value get(const Key& key) const
    {
        if (table_.empty()) return Value{};

        std::size_t idx = hasher_(key) % table_.size();
        std::size_t start_idx = idx;

        while (table_[idx].occupied)
        {
            if (equals_(table_[idx].key, key)) return table_[idx].value;

            idx = (idx + 1) % table_.size();
            if (idx == start_idx) break;
        }
        return Value{};
    }
};

struct Point
{
    long long x;
    long long y;

    bool operator==(const Point& other) const
    {
        return x == other.x && y == other.y;
    }
};

struct PointHash
{
    // std::size_t operator()(const Point& p) const
    // {
    //     return std::hash<long long>()(p.x) ^ (std::hash<long long>()(p.y) << 1);
    // }

    std::size_t operator()(const Point& p) const
    {
        std::size_t h1 = static_cast<std::size_t>(p.x) * 73856093;
        std::size_t h2 = static_cast<std::size_t>(p.y) * 19349663;
        return h1 ^ h2;
    }

};

struct Node
{
    Point point;
    long long strength;
    double priority;

    bool operator>(const Node& other) const
    {
        return priority > other.priority;
    }
};

//---------------------------------------------------------------------------------------------------------
class MinemineSolver
{
public:
    MinemineSolver(long long target_x, long long target_y, long long pickaxe_strength)
        : target_{target_x, target_y}
        , cur_pickaxe_strength_(pickaxe_strength)
    {}

    void initialize_start_state()
    {
        visited_.insert({0, 0});

        long long strength_right;
        long long strength_left;
        long long strength_up;
        long long strength_down;

        if (!(std::cin >> strength_right >> strength_left >> strength_up >> strength_down))
        {
            std::exit(0);
        }

        Point p_up    = {0,  1};
        Point p_left  = {0, -1};
        Point p_right = {1,  0};
        Point p_down  = {-1, 0};

        add_to_frontier_(p_up, strength_up);
        add_to_frontier_(p_left, strength_left);
        add_to_frontier_(p_right, strength_right);
        add_to_frontier_(p_down, strength_down);
    }

    void solve()
    {
        while (!frontier_.empty())
        {
            Node curr = frontier_.top();
            frontier_.pop();

            if (visited_.contains(curr.point))
                continue;

            if (curr.strength > cur_pickaxe_strength_)
                continue;

            std::cout << curr.point.x << " " << curr.point.y << std::endl;

            visited_.insert(curr.point);

            spent_ += curr.strength;
            broken_count_++;
            cur_pickaxe_strength_ -= curr.strength;

            if (curr.point == target_)
                break;

            process_neighbors_(curr.point);
        }
    }

private:

    long long stat_n_ = 0;
    double stat_mean_ = 0.0;
    double stat_M2_ = 0.0;

    void update_statistics_(long long strength)
    {
        stat_n_++;
        double delta = strength - stat_mean_;
        stat_mean_ += delta / stat_n_;
        double delta2 = strength - stat_mean_;
        stat_M2_ += delta * delta2;
    }

    double get_stddev_() const
    {
        if (stat_n_ < 2) return 1.0; // Защита от деления на ноль на старте
        return std::sqrt(stat_M2_ / stat_n_);
    }

    Point target_;
    long long cur_pickaxe_strength_;

    PriorityQueue<Node, std::greater<Node>> frontier_;
    HashSet<Point, PointHash> visited_;

    long long spent_ = 0;
    long long broken_count_ = 0;

    static constexpr double W = 1.5;      
    static constexpr double GAMMA = 0.1;  // Весовой коэффициент страха стен

    long long calculate_manhattan_(const Point& a, const Point& b) const
    {
        return std::abs(a.x - b.x) + std::abs(a.y - b.y);
    }

    // double calculate_priority_(const Point& p, long long strength) const
    // {

    //     // W * static_cast<double>(calculate_manhattan_(p, target_)) 
    //     // std::pow(calculate_manhattan_(p, target_), W)
    //     return static_cast<double>(strength) 
    //          + std::pow(calculate_manhattan_(p, target_), W);
    // }

    double calculate_priority_(const Point& p, long long strength) const
    {
        double manhattan = static_cast<double>(calculate_manhattan_(p, target_));
        double adjusted_strength = static_cast<double>(strength);

        // Применяем Z-оценку только после того, как собрана минимальная выборка (например, 20 блоков)
        if (stat_n_ > 20)
        {
            double stddev = get_stddev_();
            double z_score = (adjusted_strength - stat_mean_) / stddev;

            // Если блок аномально прочный (Z > 1.5, примерно 93-й перцентиль)
            if (z_score > 1.5)
            {
                adjusted_strength += std::pow(z_score, 3.0) * 100.0; 
            }

            else if (z_score < -1.0)
            {
                adjusted_strength *= 0.5;
            }
        }

        // Сохраняем квадратичный вектор направления к базе
        return adjusted_strength + (manhattan * manhattan * W);
    }

    void add_to_frontier_(const Point& p, long long strength)
    {
        if (visited_.contains(p) || strength > cur_pickaxe_strength_)
            return;

        frontier_.push({p, strength, calculate_priority_(p, strength)});
    }

   void process_neighbors_(const Point& curr)
    {
        long long strength_up, strength_down, strength_right, strength_left;
        if (!(std::cin >> strength_up >> strength_down >> strength_right >> strength_left))
            std::exit(0);

        // Обновляем статистику распределения карты до добавления в очередь
        update_statistics_(strength_up);
        update_statistics_(strength_down);
        update_statistics_(strength_right);
        update_statistics_(strength_left);

        add_to_frontier_({curr.x,     curr.y + 1}, strength_up);
        add_to_frontier_({curr.x,     curr.y - 1}, strength_down);
        add_to_frontier_({curr.x + 1, curr.y    }, strength_right);
        add_to_frontier_({curr.x - 1, curr.y    }, strength_left);
    }
};

//---------------------------------------------------------------------------------------------------------

int main()
{
    long long target_x, target_y, N;
    if (!(std::cin >> target_x >> target_y >> N))
    {
        return 0;
    }

    MinemineSolver solver(target_x, target_y, N);
    solver.initialize_start_state();
    solver.solve();

    return 0;
}