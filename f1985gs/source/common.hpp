#ifndef _COMMON_H_
#define _COMMON_H_

#include <google/protobuf/message_lite.h>
#include "net_worker.hpp"
#include "console_reader.hpp"

class msg_send_guard
{
public:
    msg_send_guard(int32_t pointer_id, uint16_t msg_id, net_worker& net_worker, google::protobuf::MessageLite& message)
        : m_pointer_id(pointer_id)
        , m_msg_id(msg_id)
        , m_net_worker(net_worker)
        , m_message(message)
    {
    }
    ~msg_send_guard()
    {
        void* ptr = m_data_packet.write_data(nullptr, m_message.ByteSize());
        if (m_message.SerializePartialToArray(ptr, m_message.ByteSize()))
        {
            std::cout << m_message.ByteSize() << std::endl;
            std::cout << m_data_packet.size() << std::endl;
            m_net_worker.send(m_pointer_id, m_msg_id, m_data_packet.get_mem_ptr(), m_data_packet.size());
        }
    }

private:
    int32_t m_pointer_id;
    uint16_t m_msg_id;
    net_worker& m_net_worker;
    google::protobuf::MessageLite& m_message;
    data_packet m_data_packet;
};

#define SEND_GUARD(POINTER_ID, MSG_ID, NET_WORKER, MSG_TYPE) MSG_TYPE msg; \
	msg_send_guard send_guard(POINTER_ID, MSG_ID, NET_WORKER, msg)

#endif
