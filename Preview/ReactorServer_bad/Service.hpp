#pragma once
#include "Protocol.hpp"
#include <functional>


using service_t = std::function<Response (const Request &req)>;

// 业务逻辑
static Response calculator(const Request &req)
{
    Response resp = {0, 0};
    switch (req.op)
    {
    case '+':
        resp.result = req.x + req.y;
        break;
    case '-':
        resp.result = req.x - req.y;
        break;
    case '*':
        resp.result = req.x * req.y;
        break;
    case '/':
    { // x_ / y_
        if (req.y == 0)
            resp.code = -1; // -1. 除0
        else
            resp.result = req.x / req.y;
    }
    break;
    case '%':
    { // x_ / y_
        if (req.y == 0)
            resp.code = -2; // -2. 模0
        else
            resp.result = req.x % req.y;
    }
    break;
    default:
        resp.code = -3; // -3: 非法操作符
        break;
    }

    return resp;
}
