#include "Fingerprint.h"


int main() {
    internal::detail::Fingerprint fingerprint;
    std::cout << fingerprint.getBoardSerial() << std::endl;
    std::cout << fingerprint.getCPUSerial() << std::endl;
    std::cout << fingerprint.getMAC() << std::endl;

    return 0;
}
