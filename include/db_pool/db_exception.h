#pragma once
#include <string>
#include <stdexcept>

namespace zhttp::zdb
{
    class DBException final : public std::runtime_error
    {
    public:
        explicit DBException(const std::string& message) : std::runtime_error(message) {}

        explicit DBException(const char* message):  std::runtime_error(message){}
    };
} // namespace zhttp::zdb