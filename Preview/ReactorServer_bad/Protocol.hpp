#pragma once
#include <iostream>
#include <vector>
#include <cstring>
#include <string>
#include <cstdio>

#define SEP 'X'
#define SEP_LEN sizeof(SEP)

#define CRLF "\r\n"
#define CRLF_LEN strlen(CRLF) 
#define SPACE " "
#define SPACE_LEN strlen(SPACE)

// 分离独立报文
void PackageSplit(std::string &inbuffer, std::vector<std::string> *result)
{
    while (true)
    {
        std::size_t pos = inbuffer.find(SEP);
        if (pos == std::string::npos)
            break;
        result->push_back(inbuffer.substr(0, pos));
        inbuffer.erase(0, pos + SEP_LEN);
    }
}

struct Request
{
    int x;
    int y;
    char op;
};

struct Response
{
    int code;
    int result;
};

// 解析请求
bool Parser(std::string &in, Request *req)
{
    // 1 + 1, 2 * 4, 5 * 9, 6 *1
    std::size_t spaceOne = in.find(SPACE);
    if (std::string::npos == spaceOne)
        return false;
    std::size_t spaceTwo = in.rfind(SPACE);
    if (std::string::npos == spaceTwo)
        return false;

    std::string dataOne = in.substr(0, spaceOne);
    std::string dataTwo = in.substr(spaceTwo + SPACE_LEN);
    std::string oper = in.substr(spaceOne + SPACE_LEN, spaceTwo - (spaceOne + SPACE_LEN));
    if (oper.size() != 1)
        return false;

    // 转成内部成员
    req->x = atoi(dataOne.c_str());
    req->y = atoi(dataTwo.c_str());
    req->op = oper[0];

    return true;
}

// 序列化响应
void Serialize(const Response &resp, std::string *out)
{
    std::string ec = std::to_string(resp.code);
    std::string res = std::to_string(resp.result);

    *out = ec;
    *out += SPACE;
    *out += res;
    *out += CRLF;
}
