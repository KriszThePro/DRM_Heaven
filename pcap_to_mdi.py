#!/usr/bin/env python3
"""Extract DRM AF/TAG UDP payloads from .pcap into a raw .mdi stream.

This helper is intended for test conversion only.
It supports classic PCAP (not PCAPNG), raw IPv4 linktype (101) and Ethernet (1).
"""

from __future__ import annotations

import argparse
import struct
from dataclasses import dataclass, field
from typing import Dict, Iterable, List, Optional, Tuple


@dataclass
class PcapPacket:
    payload: bytes


@dataclass
class ReassemblyState:
    fragments: Dict[int, bytes] = field(default_factory=dict)
    final_len: Optional[int] = None

    def add(self, offset_bytes: int, data: bytes, more_fragments: bool) -> None:
        self.fragments[offset_bytes] = data
        if not more_fragments:
            self.final_len = offset_bytes + len(data)

    def try_build(self) -> Optional[bytes]:
        if self.final_len is None:
            return None

        expected = 0
        parts: List[bytes] = []
        for offset in sorted(self.fragments):
            part = self.fragments[offset]
            if offset != expected:
                return None
            parts.append(part)
            expected += len(part)

        if expected != self.final_len:
            return None

        return b"".join(parts)


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Extract AF/TAG UDP payloads from pcap to .mdi")
    parser.add_argument("input_pcap", help="Path to input .pcap")
    parser.add_argument("output_mdi", help="Path to output .mdi")
    parser.add_argument("--dst-port", type=int, default=1234, help="Filter destination UDP port (default: 1234)")
    parser.add_argument("--src-port", type=int, default=None, help="Optional source UDP port filter")
    parser.add_argument("--all-udp", action="store_true", help="Write all filtered UDP payloads, not only AF packets")
    return parser.parse_args()


def iter_pcap_packets(path: str) -> Tuple[int, Iterable[PcapPacket]]:
    with open(path, "rb") as f:
        gh = f.read(24)
        if len(gh) < 24:
            raise ValueError("Invalid pcap: global header too short")

        magic = gh[:4]
        if magic == b"\xd4\xc3\xb2\xa1":
            endian = "<"
        elif magic == b"\xa1\xb2\xc3\xd4":
            endian = ">"
        else:
            raise ValueError(f"Unsupported pcap magic: {magic.hex()}")

        _, _, _, _, _, linktype = struct.unpack(endian + "HHiiii", gh[4:24])

        packets: List[PcapPacket] = []
        while True:
            ph = f.read(16)
            if not ph:
                break
            if len(ph) < 16:
                break

            _, _, incl_len, _ = struct.unpack(endian + "IIII", ph)
            payload = f.read(incl_len)
            if len(payload) < incl_len:
                break
            packets.append(PcapPacket(payload=payload))

    return linktype, packets


def extract_ipv4_payload(packet: bytes, linktype: int) -> Optional[bytes]:
    if linktype == 101:  # DLT_RAW
        return packet

    if linktype == 1:  # Ethernet
        if len(packet) < 14:
            return None
        eth_type = struct.unpack("!H", packet[12:14])[0]
        if eth_type != 0x0800:
            return None
        return packet[14:]

    return None


def parse_ipv4(ip: bytes) -> Optional[Tuple[Tuple[int, int, int, int], Tuple[int, int, int, int], int, int, bool, bytes]]:
    if len(ip) < 20:
        return None

    vihl = ip[0]
    version = vihl >> 4
    ihl = (vihl & 0x0F) * 4
    if version != 4 or ihl < 20 or len(ip) < ihl:
        return None

    total_len = struct.unpack("!H", ip[2:4])[0]
    ident = struct.unpack("!H", ip[4:6])[0]
    flags_off = struct.unpack("!H", ip[6:8])[0]
    protocol = ip[9]
    src = tuple(ip[12:16])
    dst = tuple(ip[16:20])

    mf = (flags_off & 0x2000) != 0
    frag_off_bytes = (flags_off & 0x1FFF) * 8

    if total_len < ihl:
        return None

    payload_end = min(total_len, len(ip))
    payload = ip[ihl:payload_end]

    return src, dst, protocol, ident, mf, (frag_off_bytes, payload)


def assemble_udp_payloads(linktype: int, packets: Iterable[PcapPacket]) -> Iterable[Tuple[int, int, bytes]]:
    reassembly: Dict[Tuple[Tuple[int, int, int, int], Tuple[int, int, int, int], int, int], ReassemblyState] = {}

    for pkt in packets:
        ip = extract_ipv4_payload(pkt.payload, linktype)
        if ip is None:
            continue

        parsed = parse_ipv4(ip)
        if parsed is None:
            continue

        src, dst, protocol, ident, mf, frag = parsed
        if protocol != 17:  # UDP only
            continue

        frag_off_bytes, frag_payload = frag

        if frag_off_bytes == 0 and not mf:
            if len(frag_payload) < 8:
                continue
            src_port, dst_port, udp_len, _ = struct.unpack("!HHHH", frag_payload[:8])
            udp_payload = frag_payload[8:8 + max(0, udp_len - 8)]
            yield src_port, dst_port, udp_payload
            continue

        key = (src, dst, protocol, ident)
        state = reassembly.setdefault(key, ReassemblyState())
        state.add(frag_off_bytes, frag_payload, mf)

        merged = state.try_build()
        if merged is None:
            continue

        if len(merged) >= 8:
            src_port, dst_port, udp_len, _ = struct.unpack("!HHHH", merged[:8])
            udp_payload = merged[8:8 + max(0, udp_len - 8)]
            yield src_port, dst_port, udp_payload

        del reassembly[key]


def main() -> int:
    args = parse_args()

    linktype, packets = iter_pcap_packets(args.input_pcap)

    total_udp = 0
    filtered_udp = 0
    af_kept = 0
    out_bytes = 0

    with open(args.output_mdi, "wb") as out:
        for src_port, dst_port, udp_payload in assemble_udp_payloads(linktype, packets):
            total_udp += 1

            if args.src_port is not None and src_port != args.src_port:
                continue
            if dst_port != args.dst_port:
                continue

            filtered_udp += 1

            if args.all_udp:
                out.write(udp_payload)
                out_bytes += len(udp_payload)
                continue

            if udp_payload.startswith(b"AF"):
                out.write(udp_payload)
                out_bytes += len(udp_payload)
                af_kept += 1

    print(f"Linktype: {linktype}")
    print(f"UDP datagrams parsed: {total_udp}")
    print(f"UDP datagrams after port filters: {filtered_udp}")
    if not args.all_udp:
        print(f"AF payloads written: {af_kept}")
    print(f"Output bytes: {out_bytes}")
    print(f"Output file: {args.output_mdi}")

    return 0


if __name__ == "__main__":
    raise SystemExit(main())

