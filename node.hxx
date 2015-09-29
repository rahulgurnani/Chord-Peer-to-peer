#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <arpa/inet.h>
#include <string>

#define M 8

using namespace std;

class info
{
public:
	// Constructor
	info(unsigned long _ip = 0,int _port = 0);
	info(string _ip,string _port);
	info(info&);					// copy constructor
	~info();						// destructor
	unsigned long get_ip();
	int get_port();
	unsigned long long get_mid();
	unsigned long ip;
	int port;
	unsigned long long mid;
};

unsigned long long oat_hash(string s);
class Node
{
public:
	// constructor to set things 
	Node(unsigned long _ip = 0, int _port = 0);
	//Node(string ip, string port);
	Node(Node&);
	Node(string _ip, string _port);
	// destructor
	~Node();
	// get set fucntions for successor and predecessor
	info get_successor();
	info get_predecessor();
	void set_successor(info&);
	void set_predecessor(info&);
	unsigned long get_ip();
	int get_port();
	unsigned long long get_mid();
private:
	unsigned long ip;
	int port;
	unsigned long long machine_id;
	info successor;			// successor has hash value just greater than current's machine id
	info predecessor;			// predecessor has hash value just lesser than current's machine_id
};