#include <iostream>
#include <string>

#include "Config.hpp"
#include "Network.hpp"

using namespace std;
using namespace sf;


map<double, Socket* > SocketList;
map<double, Socket* >::iterator SocketListIt;
typedef pair<double, Socket* > SocketListPair;
pair<map<double, Socket* >::iterator, bool> SocketListRet;
double SocketIncrement = 0;

map<double, Packet* > PacketList;
map<double, Packet* >::iterator PacketIt;
typedef pair<double, Packet* > PacketListPair;
pair<map<double, Packet* >::iterator, bool> PacketListRet;
double PacketIncrement = 0;

double insertSock(Socket* sock)
{
    SocketList.insert(SocketListPair(++SocketIncrement, sock));

    return SocketIncrement;
}

double insertPacket(Packet* sock)
{
    PacketList.insert(PacketListPair(++PacketIncrement, sock));

    return PacketIncrement;
}

Socket::Status last_error;

double net_tcp_connect(string ip, unsigned short port, bool blocking);
double net_tcp_connect_timeout(string ip, unsigned short port, bool blocking, float seconds);
double net_tcp_listen(unsigned short port, bool blocking);
double net_tcp_accept(double listener_id, bool blocking);
double net_tcp_connected(double tcpsocketId);
double net_tcp_send(double tcpsocketId, double packetId);

string net_last_error();

double net_packet_create();
double net_packet_destroy(double packetId);
template <class T> double net_packet_write(double packetId, T value);
template <class T> T net_packet_read(double packetId, T &var);
double net_packet_clear(double packetId);

double net_tcp_connect(string ip, unsigned short port, bool blocking)
{
    return net_tcp_connect_timeout(ip, port, blocking, 0.0f);
}

double net_tcp_connect_timeout(string ip, unsigned short port, bool blocking, float seconds)
{
    TcpSocket* sock = new TcpSocket();
    sock->SetBlocking(blocking);

    last_error = sock->Connect(IpAddress(ip), port, seconds);

    if(last_error == Socket::Disconnected || last_error == Socket::Error)
    {
        delete sock;
        return 0;
    }
    return insertSock(sock);
}

double net_tcp_listen(unsigned short port)
{
    TcpListener* sock = new TcpListener();

    last_error = sock->Listen(port);

    if(last_error != Socket::Done)
    {
        delete sock;
        return 0;
    }
    return insertSock(sock);
}

//Accepts an incoming connection
//Waits until it receives a connection if blocking
//Returns the newly created socket
//Returns 0 if non-blocking and no connections are available
double net_tcp_accept(double listener_id, bool blocking)
{
    if(!SocketList.count(listener_id)){
        last_error = Socket::InvalidId;
        return 0;
    }

    TcpSocket* client = new TcpSocket();
    do
    {
        TcpListener* listener = dynamic_cast<TcpListener* >(SocketList[listener_id]);
        listener->SetBlocking(blocking);

        if(listener->Accept(*client) == Socket::Done)
        {
            last_error = Socket::Done;
            return insertSock(client);
        }
    }
    while(blocking);

    delete client;
    last_error = Socket::NotReady;
    return 0;
}

double net_tcp_connected(double tcpsocketId)
{
    if(!SocketList.count(tcpsocketId)){
        last_error = Socket::InvalidId;
        return 0;
    }

    TcpSocket* sock = dynamic_cast<TcpSocket* >(SocketList[tcpsocketId]);
    if(sock->GetRemoteAddress() != 0)
    {
        return 1;
    }
    return 0;
}

double net_tcp_send(double tcpsocketId, double packetId)
{
    if(!SocketList.count(tcpsocketId)){
        last_error = Socket::InvalidId;
        return 0;
    }

    if(!PacketList.count(packetId))
    {
        last_error = Socket::InvalidId;
        return 0;
    }
    TcpSocket* sock = dynamic_cast<TcpSocket* >(SocketList[tcpsocketId]);

    last_error = sock->Send( *(PacketList[packetId]) );
    if(last_error != Socket::Done)
    {
        return 0;
    }
    return 1;
}

double net_packet_create()
{
    Packet* pack = new Packet();
    return insertPacket(pack);
}

template <class T>
double net_packet_write(double packetId, T value)
{
    if(!PacketList.count(packetId)){ return 0; }
    *(PacketList[packetId]) << value;
    return 1;
}

template <class T>
T net_packet_read(double packetId, T &var)
{
    if(!PacketList.count(packetId)){ return 0; }
    *(PacketList[packetId]) >> var;
    return var;
}

double net_packet_destroy(double packetId)
{
    if(PacketList.count(packetId))
    {
        delete PacketList[packetId];
        PacketList.erase(packetId);
        return 1;
    }
    return 0;

}

double net_packet_clear(double packetId)
{
    if(!PacketList.count(packetId)){ return 0; }
    PacketList[packetId]->Clear();
    return 1;
}

string net_last_error()
{
    string error = "";
    switch(last_error)
    {
        case Socket::Done:
            error = "Done";
            break;
        case Socket::NotReady:
            error = "NotReady";
            break;
        case Socket::Disconnected:
            error = "Disconnected";
            break;
        case Socket::Error:
            error = "Error";
            break;
        case Socket::InvalidId:
            error = "InvalidId";
            break;
    }
    return error;
}

int main()
{
    double packetId;
    double sockId;

    sockId = net_tcp_listen(1080);
    if(!sockId){
        cout << net_last_error() << endl;
    }else{
        sockId = net_tcp_accept(sockId, true);
        if(!sockId)
        {
            cout << net_last_error() << endl;
        }
        else
        {
            cout << net_tcp_connected(sockId) << endl;
            packetId = net_packet_create();

            net_packet_write(packetId, 1337);
            net_packet_clear(packetId);

            net_packet_write(packetId, 65535);
            net_packet_write(packetId, (short) 1);
            net_packet_write(packetId, (short) 0);

            int a; int b;

            net_packet_read(packetId, a);
            net_packet_read(packetId, b);

            //65535 65536
            cout << a << " " << b << endl;

            net_packet_clear(packetId);
            net_packet_write(packetId, "1\n");
            net_packet_write(packetId, "2\n");
            net_packet_write(packetId, "3\n");
            cout << net_tcp_send(sockId, packetId) << endl;
            Sleep(3);
            net_packet_write(packetId, "4\n");
            cout << net_tcp_send(sockId, packetId) << endl;
            Sleep(3);
            net_packet_clear(packetId);
            net_packet_write(packetId, "5\n");
            cout << net_tcp_send(sockId, packetId) << endl;

            net_packet_destroy(packetId);
        }
    }

    return sockId;
}

