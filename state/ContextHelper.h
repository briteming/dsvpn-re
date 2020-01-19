#pragma once

#include <string>
#include "Context.h"
#include <boost/shared_ptr.hpp>
#include "../utils/IOWorker.h"

class ContextHelper {
public:
    static boost::shared_ptr<Context> CreateContext(context_detail& detail) {
        auto context = boost::make_shared<Context>(IOWorker::GetInstance()->GetRandomContext());
        context->detail = detail;
        return context;
    }
};