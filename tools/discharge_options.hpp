#pragma once
#include "../src/eval/discharge.hpp"
#include <charconv>
#include <cmath>
#include <stdexcept>

namespace trafficsim {
// CLI-only syntax; scientific window/rank validation remains in eval.
struct DischargeOptions {
    bool enabled{}, configured{};
    DischargeSpec spec;
    std::optional<double> end;
    bool consume(const std::string& arg,int& i,int argc,char** argv) {
        if(arg=="--discharge") {enabled=true;return true;}
        if(arg!="--discharge-start"&&arg!="--discharge-end"&&arg!="--discharge-warmup"&&
           arg!="--discharge-steady-first"&&arg!="--discharge-steady-last"&&
           arg!="--discharge-startup-last"&&arg!="--discharge-type")return false;
        configured=true;
        if(++i==argc)throw std::invalid_argument("Missing value for "+arg);
        const std::string value=argv[i];
        if(arg=="--discharge-type") {
            if(value.empty()||value.starts_with("--"))throw std::invalid_argument("--discharge-type needs a type ID");
            spec.vehicleTypeIds.insert(value);return true;
        }
        if(arg=="--discharge-start"||arg=="--discharge-end"||arg=="--discharge-warmup") {
            std::size_t used=0;double seconds=std::stod(value,&used);
            if(used!=value.size()||!std::isfinite(seconds)||seconds<0)
                throw std::invalid_argument(arg+" needs finite nonnegative seconds");
            if(arg=="--discharge-start")spec.windowStart=seconds;
            else if(arg=="--discharge-end")end=seconds;
            else spec.warmup=seconds;
        } else {
            std::size_t rank=0;
            const auto parsed=std::from_chars(value.data(),value.data()+value.size(),rank);
            if(parsed.ec!=std::errc{}||parsed.ptr!=value.data()+value.size()||rank==0)
                throw std::invalid_argument(arg+" needs a positive integer rank");
            if(arg=="--discharge-steady-first")spec.steadyFirst=rank;
            else if(arg=="--discharge-steady-last")spec.steadyLast=rank;
            else spec.startupLast=rank;
        }
        return true;
    }
    void validateUsage(bool project) const {
        if(configured&&!enabled)throw std::invalid_argument("Discharge options need --discharge");
        if(enabled&&!project)throw std::invalid_argument("--discharge needs --project");
    }
    DischargeSpec forDuration(double duration) const {
        auto result=spec;result.windowEnd=end.value_or(duration);
        validateDischargeSpec(result);
        if(result.windowEnd>duration)throw std::invalid_argument("Discharge window exceeds run duration");
        return result;
    }
};
}
