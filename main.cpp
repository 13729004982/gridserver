#include "server.h"

int main() 
{
    Server server("121.36.3.126", "root", "Wxcrrdtd20021106!", "electricity_data");
    server.initAndRun();
    return 0;
}