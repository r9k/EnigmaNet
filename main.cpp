#include <iostream>
#include <string>

#include "Config.hpp"
#include "Network.hpp"

using namespace std;
using namespace sf;

Socket::Status last_error;

map<double, Socket* > SocketList;
typedef pair<double, Socket* > SocketListPair;
double SocketIncrement = 0;

map<double, Packet* > PacketList;
typedef pair<double, Packet* > PacketListPair;
double PacketIncrement = 0;

map<double, Http* > HttpList;
typedef pair<double, Http* > HttpListPair;
double HttpIncrement = 0;

map<double, Http::Request* > HttpRequestList;
typedef pair<double, Http::Request* > HttpRequestListPair;
double HttpRequestIncrement = 0;

map<double, Http::Response > HttpResponseList;
typedef pair<double, Http::Response > HttpResponseListPair;
double HttpResponseIncrement = 0;

#include "rc4.hpp"

map<double, net_utility_rc4 > Rc4List;
typedef pair<double, net_utility_rc4 > Rc4ListPair;
double Rc4Increment = 0;

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

double insertHttp(Http* sock)
{
    HttpList.insert(HttpListPair(++HttpIncrement, sock));

    return HttpIncrement;
}

double insertHttpRequest(Http::Request* sock)
{
    HttpRequestList.insert(HttpRequestListPair(++HttpRequestIncrement, sock));

    return HttpRequestIncrement;
}

double insertHttpResponse(Http::Response res)
{
    HttpResponseList.insert(HttpResponseListPair(++HttpResponseIncrement, res));

    return HttpResponseIncrement;
}

double insertRc4(net_utility_rc4 rc4)
{
    Rc4List.insert(Rc4ListPair(++Rc4Increment, rc4));

    return Rc4Increment;
}

double net_tcp_connect(string ip, unsigned short port, bool blocking);
double net_tcp_connect_timeout(string ip, unsigned short port, bool blocking, float seconds);
bool net_tcp_set_blocking(double tcpsocketId, bool blocking);
double net_tcp_listen(unsigned short port, bool blocking);
double net_tcp_accept(double listener_id, bool blocking);
bool net_tcp_connected(double tcpsocketId);
bool net_tcp_disconnect(double tcpsocketId);
bool net_tcp_send(double tcpsocketId, double packetId);
bool net_tcp_receive(double tcpsocketId, double packetId);
bool net_tcp_destroy(double tcpsocketId);


bool net_udp_bind(unsigned short port, bool blocking);
bool net_udp_bind_available(bool blocking);
bool net_udp_unbind(double udpsocketId);
unsigned short net_udp_localport(double udpsocketId);
bool net_udp_set_blocking(double udpsocketId, bool blocking);
bool net_udp_send(double udpsocketId, double packetId, string ipAddress, unsigned short remotePort);
bool net_udp_receive(double udpsocketId, double packetId, string &ipAddress, unsigned short &remotePort);
bool net_udp_destroy(double udpsocketId);

double net_http_create();
bool net_http_host(double httpId, string host);
double net_http_send(double httpId, double httpRequestId);
bool net_http_destroy(double httpId);
double net_http_request_create();
bool net_http_request_method(double httpRequestId, string method);
bool net_http_request_uri(double httpRequestId, string uri);
bool net_http_request_field(double httpRequestId, string name, string value);
bool net_http_request_body(double httpRequestId, string body);
bool net_http_request_destroy(double httpRequestId);
double net_http_response_status(double httpResponseId);
string net_http_response_body(double httpResponseId);
bool net_http_response_destroy(double httpResponseId);

string net_last_error();

double net_packet_create();
bool net_packet_destroy(double packetId);
template <class T> bool net_packet_write(double packetId, T value);
template <class T> bool net_packet_read(double packetId, T &var);
bool net_packet_clear(double packetId);
double net_packet_size(double packetId);

