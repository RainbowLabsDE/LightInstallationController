#include <stdint.h>
#include <stddef.h>


class UART {
    public:
    UART();
    void init();
    void setBaudrate(int baudrate);
    int available();
    uint8_t read();
    size_t readBytes(uint8_t *buf, size_t size);

    void _isrCallback();

    private:
    void initGpio();
    void initUart();
    void initDma();
    uint8_t _uartRxBuf[64];
    uint16_t _uartRxBufReadIdx = 0;
};

static UART uart1;
