
#include "Message.h"

Message::Message(){
    dispatch = false;
	printedToSerial = false;
}

Message::~Message(){
    // delete &length;
    // delete &frame_id;
    // delete frame_data;
}

