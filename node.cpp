#include "node.hxx"

using namespace std;
unsigned long long oat_hash(string s)
{
	unsigned long long h = 0;
	int i;
	int len = s.length();
	for (i = 0; i < len; i++)
	{
		h += s[i];
		h += (h << 10);
		h ^= (h >> 6);
	}

	h += (h << 3);
	h ^= (h >> 11);
	h += (h << 15);
	return h%(1<<M);
}
// constructor
info::info(unsigned long _ip,int _port)
{
	ip = _ip;
	port = _port;
	if(_ip == 0 && _port == 0)
		mid = 0;
	else
	{	
		struct in_addr ip_addr;
		ip_addr.s_addr = _ip;
		char temp[200];
		sprintf(temp,"%s:%d",inet_ntoa(ip_addr),_port);
		mid = oat_hash(string(temp));
	}
}
info::info(string _ip,string _port)
{
	struct in_addr ip_addr;
	inet_aton(_ip.c_str(),&ip_addr);
	ip =  ip_addr.s_addr;
	port = atoi(_port.c_str());
	mid = oat_hash(_ip+":"+_port);
}
// copy constructor
info::info(info& i)
{
	ip = i.ip;
	port = i.port;
	mid = i.mid;
}
// destructor
info::~info()
{
	// nothing to be done in destructor
}
unsigned long info::get_ip()
{
	return ip;
}
int info::get_port()
{
	return port;
}
unsigned long long info::get_mid()
{
	return mid;
}

Node::Node(string _ip, string _port)
{
	struct in_addr ip_addr;
	inet_aton(_ip.c_str(),&ip_addr);
	ip =  ip_addr.s_addr;
	port = atoi(_port.c_str());
	successor = info();
	predecessor = info();
	machine_id = oat_hash(_ip+":"+_port);

}
Node::Node(unsigned long _ip, int _port)
{
	struct in_addr ip_addr;
	ip_addr.s_addr = _ip;
	ip = _ip;
	port = _port;
	string s_ip = string(inet_ntoa(ip_addr));
	char temp[200];
	sprintf(temp,"%s:%d",s_ip.c_str(),_port);
	machine_id = oat_hash(string(temp));
	successor = info();
	predecessor = info();
}
Node::Node(Node& node)
{
	// copy constructor
	ip = node.ip;
	port = node.port;
	machine_id = node.machine_id;
	successor = node.successor;
	predecessor = node.predecessor;
}
Node::~Node()
{
	// nothing to be done here
}
info Node::get_successor()
{
	return successor;
}
info Node::get_predecessor()
{
	return predecessor;
}
void Node::set_successor(info& s)
{
	successor = info(s);
	//printf(" after setting successor\n");
}
void Node::set_predecessor(info& p)
{
	predecessor = info(p);
}
unsigned long Node::get_ip()
{
	return ip;
}
int Node::get_port()
{
	return port;
}
unsigned long long Node::get_mid()
{
	return machine_id;
}
