/*
File: include/benchmarking.hpp

This file is part of the Zee partitioning framework

Copyright (C) 2015 Jan-Willem Buurlage <janwillembuurlage@gmail.com>

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU Lesser General Public License (LGPL)
as published by the Free Software Foundation, either version 3 of the
License, or (at your option) any later version.
*/

#pragma once

#include <chrono>
#include <iomanip>
#include <sstream>
#include <utility>
#include <vector>

#include "jw.hpp"

namespace Zee {

class Benchmark {
   public:
    using TTimePoint =
        std::chrono::time_point<std::chrono::high_resolution_clock>;

    Benchmark(std::string title) : title_(title) {
        start_ = std::chrono::high_resolution_clock::now();
    }

    ~Benchmark() {
        if (!silent_ && !finished_) {
            finish();
        }
    }

    void phase(std::string splitTitle) {
        splitTitle.resize(30, ' ');
        auto now = std::chrono::high_resolution_clock::now();
        splits_.push_back(make_pair(splitTitle, now));
    }

    void silence() { silent_ = true; }

    void finish() {
        if (silent_) return;

        finished_ = true;

        auto end = std::chrono::high_resolution_clock::now();
        auto total_ms =
            std::chrono::duration<double, std::milli>(end - start_).count();

        std::stringstream splitOutput;
        if (!splits_.empty()) {
            splits_.push_back(make_pair("", end));
            auto hline =
                "----------------------------------------------------------";
            splitOutput << "\n" << hline << '\n';
            for (unsigned int i = 0; i < splits_.size() - 1; ++i) {
                auto splitTime = splits_[i + 1].second - splits_[i].second;
                auto ms = std::chrono::duration<double, std::milli>(splitTime)
                              .count();
                splitOutput << std::fixed << std::setprecision(2)
                            << splits_[i].first << " \t" << ms << " ms"
                            << " \t" << (ms / total_ms) * 100 << "%" << endl;
            }
            splitOutput << hline;
        }

        JWLogBenchmark << title_ << " total runtime: " << total_ms << " ms"
                       << splitOutput.str() << endLog;
    }

    auto getSplits() const { return splits_; }
    auto getStart() const { return start_; }

   private:
    std::vector<std::pair<std::string, TTimePoint>> splits_;
    std::string title_;
    bool silent_ = false;
    bool finished_ = false;
    TTimePoint start_;
};

}  // namespace Zee
