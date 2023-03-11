#include <stdint.h>
#include <stddef.h>

#define RX_BUFFER_LEN   64

class UART {
    public:
    UART();
    void init();
    void setBaudrate(int baudrate);
    int available();
    uint8_t read();
    size_t readBytes(uint8_t *buf, size_t size);
    size_t sendBytes(const uint8_t *buf, size_t size);

    void _isrCallback();

    private:
    void initGpio();
    void initUart();
    void initDma();
    uint8_t _rxBuf[RX_BUFFER_LEN];
    uint16_t _rxBufIdx = 0;
};

extern UART uart1;
