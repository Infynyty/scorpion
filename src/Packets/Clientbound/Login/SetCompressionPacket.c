//
// Created by Kasimir on 16.10.2022.
//

#include "SetCompressionPacket.h"
#include "../../../Util/NetworkBuffer.h"
#include "../../../Util/Logging/Logger.h"

void set_compression_packet_handle(SocketWrapper *socket) {
    NetworkBuffer* buffer = buffer_new();
    int compression_threshold = varint_receive(socket);

    buffer_free(buffer);
    cmc_log(INFO, "Compression threshold set to %d bytes", compression_threshold);
}