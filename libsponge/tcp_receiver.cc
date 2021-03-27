#include "tcp_receiver.hh"

// Dummy implementation of a TCP receiver

// For Lab 2, please replace with a real implementation that passes the
// automated checks run by `make check_lab2`.

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

void TCPReceiver::segment_received(const TCPSegment &seg) {
    if(seg.header().syn) { ISN = seg.header().seqno; SYN = true;}
    else if(!SYN) return;
    if(seg.header().fin) FIN = true;
    uint64_t index = unwrap(seg.header().seqno, ISN, checkpoint);
    _reassembler.push_substring(seg.payload().copy(), (index>0?index-1:0), FIN);
    checkpoint = index;
    if(seg.header().seqno == ACKNO || seg.header().syn) {
        uint32_t SF = (seg.header().syn ? 1 : 0) + (seg.header().fin ? 1 : 0);
        ACKNO = WrappingInt32(seg.payload().size() + seg.header().seqno.raw_value() + SF);
    }
}

optional<WrappingInt32> TCPReceiver::ackno() const { 
    if(SYN) return ACKNO;
    else return {};
}

size_t TCPReceiver::window_size() const { return _capacity - _reassembler.stream_out().buffer_size(); }
