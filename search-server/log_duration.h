#pragma once

#include <chrono>
#include <iostream>

// Объединяем в уникальное имя переменной
// Для этого протаскиваем __LINE__ 2 раза как параметр
#define PROFILE_CONCAT_INTERNAL(X, Y) X ## Y
#define PROFILE_CONCAT(X, Y) PROFILE_CONCAT_INTERNAL(X, Y)
#define UNIQUE_VAR_NAME_PROFILE PROFILE_CONCAT(profile_guard_, __LINE__)

// Вызов конструктора с уникальным именем
#define LOG_DURATION(x) LogDuration UNIQUE_VAR_NAME_PROFILE(x)

// Вызов конструктора с указанием потока вывода
#define LOG_DURATION_STREAM(x, s) LogDuration UNIQUE_VAR_NAME_PROFILE(x, s)

class LogDuration {
public:
    // заменим имя типа std::chrono::steady_clock
    // с помощью using для удобства
    using Clock = std::chrono::steady_clock;

    LogDuration(const std::string& name, std::ostream& os = std::cerr)
        : duration_name_(name)
        , os_(os)
    {}

    ~LogDuration() {
        using namespace std::chrono;
        using namespace std::literals;

        const auto end_time = Clock::now();
        const auto dur = end_time - start_time_;
        os_ << duration_name_ << ": "s 
            << duration_cast<milliseconds>(dur).count() 
            << " ms"s << std::endl;
    }

private:
    const Clock::time_point start_time_ = Clock::now();
    std::string duration_name_; // Имя измеряемого участка
    std::ostream& os_; // Поток вывода
};