double net_utility_rc4_create(string keyString);
bool net_utility_rc4_step(double rc4Id);
bool net_utility_rc4_step_add(double rc4Id, double step);
bool net_utility_rc4_step_set(double rc4Id, double step);
bool net_utility_rc4_xor(double rc4Id, double packetId);
char net_utility_rc4_byte(double rc4Id);
bool net_utility_rc4_destroy(double rc4Id);

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
        last_error = Socket::InvalidSocketId;
        return 0;
    }

    TcpSocket* client = new TcpSocket();
    do
    {
        TcpListener* listener = dynamic_cast<TcpListener* >(SocketList[listener_id]);
        if(!listener)
        {
            last_error = Socket::InvalidSocketId;
            return 0;
        }
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

bool net_tcp_disconnect(double tcpsocketId)
{
    if(!SocketList.count(tcpsocketId)){
        last_error = Socket::InvalidSocketId;
        return 0;
    }
    TcpSocket* sock = dynamic_cast<TcpSocket* >(SocketList[tcpsocketId]);
    if(!sock)
    {
        last_error = Socket::InvalidSocketId;
        return 0;
    }
    sock->Disconnect();
    return 1;
}

bool net_tcp_connected(double tcpsocketId)
{
    if(!SocketList.count(tcpsocketId)){
        last_error = Socket::InvalidSocketId;
        return 0;
    }

    TcpSocket* sock = dynamic_cast<TcpSocket* >(SocketList[tcpsocketId]);
    if(!sock)
    {
        last_error = Socket::InvalidSocketId;
        return 0;
    }
    if(sock->GetRemoteAddress() != 0)
    {
        return 1;
    }
    return 0;
}

bool net_tcp_send(double tcpsocketId, double packetId)
{
    if(!SocketList.count(tcpsocketId)){
        last_error = Socket::InvalidSocketId;
        return 0;
    }
    if(!PacketList.count(packetId))
    {
        last_error = Socket::InvalidPacketId;
        return 0;
    }

    TcpSocket* sock = dynamic_cast<TcpSocket* >(SocketList[tcpsocketId]);
    if(!sock)
    {
        last_error = Socket::InvalidSocketId;
        return 0;
    }

    last_error = sock->Send( *(PacketList[packetId]) );
    if(last_error != Socket::Done)
    {
        return 0;
    }
    return 1;
}

bool net_tcp_receive(double tcpsocketId, double packetId)
{
    if(!SocketList.count(tcpsocketId)){
        last_error = Socket::InvalidSocketId;
        return 0;
    }
    if(!PacketList.count(packetId))
    {
        last_error = Socket::InvalidPacketId;
        return 0;
    }

    TcpSocket* sock = dynamic_cast<TcpSocket* >(SocketList[tcpsocketId]);
    if(!sock)
    {
        last_error = Socket::InvalidSocketId;
        return 0;
    }

    last_error = sock->Receive( *(PacketList[packetId]) );
    if(last_error != Socket::Done)
    {
        return 0;
    }
    return 1;
}

bool net_tcp_set_blocking(double tcpsocketId, bool blocking)
{
    if(!SocketList.count(tcpsocketId)){
        last_error = Socket::InvalidSocketId;
        return 0;
    }

    TcpSocket* sock = dynamic_cast<TcpSocket* >(SocketList[tcpsocketId]);
    if(!sock)
    {
        last_error = Socket::InvalidSocketId;
        return 0;
    }

    sock->SetBlocking(blocking);
    return 1;
}

bool net_tcp_destroy(double tcpsocketId)
{
    if(!SocketList.count(tcpsocketId)){
        last_error = Socket::InvalidSocketId;
        return 0;
    }

    delete SocketList[tcpsocketId];
    SocketList.erase(tcpsocketId);
    return 1;
}

bool net_udp_bind(unsigned short port, bool blocking)
{
    UdpSocket* sock = new UdpSocket();
    sock->SetBlocking(blocking);

    last_error = sock->Bind(port);

    if(last_error != Socket::Done)
    {
        delete sock;
        return 0;
    }
    return insertSock(sock);
}

bool net_udp_bind_available(bool blocking)
{
    return net_udp_bind(Socket::AnyPort, blocking);
}

unsigned short net_udp_localport(double udpsocketId)
{
    if(!SocketList.count(udpsocketId)){
        last_error = Socket::InvalidSocketId;
        return 0;
    }

    UdpSocket* sock = dynamic_cast<UdpSocket* >(SocketList[udpsocketId]);
    if(!sock)
    {
        last_error = Socket::InvalidSocketId;
        return 0;
    }

    return sock->GetLocalPort();
}

bool net_udp_unbind(double udpsocketId)
{
    if(!SocketList.count(udpsocketId)){
        last_error = Socket::InvalidSocketId;
        return 0;
    }

    UdpSocket* sock = dynamic_cast<UdpSocket* >(SocketList[udpsocketId]);
    if(!sock)
    {
        last_error = Socket::InvalidSocketId;
        return 0;
    }

    sock->Unbind();
    return 1;
}

bool net_udp_set_blocking(double udpsocketId, bool blocking)
{
    if(!SocketList.count(udpsocketId)){
        last_error = Socket::InvalidSocketId;
        return 0;
    }

    UdpSocket* sock = dynamic_cast<UdpSocket* >(SocketList[udpsocketId]);
    if(!sock)
    {
        last_error = Socket::InvalidSocketId;
        return 0;
    }

    sock->SetBlocking(blocking);
    return 1;
}

bool net_udp_send(double udpsocketId, double packetId, string ipAddress, unsigned short remotePort)
{
    if(!SocketList.count(udpsocketId)){
        last_error = Socket::InvalidSocketId;
        return 0;
    }
    if(!PacketList.count(packetId))
    {
        last_error = Socket::InvalidPacketId;
        return 0;
    }

    UdpSocket* sock = dynamic_cast<UdpSocket* >(SocketList[udpsocketId]);
    if(!sock)
    {
        last_error = Socket::InvalidSocketId;
        return 0;
    }

    last_error = sock->Send( *(PacketList[packetId]), IpAddress(ipAddress), remotePort);
    if(last_error != Socket::Done)
    {
        return 0;
    }
    return 1;
}

bool net_udp_receive(double udpsocketId, double packetId, string &ipAddress, unsigned short &remotePort)
{
    if(!SocketList.count(udpsocketId)){
        last_error = Socket::InvalidSocketId;
        return 0;
    }
    if(!PacketList.count(packetId))
    {
        last_error = Socket::InvalidPacketId;
        return 0;
    }

    UdpSocket* sock = dynamic_cast<UdpSocket* >(SocketList[udpsocketId]);
    if(!sock)
    {
        last_error = Socket::InvalidSocketId;
        return 0;
    }

    IpAddress ipadd(ipAddress);

    last_error = sock->Receive( *(PacketList[packetId]), ipadd, remotePort);

    ipAddress = ipadd.ToString();

    if(last_error != Socket::Done)
    {
        return 0;
    }
    return 1;
}

bool net_udp_destroy(double udpsocketId)
{
    if(!SocketList.count(udpsocketId)){
        last_error = Socket::InvalidSocketId;
        return 0;
    }

    delete SocketList[udpsocketId];
    SocketList.erase(udpsocketId);
    return 1;
}

double net_http_create()
{
    Http *ht = new Http();
    return insertHttp(ht);
}

bool net_http_host(double httpId, string host)
{
    if(!HttpList.count(httpId)){
        last_error = Socket::InvalidHttpId;
        return 0;
    }

    HttpList[httpId]->SetHost(host);
    return 1;
}

bool net_http_destroy(double httpId)
{
    if(!HttpList.count(httpId)){
        last_error = Socket::InvalidHttpId;
        return 0;
    }

    delete HttpList[httpId];
    HttpList.erase(httpId);
    return 1;
}

double net_http_request_create()
{
    Http::Request *hr = new Http::Request();
    return insertHttpRequest(hr);
}

bool net_http_request_method(double httpRequestId, string method)
{
    if(!HttpRequestList.count(httpRequestId)){
        last_error = Socket::InvalidHttpRequestId;
        return 0;
    }

    if(method == "GET")
    {
        HttpRequestList[httpRequestId]->SetMethod(Http::Request::Get);
    }
    else if(method == "POST")
    {
        HttpRequestList[httpRequestId]->SetMethod(Http::Request::Post);
    }
    else if(method == "HEAD")
    {
        HttpRequestList[httpRequestId]->SetMethod(Http::Request::Head);
    }
    else
    {
        return 0;
    }

    return 1;
}

bool net_http_request_uri(double httpRequestId, string uri)
{
    if(!HttpRequestList.count(httpRequestId)){
        last_error = Socket::InvalidHttpRequestId;
        return 0;
    }

    HttpRequestList[httpRequestId]->SetUri(uri);

    return 1;
}

bool net_http_request_field(double httpRequestId, string name, string value)
{
    if(!HttpRequestList.count(httpRequestId)){
        last_error = Socket::InvalidHttpRequestId;
        return 0;
    }

    HttpRequestList[httpRequestId]->SetField(name, value);

    return 1;
}

bool net_http_request_body(double httpRequestId, string body)
{
    if(!HttpRequestList.count(httpRequestId)){
        last_error = Socket::InvalidHttpRequestId;
        return 0;
    }

    HttpRequestList[httpRequestId]->SetBody(body);

    return 1;
}

bool net_http_request_destroy(double httpRequestId)
{
    if(!HttpRequestList.count(httpRequestId)){
        last_error = Socket::InvalidHttpRequestId;
        return 0;
    }

    delete HttpRequestList[httpRequestId];
    HttpRequestList.erase(httpRequestId);
    return 1;
}

double net_http_send(double httpId, double httpRequestId)
{
    if(!HttpList.count(httpId)){
        last_error = Socket::InvalidHttpId;
        return 0;
    }

    if(!HttpRequestList.count(httpRequestId)){
        last_error = Socket::InvalidHttpRequestId;
        return 0;
    }

    return insertHttpResponse(HttpList[httpId]->SendRequest( *(HttpRequestList[httpRequestId]) ));
}

double net_http_response_status(double httpResponseId)
{
    if(!HttpResponseList.count(httpResponseId)){
        last_error = Socket::InvalidHttpResponseId;
        return 0;
    }

    return (double) HttpResponseList[httpResponseId].GetStatus();
}

string net_http_response_body(double httpResponseId)
{
    if(!HttpResponseList.count(httpResponseId)){
        last_error = Socket::InvalidHttpResponseId;
        return 0;
    }

    return HttpResponseList[httpResponseId].GetBody();
}

bool net_http_response_destroy(double httpResponseId)
{
    if(!HttpResponseList.count(httpResponseId)){
        last_error = Socket::InvalidHttpResponseId;
        return 0;
    }

    HttpResponseList.erase(httpResponseId);
    return 1;
}

double net_utility_rc4_create(string keyString)
{
    return insertRc4(net_utility_rc4(keyString));
}

bool net_utility_rc4_step_set(double rc4Id, double step)
{
    if(!Rc4List.count(rc4Id)){
        last_error = Socket::InvalidRc4Id;
        return 0;
    }

    return Rc4List[rc4Id].step_set(step);
}

bool net_utility_rc4_step_add(double rc4Id, double step)
{
    if(!Rc4List.count(rc4Id)){
        last_error = Socket::InvalidRc4Id;
        return 0;
    }

    return Rc4List[rc4Id].step_add(step);
}

bool net_utility_rc4_step(double rc4Id)
{
    if(!Rc4List.count(rc4Id)){
        last_error = Socket::InvalidRc4Id;
        return 0;
    }

    return Rc4List[rc4Id].do_step();
}

bool net_utility_rc4_xor(double rc4Id, double packetId)
{
    if(!Rc4List.count(rc4Id)){
        last_error = Socket::InvalidRc4Id;
        return 0;
    }

    return Rc4List[rc4Id].do_xor(packetId);
}

bool net_utility_rc4_destroy(double rc4Id)
{
    if(!Rc4List.count(rc4Id)){
        last_error = Socket::InvalidRc4Id;
        return 0;
    }

    Rc4List.erase(rc4Id);
    return 1;
}

char net_utility_rc4_byte(double rc4Id)
{
    if(!Rc4List.count(rc4Id)){
        last_error = Socket::InvalidRc4Id;
        return 0;
    }

    return Rc4List[rc4Id].currentByte();
}

double net_packet_create()
{
    Packet* pack = new Packet();
    return insertPacket(pack);
}

template <class T>
bool net_packet_write(double packetId, T value)
{
    if(!PacketList.count(packetId))
    {
        last_error = Socket::InvalidPacketId;
        return 0;
    }
    *(PacketList[packetId]) << value;
    return 1;
}

template <class T>
bool net_packet_read(double packetId, T &var)
{
    if(!PacketList.count(packetId))
    {
        last_error = Socket::InvalidPacketId;
        return 0;
    }

    if(*(PacketList[packetId]) >> var)
    {
        return 1;
    }
    last_error = Socket::PacketEmpty;
    return 0;
}

bool net_packet_destroy(double packetId)
{
    if(PacketList.count(packetId))
    {
        delete PacketList[packetId];
        PacketList.erase(packetId);
        return 1;
    }
    last_error = Socket::InvalidPacketId;
    return 0;
}

bool net_packet_clear(double packetId)
{
    if(!PacketList.count(packetId))
    {
        last_error = Socket::InvalidPacketId;
        return 0;
    }
    PacketList[packetId]->Clear();
    return 1;
}

double net_packet_size(double packetId)
{
    if(!PacketList.count(packetId))
    {
        last_error = Socket::InvalidPacketId;
        return 0;
    }
    return PacketList[packetId]->GetDataSize();
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
        case Socket::InvalidSocketId:
            error = "InvalidSocketId";
            break;
        case Socket::InvalidPacketId:
            error = "InvalidPacketId";
            break;
        case Socket::InvalidHttpId:
            error = "InvalidHttpId";
            break;
        case Socket::InvalidHttpRequestId:
            error = "InvalidHttpRequestId";
            break;
        case Socket::InvalidHttpResponseId:
            error = "InvalidHttpResponseId";
            break;
        case Socket::InvalidRc4Id:
            error = "InvalidRc4Id";
            break;
        case Socket::PacketEmpty:
            error = "PacketEmpty";
            break;
    }
    return error;
}

