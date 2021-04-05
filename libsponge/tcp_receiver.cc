#include "tcp_receiver.hh"

#include <iostream>

// Dummy implementation of a TCP receiver

// For Lab 2, please replace with a real implementation that passes the
// automated checks run by `make check_lab2`.

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

void TCPReceiver::segment_received(const TCPSegment &seg) {
    if (seg.header().syn) {
        ISN = seg.header().seqno;
        SYN = true;
    } else if (!SYN)
        return;
    uint64_t index = unwrap(seg.header().seqno, ISN, checkpoint);
    if(seg.header().seqno == ISN && seg.payload().size() != 0 && repeat) return;
    if(index >= checkpoint + _capacity - unassembled_bytes() + 1) return;
    _reassembler.push_substring(seg.payload().copy(), (index > 0 ? index - 1 : 0), seg.header().fin);
    checkpoint = index;
    if (seg.header().seqno == ACKNO || seg.header().syn) {
        uint32_t F = (_reassembler.stream_out().input_ended() ? 1 : 0);
        ACKNO = wrap(_reassembler.index() + 1 + F, ISN);
    }
    if(SYN) repeat = true;
}

optional<WrappingInt32> TCPReceiver::ackno() const {
    if (SYN)
        return ACKNO;
    else
        return {};
}

size_t TCPReceiver::window_size() const { return _capacity - _reassembler.stream_out().buffer_size(); }
