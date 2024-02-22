#include <cstdint>
#include <concepts>
#include <functional>
#include <iostream>
#include <limits>
#include <ranges>
#include <stack>
#include <string>
#include <string_view>
#include <unordered_map>

template<std::totally_ordered T, typename Sequence = std::stack<T>::container_type>
class HanoiTower
{
public:
    using adapter_type = std::stack<T, Sequence>;
    using container_type = adapter_type::container_type;
    using value_type = adapter_type::value_type;
    using size_type = adapter_type::size_type;
    using reference = adapter_type::reference;
    using const_reference = adapter_type::const_reference;

public:
    HanoiTower()
            : m_stack{}
    {
    }

    explicit HanoiTower(const adapter_type& stack)
            : m_stack{ stack }
    {
    }

    [[nodiscard]] const_reference top() const
    {
        return m_stack.top();
    }

    [[nodiscard]] bool empty() const
    {
        return m_stack.empty();
    }

    [[nodiscard]] size_type size() const
    {
        return m_stack.size();
    }

    [[nodiscard]] bool placeable(const_reference element) const
    {
        return empty() || top() > element;
    }

    bool push(const_reference element)
    {
        if (placeable(element))
        {
            m_stack.push(element);
            return true;
        }
        return false;
    }

    template<typename... Args>
    bool emplace(Args&& ...args)
    {
        auto&& element{ value_type{ std::forward<Args>(args)... }};
        if (placeable(element))
        {
            m_stack.push(element);
            return true;
        }
        return false;
    }

    void pop()
    {
        m_stack.pop();
    }

    [[nodiscard]] const adapter_type& adapter() const
    {
        return m_stack;
    }

private:
    adapter_type m_stack;
};

template<std::totally_ordered T, typename Sequence = std::stack<T>::container_type>
std::ostream& operator<<(std::ostream& os, const HanoiTower<T, Sequence>& adapter)
{
    struct StackHack : protected HanoiTower<T, Sequence>::adapter_type
    {
        static const Sequence& container(const HanoiTower<T, Sequence>::adapter_type& base)
        {
            return static_cast<const StackHack&>(base).c;
        }
    };
    const auto& container{ StackHack::container(adapter.adapter()) };

    for (auto&& e: container)
    {
        os << e;
    }

    return os;
}

class TheTowerOfHanoi
{
public:
    using container_type = std::unordered_map<std::string_view, HanoiTower<std::uint_fast32_t>>;
    using key_type = container_type::key_type;
    using mapped_type = container_type::mapped_type;
    struct create_result_type
    {
        container_type::iterator iterator;
        bool ok{ false };
    };

public:
    explicit TheTowerOfHanoi()
            : m_towerMap{}
    {
    }

    [[nodiscard]] bool has(key_type name) const
    {
        return m_towerMap.contains(name);
    }

    create_result_type create(key_type name)
    {
        auto&& [iter, ok]{ m_towerMap.insert({ name, {}}) };
        return { .iterator = iter, .ok = ok };
    }

    create_result_type create(key_type name, const std::function<bool(container_type::iterator)>& onSuccess)
    {
        auto&& result{ create(name) };
        if (result.ok)
        {
            result.ok = onSuccess(result.iterator);
        }
        return result;
    }

    mapped_type& select(key_type name)
    {
        return m_towerMap.at(name);
    }

    const mapped_type& select(key_type name) const
    {
        return m_towerMap.at(name);
    }

    bool move(key_type fromName, key_type toName)
    {
        if (fromName == toName)
        {
            return true;
        }

        auto& from{ select(fromName) };
        auto& to{ select(toName) };
        if (from.empty())
        {
            return false;
        }

        if (to.push(from.top()))
        {
            from.pop();
            return true;
        }

        return false;
    }

    friend std::ostream& operator<<(std::ostream& os, const TheTowerOfHanoi& theTowerOfHanoi);

private:
    container_type m_towerMap;
};

