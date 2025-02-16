#pragma once

#include <btree_olc.h>
#include "workload/workload_set.h"

class CoroutineRoundRobinExecutor {
public:
    template<typename K, typename V>
    static void execute(BTree<K, V> &tree, const std::vector<NumericTuple> &workload) {
        /// Number of coroutines executed in parallel.
        const auto parallel_coroutines = std::min<std::uint64_t>(12U, workload.size());

        /// Space for lookup values.
        auto values = std::vector<V>{};
        values.resize(workload.size());

        /// Coroutines that await execution.
        auto active_coroutine_frames = std::vector<Coroutine>{};

        auto request_index = 0ULL;

        /// Store the first coroutines within the active frame.
        for (auto i = 0U; i < parallel_coroutines; ++i) {
            const auto index = request_index++;
            const auto &request = workload[index];
            if (request == NumericTuple::Type::INSERT || request == NumericTuple::Type::UPDATE) {
                active_coroutine_frames.push_back(tree.insert(request.key(), request.value()));
            } else if (request == NumericTuple::Type::LOOKUP) {
                active_coroutine_frames.push_back(tree.lookup(request.key(), values[index]));
            }
        }

        /// Dispatch coroutines until all requests are done AND all coroutines finished.
        std::uint32_t count_finished_coroutine_frames;
        do {
            count_finished_coroutine_frames = 0U;
            for (auto i = 0U; i < parallel_coroutines; ++i) {
                /// Resume this coroutine as it has not entirely executed the request.
                if (!active_coroutine_frames[i].is_done()) {
                    active_coroutine_frames[i].resume();
                }

                    /// The coroutine has completed the request. Replace by a new one, if there are pending requests.
                else {
                    const auto is_pending_requests = request_index < workload.size();
                    if (is_pending_requests) {
                        /// Free the coro frame.
                        active_coroutine_frames[i].destroy();

                        /// If the coroutine was finished, create a new one for the next request---if any.
                        const auto index = request_index++;
                        const auto &request = workload[index];
                        if (request == NumericTuple::Type::INSERT || request == NumericTuple::Type::UPDATE) {
                            active_coroutine_frames[i] =
                                    tree.insert(request.key(), request.value());
                        } else if (request == NumericTuple::Type::LOOKUP) {
                            active_coroutine_frames[i] =
                                    tree.lookup(request.key(), values[index]);
                        }
                    } else /// Otherwise, only wait to finish the last requests.
                    {
                        ++count_finished_coroutine_frames;
                    }
                }
            }
        } while (count_finished_coroutine_frames < parallel_coroutines);
    }
};