int main()
{
    double httpId = net_http_create();
    net_http_host(httpId, "enigma-dev.org");

    double request = net_http_request_create();
    net_http_request_method(request, "GET");
    net_http_request_uri(request, "/tracker/");

    double response = net_http_send(httpId, request);
    cout << net_http_response_body(response) << endl;

    net_http_response_destroy(response);
    net_http_request_destroy(request);
    net_http_destroy(httpId);

    double rc4_1 = net_utility_rc4_create("A-vEry-lONg-test-KEY");
    double rc4_2 = net_utility_rc4_create("A-vEry-lONg-test-KEY");

    net_utility_rc4_step_add(rc4_1, 1000);
    net_utility_rc4_step_add(rc4_1, 350);

    net_utility_rc4_step_set(rc4_2, 1350);

    cout << net_utility_rc4_byte(rc4_1);
    net_utility_rc4_step(rc4_1);
    cout << net_utility_rc4_byte(rc4_1);
    net_utility_rc4_step(rc4_1);
    cout << net_utility_rc4_byte(rc4_1);
    net_utility_rc4_step(rc4_1);
    cout << net_utility_rc4_byte(rc4_1);
    cout << endl;

    cout << net_utility_rc4_byte(rc4_2) << endl;

    net_utility_rc4_destroy(rc4_1);
    net_utility_rc4_destroy(rc4_2);

    return 1;
}