std::ostream& operator<<(std::ostream& os, const TheTowerOfHanoi& theTowerOfHanoi)
{
    for (auto&& [key, value]: theTowerOfHanoi.m_towerMap)
    {
        os << key << '#' << value << '\n';
    }
    return os;
}

enum class command_type
{
    nop,
    move,
    undo,
    quit
};

struct parse_result_type
{
    bool ok{ false };
    command_type type{ command_type::nop };
    std::string_view from{};
    std::string_view to{};
};

parse_result_type parse(std::string_view input)
{
    parse_result_type result{};

    if (input.starts_with('/'))
    {
        result.ok = true;

        auto command{ input.substr(1) };

        if (command == "quit")
        {
            result.type = command_type::quit;
        }
        else if (command == "undo")
        {
            result.type = command_type::undo;
        }
    }
    else if (auto pos{ input.find(',') }; pos != std::string_view::npos)
    {
        result.ok = true;
        result.type = command_type::move;
        result.from = input.substr(0, pos);
        result.to = input.substr(pos + 1);
    }

    return result;
}

int main()
{
    TheTowerOfHanoi hanoi{};

    auto&& [it, ok] { hanoi.create("a", [](const TheTowerOfHanoi::container_type::iterator& iterator) -> bool
    {
        for (TheTowerOfHanoi::mapped_type::size_type i{ 9 }; i > 0; --i)
        {
            if (auto pushed{ iterator->second.push(i) }; !pushed)
            {
                return false;
            }
        }
        return true;
    }) };

    hanoi.create("b");
    hanoi.create("c");

    bool running{ true };

    std::string input{};

    struct last_op_type
    {
        command_type command;
        std::string from;
        std::string to;
    } lastOp{ .command = command_type::nop };

    while (running)
    {
        std::cout << "\033[2J\033[1;1H";

        std::cout << hanoi << '\n';

        std::getline(std::cin, input, '\n');

        if (std::cin.fail())
        {
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            std::cin.clear();
            continue;
        }

        auto result{ parse(input) };

        if (result.ok)
        {
            switch (result.type)
            {
                case command_type::quit:
                    running = false;
                    break;
                case command_type::move:
                    if (hanoi.has(result.from) && hanoi.has(result.to))
                    {
                        if (hanoi.move(result.from, result.to))
                        {
                            lastOp.command = command_type::move;
                            lastOp.from = result.from;
                            lastOp.to = result.to;
                        }
                    }
                    break;
                case command_type::undo:
                    if (lastOp.command == command_type::move)
                    {
                        if (hanoi.move(lastOp.to, lastOp.from))
                        {
                            lastOp.command = command_type::move;
                            std::swap(lastOp.from, lastOp.to);
                        }
                    }
                    break;
                default:
                    break;
            }
        }
    }


//    hanoi.move("a", "c"); //
//
//    std::cout << hanoi << '\n';
//
//    hanoi.move("c", "b");
//    hanoi.move("a", "c"); //
//    hanoi.move("b", "c");
//
//    std::cout << hanoi << '\n';
//
//    hanoi.move("c", "a");
//    hanoi.move("c", "b");
//    hanoi.move("a", "b");
//    hanoi.move("a", "c"); //
//    hanoi.move("b", "a");
//    hanoi.move("b", "c");
//    hanoi.move("a", "c");
//
//    std::cout << hanoi << '\n';
//
//    hanoi.move("c", "a");
//    hanoi.move("c", "b");
//    hanoi.move("a", "c");
//    hanoi.move("b", "a");
//    hanoi.move("c", "a");
//    hanoi.move("c", "b");
//    hanoi.move("a", "c");
//    hanoi.move("a", "b");
//    hanoi.move("c", "b");
//    hanoi.move("a", "c"); //
//    hanoi.move("b", "c");
//    hanoi.move("b", "a");
//    hanoi.move("c", "a");
//    hanoi.move("b", "c");
//    hanoi.move("a", "c");
//    hanoi.move("a", "b");
//    hanoi.move("c", "a");
//    hanoi.move("b", "c");
//    hanoi.move("a", "c");
//
//    std::cout << hanoi << '\n';

    return 0;
}
