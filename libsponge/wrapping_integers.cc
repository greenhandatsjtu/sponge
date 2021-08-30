#include "wrapping_integers.hh"

// Dummy implementation of a 32-bit wrapping integer

// For Lab 2, please replace with a real implementation that passes the
// automated checks run by `make check_lab2`.

using namespace std;

//! Transform an "absolute" 64-bit sequence number (zero-indexed) into a WrappingInt32
//! \param n The input absolute 64-bit sequence number
//! \param isn The initial sequence number
WrappingInt32 wrap(uint64_t n, WrappingInt32 isn) { return isn + static_cast<uint32_t>(n); }

//! Transform a WrappingInt32 into an "absolute" 64-bit sequence number (zero-indexed)
//! \param n The relative sequence number
//! \param isn The initial sequence number
//! \param checkpoint A recent absolute 64-bit sequence number
//! \returns the 64-bit sequence number that wraps to `n` and is closest to `checkpoint`
//!
//! \note Each of the two streams of the TCP connection has its own ISN. One stream
//! runs from the local TCPSender to the remote TCPReceiver and has one ISN,
//! and the other stream runs from the remote TCPSender to the local TCPReceiver and
//! has a different ISN.
uint64_t unwrap(WrappingInt32 n, WrappingInt32 isn, uint64_t checkpoint) {
    WrappingInt32 w = wrap(checkpoint, isn);
    uint64_t a = static_cast<int64_t>(n - w) + checkpoint;
    uint64_t b = a + (1ull << 32);
    uint64_t c = a - (1ull << 32);
    uint64_t distance_a = a > checkpoint ? a - checkpoint : checkpoint - a;
    uint64_t distance_b = b > checkpoint ? b - checkpoint : checkpoint - b;
    uint64_t distance_c = c > checkpoint ? c - checkpoint : checkpoint - c;
    return distance_a < distance_b ? (distance_a < distance_c ? a : (distance_c < distance_b ? c : 0))
                                   : (distance_b < distance_c ? b : (distance_c < distance_a ? c : 0));
}
