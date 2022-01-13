#ifndef _NET_MSG_H_
#define _NET_MSG_H_

#include <cstring>
#include <iostream>

class msg_base
{
public:
	msg_base() { }
	msg_base(uint32_t sid, uint16_t msg_id, uint16_t size, const char *buf)
		: m_sid(sid),
		m_msg_id(msg_id),
		m_size(size),
		m_buf(NULL)
	{
		m_buf = new char[m_size];
		memcpy(m_buf, buf, m_size);
	}
	virtual ~msg_base() { delete[]m_buf; }

public:
	uint32_t get_sid() { return m_sid; }
	uint16_t get_msg_id() { return m_msg_id; }
	uint16_t get_size() { return m_size; }

protected:
	char *get_buf() { return m_buf; }

private:
	uint32_t m_sid;

	uint16_t m_msg_id;
	uint16_t m_size;
	char *m_buf;
};

class msg_hello : public msg_base
{
public:
	msg_hello() { }
	virtual ~msg_hello() { }

public:
	int32_t get_hp() { return *(int32_t *)(this->get_buf()); }
	int32_t get_mp() { return *(int32_t *)(this->get_buf() + 4); }
	//void set_hp(int32_t hp) { *(int32_t *)(this->get_buf()) = hp; }
	//void set_mp(int32_t mp) { *(int32_t *)(this->get_buf() + 4) = mp; }
};

class msg_world : public msg_base
{
public:
	msg_world() { }
	virtual ~msg_world() { }

public:
	std::string get_content() { return (const char *)(this->get_buf()); }
	//void set_content(std::string &content) { return (char *)(this->get_buf()); }
};

#endif