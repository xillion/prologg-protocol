#include "transport/prologg_transport.hpp"

#include <cstring>

using namespace std::chrono;

ProloggTransport::ProloggTransport(USBSerial &usb,
                                    uint8_t *cobs_buffer, std::size_t cobs_buffer_size,
                                    uint8_t *prologg_buffer, std::size_t prologg_buffer_size,
                                    DigitalOut *activity_led)
    : _usb(usb),
      _cobs_buffer(cobs_buffer),
      _cobs_buffer_size(cobs_buffer_size),
      _prologg_buffer(prologg_buffer),
      _prologg_buffer_size(prologg_buffer_size),
      _activity_led(activity_led)
{
}

void ProloggTransport::runSession()
{
    static Request request = Request_init_zero;
    static Response response = Response_init_zero;

    Timer usb_timeout;
    std::size_t rx_idx = 0;

    usb_timeout.start();
    std::memset(_cobs_buffer, 0, _cobs_buffer_size);

    while (_usb.connected()) {
        if (_usb.available()) {
            uint8_t byte = _usb.getc();

            if (byte == 0x00) {
                if (_activity_led) {
                    *_activity_led = 0;
                }

                if (rx_idx > 0) {
                    if (rx_idx < _cobs_buffer_size) {
                        _cobs_buffer[rx_idx++] = byte;
                    }

                    std::size_t decoded_len = 0;
                    cobs_ret_t ret = cobs_decode(
                        _cobs_buffer,          // in: coded data
                        rx_idx,                // in len
                        _prologg_buffer,       // out: decoded data
                        _prologg_buffer_size,  // max size for decoding
                        &decoded_len           // actual length after decoding
                    );

                    if (ret == COBS_RET_SUCCESS && decoded_len >= 4) {
                        std::uint32_t calculated_crc;
                        std::uint32_t received_crc = (_prologg_buffer[decoded_len - 4] << 24) |
                                                (_prologg_buffer[decoded_len - 3] << 16) |
                                                (_prologg_buffer[decoded_len - 2] << 8)  |
                                                    _prologg_buffer[decoded_len - 1];

                        _crc.compute((void *)_prologg_buffer, decoded_len - 4, &calculated_crc);

                        if (received_crc == calculated_crc) {
                            pb_istream_t stream = pb_istream_from_buffer(_prologg_buffer, decoded_len - 4);
                            if (pb_decode(&stream, Request_fields, &request)) {
                                handle_request(request, response);
                            } else {
                                // Error protobuf — bad format
                                response.error = ErrorCode_PROTOCOL_ERROR;
                            }
                        } else {
                            // Error bad CRC
                            response.error = ErrorCode_CRC_ERROR;
                        }
                    } else {
                        // Error COBS decode or too short packet
                        switch (ret) {
                            case COBS_RET_ERR_BAD_ARG:
                                response.error = ErrorCode_COBS_BAD_ARG;
                                break;
                            case COBS_RET_ERR_BAD_PAYLOAD:
                                response.error = ErrorCode_COBS_BAD_PAYLOAD;
                                break;
                            case COBS_RET_ERR_EXHAUSTED:
                                response.error = ErrorCode_COBS_EXHAUSTED;
                                break;
                            default:
                                break;
                        }
                    }

                    // Send response
                    pb_ostream_t stream = pb_ostream_from_buffer(_prologg_buffer, _prologg_buffer_size);
                    pb_encode(&stream, Response_fields, &response);

                    std::size_t payload_len = stream.bytes_written;

                    // Add crc to the end of payload
                    std::uint32_t crc;
                    _crc.compute((void *)_prologg_buffer, payload_len, &crc);

                    _prologg_buffer[payload_len++] = (crc >> 24) & 0xFF;
                    _prologg_buffer[payload_len++] = (crc >> 16) & 0xFF;
                    _prologg_buffer[payload_len++] = (crc >> 8)  & 0xFF;
                    _prologg_buffer[payload_len++] = crc & 0xFF;

                    // COBS encode
                    std::size_t cobs_len = 0;
                    ret = cobs_encode(
                        _prologg_buffer,      // in: payload + CRC
                        payload_len,          // length
                        _cobs_buffer,         // out
                        _cobs_buffer_size,    // maximum
                        &cobs_len             // actual length
                    );

                    if (ret == COBS_RET_SUCCESS) {
                        _cobs_buffer[cobs_len] = 0x00;
                        _usb.write(_cobs_buffer, cobs_len + 1);
                    }
                }
                rx_idx = 0;
                usb_timeout.reset();
            } else {
                if (rx_idx < _cobs_buffer_size) {
                    _cobs_buffer[rx_idx++] = byte;
                    if (_activity_led) {
                        *_activity_led = (rx_idx % 2 == 0) ? 1 : 0;
                    }
                } else {
                    rx_idx = 0;
                    if (_activity_led) {
                        *_activity_led = 0;
                    }
                }
                usb_timeout.reset();
            }
        }

        if (rx_idx > 0 && duration_cast<milliseconds>(usb_timeout.elapsed_time()).count() > 100) {
            rx_idx = 0;
            usb_timeout.reset();
        }
    }
}
