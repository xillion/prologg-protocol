#pragma once

#include <cstddef>
#include <cstdint>
#include "mbed.h"
#include "USBSerial.h"
#include "cobs.h"
#include "prologg.pb.h"
#include "prologg_dispatch.h"
#include "pb_decode.h"
#include "pb_encode.h"

// Общий транспортный слой протокола prologg: COBS-кадрирование + CRC-32/ANSI
// + диспетчеризация через handle_request() (сгенерированную из prologg.proto
// по тегам, см. generate_dispatch.py в этом же submodule).
//
// Это именно та часть, которая раньше была продублирована дословно в
// pt-logger и pt-logger-test и уже успела разойтись (разные сигнатуры
// handle_request, разная обработка USB connect/disconnect) — источник
// нескольких найденных на стенде багов. Логика конкретной USB-сессии
// (подключение/отключение по DTR, синхронизация времени, координация с
// измерительным потоком) сюда намеренно НЕ входит: она завязана на разные
// мьютексы/треды в каждой прошивке и остаётся в main.cpp каждой из них.
//
// CRC-32/ANSI: poly 0x04C11DB7, init 0x00000000, без bit-reflect, без
// final XOR (см. sensors_data_format.md в этом submodule).
class ProloggTransport {
public:
    // cobs_buffer/prologg_buffer — буферы, которыми владеет прошивка
    // (обычно глобальные массивы); transport их не выделяет и не освобождает.
    // activity_led — опциональный индикатор приёма байт (мигание на RX),
    // можно передать nullptr, если индикация не нужна.
    ProloggTransport(USBSerial &usb,
                      uint8_t *cobs_buffer, std::size_t cobs_buffer_size,
                      uint8_t *prologg_buffer, std::size_t prologg_buffer_size,
                      DigitalOut *activity_led = nullptr);

    // Блокирующий цикл одной USB-сессии. Вызывать после того, как вызывающий
    // код убедился, что usb.connected() && usb.configured() (DTR + сессия
    // терминала). Читает байты, при получении полного COBS-кадра —
    // декодирует, проверяет CRC, вызывает handle_request(), кодирует и
    // отправляет ответ. Возврат — когда usb.connected() становится false
    // (обрыв сессии) либо истёк межбайтовый таймаут кадра (100 мс).
    void runSession();

private:
    USBSerial &_usb;
    uint8_t *_cobs_buffer;
    std::size_t _cobs_buffer_size;
    uint8_t *_prologg_buffer;
    std::size_t _prologg_buffer_size;
    DigitalOut *_activity_led;

    MbedCRC<POLY_32BIT_ANSI, 32> _crc{0, 0, false, false};
};
