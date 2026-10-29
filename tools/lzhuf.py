#!/usr/bin/env python3
"""DAZZLE's LZHUF-compatible stream codec (4 KiB window, 60-byte lookahead)."""

from __future__ import annotations

import argparse
import sys
from pathlib import Path


WINDOW = 4096
LOOKAHEAD = 60
THRESHOLD = 2
SYMBOLS = 256 - THRESHOLD + LOOKAHEAD  # 314
TREE = SYMBOLS * 2 - 1                # 627
ROOT = TREE - 1                       # 626
NIL = WINDOW
MASK = WINDOW - 1


def _position_prefix(position_class: int) -> tuple[int, int]:
    """Return the fixed Huffman prefix (value, bit count) for one class."""
    if position_class == 0:
        return 0, 3
    if position_class <= 3:
        return position_class + 1, 4
    if position_class <= 11:
        return position_class + 6, 5
    if position_class <= 23:
        return position_class + 24, 6
    if position_class <= 47:
        return position_class + 72, 7
    return position_class + 192, 8


_POSITION_CLASS = [0] * 256
_POSITION_LENGTH = [0] * 256
for _class in range(64):
    _prefix, _length = _position_prefix(_class)
    _first = _prefix << (8 - _length)
    _limit = (_prefix + 1) << (8 - _length)
    for _value in range(_first, _limit):
        _POSITION_CLASS[_value] = _class
        _POSITION_LENGTH[_value] = _length


class BitReader:
    def __init__(self, data: bytes):
        self.data = data
        self.byte_offset = 0
        self.bit_offset = 0

    def read_bit(self) -> int:
        if self.byte_offset >= len(self.data):
            return 0
        bit = (self.data[self.byte_offset] >> (7 - self.bit_offset)) & 1
        self.bit_offset += 1
        if self.bit_offset == 8:
            self.bit_offset = 0
            self.byte_offset += 1
        return bit

    def read_bits(self, count: int) -> int:
        value = 0
        for _ in range(count):
            value = (value << 1) | self.read_bit()
        return value


class BitWriter:
    def __init__(self):
        self.data = bytearray()
        self.buffer = 0
        self.count = 0

    def write(self, value: int, count: int) -> None:
        for shift in range(count - 1, -1, -1):
            self.buffer = (self.buffer << 1) | ((value >> shift) & 1)
            self.count += 1
            if self.count == 8:
                self.data.append(self.buffer)
                self.buffer = 0
                self.count = 0

    def finish(self) -> bytes:
        if self.count:
            self.data.append(self.buffer << (8 - self.count))
            self.buffer = 0
            self.count = 0
        return bytes(self.data)


class AdaptiveHuffman:
    """The exact 314-symbol adaptive tree used by DAZZLE's decoder."""

    def __init__(self):
        self.frequency = [0] * (TREE + 1)
        self.son = [0] * TREE
        self.parent = [0] * (TREE + SYMBOLS)
        self.start()

    def start(self) -> None:
        for symbol in range(SYMBOLS):
            self.frequency[symbol] = 1
            self.son[symbol] = symbol + TREE
            self.parent[symbol + TREE] = symbol
        i = 0
        j = SYMBOLS
        while j <= ROOT:
            self.frequency[j] = self.frequency[i] + self.frequency[i + 1]
            self.son[j] = i
            self.parent[i] = self.parent[i + 1] = j
            i += 2
            j += 1
        self.frequency[TREE] = 0xFFFF
        self.parent[ROOT] = 0

    def reconstruct(self) -> None:
        j = 0
        for i in range(TREE):
            if self.son[i] >= TREE:
                self.frequency[j] = (self.frequency[i] + 1) >> 1
                self.son[j] = self.son[i]
                j += 1
        i = 0
        j = SYMBOLS
        while j < TREE:
            k = i + 1
            frequency = self.frequency[i] + self.frequency[k]
            self.frequency[j] = frequency
            k = j - 1
            while frequency < self.frequency[k]:
                k -= 1
            k += 1
            for p in range(j, k, -1):
                self.frequency[p] = self.frequency[p - 1]
                self.son[p] = self.son[p - 1]
            self.frequency[k] = frequency
            self.son[k] = i
            i += 2
            j += 1
        for i in range(TREE):
            child = self.son[i]
            if child >= TREE:
                self.parent[child] = i
            else:
                self.parent[child] = self.parent[child + 1] = i

    def update(self, symbol: int) -> None:
        if self.frequency[ROOT] == 0x8000:
            self.reconstruct()
        i = self.parent[symbol + TREE]
        while True:
            self.frequency[i] += 1
            frequency = self.frequency[i]
            if frequency > self.frequency[i + 1]:
                p = i + 2
                while frequency > self.frequency[p]:
                    p += 1
                other = p - 1
                self.frequency[i] = self.frequency[other]
                self.frequency[other] = frequency
                child = self.son[i]
                self.parent[child] = other
                if child < TREE:
                    self.parent[child + 1] = other
                other_child = self.son[other]
                self.son[other] = child
                self.parent[other_child] = i
                if other_child < TREE:
                    self.parent[other_child + 1] = i
                self.son[i] = other_child
                i = other
            i = self.parent[i]
            if i == 0:
                return

    def encode(self, symbol: int, writer: BitWriter) -> None:
        # In this tree layout the leaf's parent slot is also its symbol index;
        # the code consists of the path from that slot to ROOT.
        node = self.parent[symbol + TREE]
        bits = []
        while node != ROOT:
            parent = self.parent[node]
            bits.append(node - self.son[parent])
            node = parent
        for bit in reversed(bits):
            writer.write(bit, 1)
        self.update(symbol)

    def decode(self, reader: BitReader) -> int:
        node = self.son[ROOT]
        while node < TREE:
            node += reader.read_bit()
            node = self.son[node]
        symbol = node - TREE
        self.update(symbol)
        return symbol


