#pragma once

#include <chrono>
#include <stdexcept>
#include <string>
#include <thread>
#include <utility>
#include <vector>

#include "app/alerts.h"
#include "app/exit_req.h"
#include "app/transfer_state.h"
#include "etc/logger.h"

namespace wfz::daemon
{
    class Cancelled : public std::runtime_error
    {
    public:
        Cancelled() : std::runtime_error("Operation cancelled") {}
    };

    template <typename Fn>
    auto WithRetry(const std::string &label, const std::vector<std::string> &urls, Fn &&job)
        -> decltype(job(urls.front()))
    {
        if (urls.empty())
            throw std::invalid_argument("No URLs configured");

        std::string last_error;
        for (int attempt = 1; attempt <= 3; ++attempt)
        {
            if (wfz::app::ExitRequested())
                throw Cancelled();

            const std::string &url = urls[(attempt - 1) % urls.size()];
            wfz::transfer_state::Start(label, url, attempt, 3);
            wfz::logger::Info("[%s] attempt %d/3: %s", label.c_str(), attempt, url.c_str());
            try
            {
                auto value = job(url);
                wfz::transfer_state::SetPhase(wfz::transfer_state::Phase::Completed);
                return value;
            }
            catch (const Cancelled &)
            {
                throw;
            }
            catch (const std::exception &e)
            {
                if (wfz::app::ExitRequested())
                    throw Cancelled();

                last_error = e.what();
                wfz::transfer_state::Fail(last_error);
                wfz::logger::Warning("[%s] failed %d/3: %s (%s)", label.c_str(), attempt, last_error.c_str(), url.c_str());
            }
            if (attempt < 3)
            {
                for (int t = 0; t < attempt * 10; ++t)
                {
                    if (wfz::app::ExitRequested())
                        throw Cancelled();

                    std::this_thread::sleep_for(std::chrono::milliseconds(100));
                }
            }
        }
        wfz::alerts::Post({"НЕ УДАЛОСЬ ПОЛУЧИТЬ ДАННЫЕ",
                           label + ": три неудачные попытки. Проверь соединение или доступ к источнику.",
                           last_error + "\nПоследний адрес: " + urls[(3 - 1) % urls.size()],
                           "ПОВТОРИТЬ",
                           wfz::alerts::Action::RetryAll});
        throw std::runtime_error(label + ": " + last_error);
    }
}
