#include <memory>
#include "TcpServer.hpp"
#include "Service.hpp"

using namespace std;
static void usage(std::string process)
{
    cerr << "\nUsage: " << process << " port\n"
         << endl;
}
int BeginHandler(Connection *conn, std::string &message, service_t service)
{
    // message一定是一个完整的报文，因为我们已经对它进行了解码
    Request req;
    // 反序列化，进行处理的问题
    if (!Parser(message, &req))
    {
        // 写回错误消息
        return -1;
    }
    // 业务逻辑
    Response resp = service(req);

    std::cout << req.x << " " << req.op << " " << req.y << std::endl;
    std::cout << resp.code << " " << resp.result << std::endl;

    // 序列化
    std::string sendstr;
    Serialize(resp, &sendstr);

    // 处理完毕的结果，发送回给client
    conn->outbuffer_ += sendstr;
    conn->sender_(conn);
    if(conn->outbuffer_.empty()) conn->R_->EnableReadWrite(conn->sock_, true, false);
    else conn->R_->EnableReadWrite(conn->sock_, true, true);

    std::cout << "--- end ---" << std::endl;
    return 0;
}


// 1 + 1X2 + 3X5 + 6X
int HandlerRequest(Connection *conn, std::string &message)
{
    // beginhandler里面是具体的调用逻辑，calculator是本次事务处理函数
    return BeginHandler(conn, message, calculator);
}
//./test 8080
int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        usage(argv[0]);
        exit(0);
    }
    TcpServer svr(HandlerRequest, atoi(argv[1]));
    svr.Run();

    return 0;
}
