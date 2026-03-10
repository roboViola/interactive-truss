#ifndef MESSAGE_TYPES_H
#define MESSAGE_TYPES_H

// Enumerated Datatype for Message Type
enum MessageType : uint8_t {
    MSG_DATA,
    MSG_RESET
}

// Structure for Sensor Data
struct sense_msg {
    MessageType msg_type;
    float force_data;
};

// Structure for Reset
struct reset_msg {
    MessageType msg_type;
    bool reset_signal;
};

#endif