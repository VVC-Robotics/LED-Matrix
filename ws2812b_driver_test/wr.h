
#define NOP __asm__ __volatile__ ("nop\n\t")

#pragma GCC optimize "Os"

template<byte bit>
__attribute__((always_inline)) void bit_write(volatile byte *port, const byte &b) { }

template<>
__attribute__((always_inline)) inline void bit_write<0>(volatile byte *port, const byte &b) {
    *port &= ~(1 << b);
}

template<>
__attribute__((always_inline)) inline void bit_write<1>(volatile byte *port, const byte &b) {
    *port |= (1 << b);
}

#pragma GCC optimize "Os"
