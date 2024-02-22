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
#include <utility>

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

class TheTowerOfHanoiGame
{
public:
    using engine_type = TheTowerOfHanoi;
    enum class command_type
    {
        nop,
        move,
        undo,
        quit
    };
    struct parse_result_type
    {
        bool ok;
        command_type type;
        std::string_view from;
        std::string_view to;
    };

    static parse_result_type parse(std::string_view input)
    {
        parse_result_type result{ .ok = false, .type = command_type::nop };

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

    struct last_op_type
    {
        command_type command;
        std::string from;
        std::string to;
    };

public:
    TheTowerOfHanoiGame()
            : m_engine{}
    {
    }

    explicit TheTowerOfHanoiGame(engine_type::mapped_type::size_type initial)
            : m_engine{}
    {
        auto&& [it, ok] {
                m_engine.create("a", [initial](const TheTowerOfHanoi::container_type::iterator& iterator) -> bool
                {
                    for (auto i{ initial }; i > 0; --i)
                    {
                        if (auto pushed{ iterator->second.push(i) }; !pushed)
                        {
                            return false;
                        }
                    }
                    return true;
                }) };

        m_engine.create("b");
        m_engine.create("c");
    }

    explicit TheTowerOfHanoiGame(engine_type engine)
            : m_engine(std::move(engine))
    {
    }

    void run()
    {
        m_running = true;
        while (m_running)
        {
            std::cout << "\033[2J\033[1;1H";

            std::cout << m_engine << '\n';

            std::getline(std::cin, m_input, '\n');

            if (std::cin.fail())
            {
                std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
                std::cin.clear();
                continue;
            }

            auto result{ parse(m_input) };

            if (result.ok)
            {
                switch (result.type)
                {
                    case command_type::quit:
                        m_running = false;
                        break;
                    case command_type::move:
                        if (m_engine.has(result.from) && m_engine.has(result.to))
                        {
                            if (m_engine.move(result.from, result.to))
                            {
                                m_lastOp.command = command_type::move;
                                m_lastOp.from = result.from;
                                m_lastOp.to = result.to;
                            }
                        }
                        break;
                    case command_type::undo:
                        if (m_lastOp.command == command_type::move)
                        {
                            if (m_engine.move(m_lastOp.to, m_lastOp.from))
                            {
                                m_lastOp.command = command_type::move;
                                std::swap(m_lastOp.from, m_lastOp.to);
                            }
                        }
                        break;
                    default:
                        break;
                }
            }
        }
    }

private:
    engine_type m_engine;
    bool m_running{ false };
    std::string m_input{};
    last_op_type m_lastOp{ .command = command_type::nop };
};

int main()
{
    TheTowerOfHanoiGame game{ 9 };

    game.run();

    return 0;
}
