#include "server.h"

int main() 
{
    Server server("localhost", "root", "password", "test");
    server.initAndRun();
    return 0;
}