class MatchTree:
    """LZHUF binary tree with DAZZLE's folded second/third-byte root hash."""

    def __init__(self, dictionary: bytearray):
        self.dictionary = dictionary
        self.left = [NIL] * (WINDOW + 1)
        self.parent = [NIL] * (WINDOW + 1)
        self.right = [NIL] * (WINDOW * 2 + 1)
        self.match_position = 0
        self.match_length = 0

    def delete(self, node: int) -> None:
        if self.parent[node] == NIL:
            return
        if self.right[node] == NIL:
            child = self.left[node]
        elif self.left[node] == NIL:
            child = self.right[node]
        else:
            child = self.left[node]
            if self.right[child] != NIL:
                while self.right[child] != NIL:
                    child = self.right[child]
                self.right[self.parent[child]] = self.left[child]
                self.parent[self.left[child]] = self.parent[child]
                self.left[child] = self.left[node]
                self.parent[self.left[node]] = child
            self.right[child] = self.right[node]
            self.parent[self.right[node]] = child
        self.parent[child] = self.parent[node]
        if self.right[self.parent[node]] == node:
            self.right[self.parent[node]] = child
        else:
            self.left[self.parent[node]] = child
        self.parent[node] = NIL

    def insert(self, node: int) -> tuple[int, int]:
        key = node
        p = WINDOW + 1 + self.dictionary[key]
        common = self.dictionary[key + 1] ^ self.dictionary[key + 2]
        common ^= common >> 4
        p += (common & 15) << 8
        self.right[node] = self.left[node] = NIL
        self.match_length = 0
        cmp = 1
        while True:
            if cmp >= 0:
                if self.right[p] != NIL:
                    p = self.right[p]
                else:
                    self.right[p] = node
                    self.parent[node] = p
                    return self.match_position, self.match_length
            else:
                if self.left[p] != NIL:
                    p = self.left[p]
                else:
                    self.left[p] = node
                    self.parent[node] = p
                    return self.match_position, self.match_length
            for common in range(1, LOOKAHEAD):
                cmp = self.dictionary[key + common] - self.dictionary[p + common]
                if cmp:
                    break
            else:
                common = LOOKAHEAD
            if common > THRESHOLD:
                if common > self.match_length:
                    self.match_position = ((node - p) & MASK) - 1
                    self.match_length = common
                    if common >= LOOKAHEAD:
                        break
                if common == self.match_length:
                    distance = ((node - p) & MASK) - 1
                    if distance < self.match_position:
                        self.match_position = distance
        self.parent[node] = self.parent[p]
        self.left[node] = self.left[p]
        self.right[node] = self.right[p]
        self.parent[self.left[p]] = node
        self.parent[self.right[p]] = node
        if self.right[self.parent[p]] == p:
            self.right[self.parent[p]] = node
        else:
            self.left[self.parent[p]] = node
        self.parent[p] = NIL
        return self.match_position, self.match_length


