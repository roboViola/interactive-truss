#ifndef MESSAGE_TYPES_H
#define MESSAGE_TYPES_H

// Enumerated Datatype for Message Type
enum class MessageType : uint8_t {
    MSG_DATA,
    MSG_RESET
};

// Structure for Sensor Data
struct sense_msg {
    MessageType msg_type = MessageType::MSG_DATA;
    float force_data;
};

// Structure for Zeroing
struct zero_msg {
    MessageType msg_type = MessageType::MSG_RESET;
    bool zero_signal;
};

#endif