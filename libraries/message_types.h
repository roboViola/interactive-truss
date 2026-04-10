#ifndef MESSAGE_TYPES_H
#define MESSAGE_TYPES_H

// Enumerated Datatype for Message Type
enum class MessageType : uint8_t {
    MSG_DATA,
    MSG_RESET,
    MSG_PAIR
};

// Structure for Sensor Data
struct sense_msg {
    MessageType msg_type = MessageType::MSG_DATA;
    uint8_t id;
    float force_data;
};

// Structure for Zeroing
struct zero_msg {
    MessageType msg_type = MessageType::MSG_RESET;
    bool zero_signal;
};

// Structure for Pairing
struct zero_msg {
    MessageType msg_type = MessageType::MSG_PAIR;
    uint8_t id;
};

#endif