def decode(compressed: bytes, expanded_size: int) -> bytes:
    """Decode a DAZZLE raw stream using its externally stored output length."""
    if expanded_size < 0:
        raise ValueError("expanded size must be nonnegative")
    reader = BitReader(compressed)
    huffman = AdaptiveHuffman()
    dictionary = bytearray(WINDOW + LOOKAHEAD - 1)
    dictionary[:WINDOW - LOOKAHEAD] = b" " * (WINDOW - LOOKAHEAD)
    cursor = WINDOW - LOOKAHEAD
    output = bytearray()
    while len(output) < expanded_size:
        symbol = huffman.decode(reader)
        if symbol < 256:
            value = symbol
            output.append(value)
            dictionary[cursor] = value
            cursor = (cursor + 1) & MASK
        else:
            # DAZZLE reads an aligned eight-bit prefix window, even when it
            # begins partway through the current physical byte.
            first_byte = reader.read_bits(8)
            position_class = _POSITION_CLASS[first_byte]
            prefix_length = _POSITION_LENGTH[first_byte]
            extra = prefix_length - 2
            offset = position_class << 6
            offset |= ((first_byte << extra) | reader.read_bits(extra)) & 0x3F
            source = (cursor - offset - 1) & MASK
            count = min(symbol - 255 + THRESHOLD, expanded_size - len(output))
            for _ in range(count):
                value = dictionary[source]
                source = (source + 1) & MASK
                output.append(value)
                dictionary[cursor] = value
                cursor = (cursor + 1) & MASK
    return bytes(output)


def encode(data: bytes) -> bytes:
    """Encode using DAZZLE's match tree, tie breaks and adaptive Huffman."""
    if not data:
        return b""
    dictionary = bytearray(WINDOW + LOOKAHEAD - 1)
    dictionary[:WINDOW - LOOKAHEAD] = b" " * (WINDOW - LOOKAHEAD)
    source_offset = 0
    cursor = WINDOW - LOOKAHEAD
    lookahead = min(LOOKAHEAD, len(data))
    for i in range(lookahead):
        dictionary[cursor + i] = data[i]
    source_offset = lookahead

    tree = MatchTree(dictionary)
    for i in range(1, LOOKAHEAD + 1):
        tree.insert(cursor - i)
    tree.insert(cursor)

    huffman = AdaptiveHuffman()
    writer = BitWriter()
    while lookahead > 0:
        match_position, match_length = tree.match_position, tree.match_length
        if match_length > lookahead:
            match_length = lookahead
        if match_length <= THRESHOLD:
            match_length = 1
            huffman.encode(dictionary[cursor], writer)
        else:
            huffman.encode(255 - THRESHOLD + match_length, writer)
            position_class = match_position >> 6
            prefix, prefix_length = _position_prefix(position_class)
            writer.write(prefix, prefix_length)
            writer.write(match_position & 0x3F, 6)

        consumed = 0
        while consumed < match_length and source_offset < len(data):
            value = data[source_offset]
            source_offset += 1
            # The old source node leaves the tree before its byte is replaced.
            slot = (cursor - (WINDOW - LOOKAHEAD)) & MASK
            tree.delete(slot)
            dictionary[slot] = value
            if slot < LOOKAHEAD - 1:
                dictionary[slot + WINDOW] = value
            cursor = (cursor + 1) & MASK
            tree.insert(cursor)
            consumed += 1

        while consumed < match_length:
            tree.delete((cursor - (WINDOW - LOOKAHEAD)) & MASK)
            cursor = (cursor + 1) & MASK
            lookahead -= 1
            if lookahead:
                tree.insert(cursor)
            consumed += 1
    return writer.finish()


def _main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    subparsers = parser.add_subparsers(dest="operation", required=True)
    decoder = subparsers.add_parser("decode", help="decode a raw stream")
    decoder.add_argument("input", type=Path)
    decoder.add_argument("output", type=Path)
    decoder.add_argument("--length", type=int, required=True,
                         help="uncompressed byte length stored in DAZZLE's tail")
    encoder = subparsers.add_parser("encode", help="encode a raw stream")
    encoder.add_argument("input", type=Path)
    encoder.add_argument("output", type=Path)
    args = parser.parse_args()
    source = args.input.read_bytes()
    result = decode(source, args.length) if args.operation == "decode" else encode(source)
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_bytes(result)
    print(f"{args.operation}: {len(source)} -> {len(result)} bytes")
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(_main())
    except (OSError, ValueError) as exc:
        print(f"lzhuf.py: {exc}", file=sys.stderr)
        raise SystemExit(2)
