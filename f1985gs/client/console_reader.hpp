#ifndef _CONSOLE_READER_H_
#define _CONSOLE_READER_H_

#include <iostream>
#include <string>
#include <queue>
#include <vector>
#include <thread>

class console_reader
{
public:
    struct command
    {
        std::string name;
        std::vector<std::string> params;
    };
public:
    console_reader(boost::asio::io_context& context)
        : m_thread_ptr(nullptr)
    {
    }

    void start()
    {
        m_thread_ptr.reset(new std::thread(boost::bind(&console_reader::process, this)));
    }

    bool pop_front(command& cmd)
    {
        if (m_commands.empty())
        {
            return false;
        }
        std::lock_guard<std::mutex> lock(m_mutex);
        if (m_commands.empty())
        {
            return false;
        }
        cmd = m_commands.front();
        m_commands.pop();
        return true;
    }

private:
    void process()
    {
        char cmd_str[1024];
        memset(cmd_str, 0, sizeof(cmd_str));
        while (true)
        {
            std::this_thread::sleep_for(boost::asio::chrono::milliseconds(200));
            std::cin.getline(cmd_str, sizeof(cmd_str));
            command cmd;
            parse(cmd, cmd_str);
            push_back(cmd);
        }
    }

    void push_back(command& cmd)
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_commands.push(cmd);
    }

    void parse(command& cmd, std::string cmd_ptr)
    {
        char delimiter = ' ';
        auto get_begin_pos = [delimiter](std::string& str, size_t pos)
        {
            while (pos < str.length())
            {
                if (str[pos] != delimiter)
                {
                    break;
                }
                ++pos;
            }
            return pos;
        };
        size_t pos = 0;
        size_t next_pos = 0;
        do
        {
            pos = get_begin_pos(cmd_ptr, next_pos);
            if (pos >= cmd_ptr.length())
            {
                break;
            }
            next_pos = cmd_ptr.find(delimiter, pos);
            if (next_pos == std::string::npos)
            {
                next_pos = cmd_ptr.length();
            }
            std::string str = cmd_ptr.substr(pos, next_pos - pos);
            if (cmd.name.empty())
            {
                cmd.name = str;
            }
            else
            {
                cmd.params.push_back(str);
            }
        } while (true);
    }
private:
    std::unique_ptr<std::thread> m_thread_ptr;
    std::queue<command> m_commands;
    std::mutex m_mutex;
};

#endif
