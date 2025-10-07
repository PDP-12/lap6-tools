#!/usr/bin/env python3

import re
import sys

memsize = 2048

symtab = {
    "u":   0o0010,
    "i":   0o0020,
    "&":   0o0020,
    "HLT": 0o0000,
    "ZTA": 0o0005,
    "CLR": 0o0011,
    "DIN": 0o0012,
    "ATR": 0o0014,
    "RTA": 0o0015,
    "NOP": 0o0016,
    "COM": 0o0017,
    "SET": 0o0040,
    "SAM": 0o0100,
    "DIS": 0o0140,
    "XSK": 0o0200,
    "ROL": 0o0240,
    "ROR": 0o0300,
    "SCR": 0o0340,
    "SXL": 0o0400,
    "KST": 0o0415,
    "SNS": 0o0440,
    "AZE": 0o0450,
    "APO": 0o0451,
    "LZE": 0o0452,
    "IBZ": 0o0453,
    "OVF": 0o0454,
    "ZZZ": 0o0455,
    "OPR": 0o0500,
    "KBD": 0o0515,
    "RSW": 0o0516,
    "LSW": 0o0517,
    "LMB": 0o0600,
    "UMB": 0o0640,
    "RDC": 0o0700,
    "RCG": 0o0701,
    "RDE": 0o0702,
    "MTB": 0o0703,
    "WRC": 0o0704,
    "WCG": 0o0705,
    "WRI": 0o0706,
    "CHK": 0o0707,
    "LDA": 0o1000,
    "STA": 0o1040,
    "ADA": 0o1100,
    "ADM": 0o1140,
    "LAM": 0o1200,
    "MUL": 0o1240,
    "LDH": 0o1300,
    "STH": 0o1340,
    "SHD": 0o1400,
    "SAE": 0o1440,
    "SRO": 0o1500,
    "BCL": 0o1540,
    "BSE": 0o1600,
    "BCO": 0o1640,
    "DSC": 0o1740,
    "ADD": 0o2000,
    "STC": 0o4000,
    "JMP": 0o6000
}


chartab = {
    "0": 0o00,
    "1": 0o01,
    "2": 0o02,
    "3": 0o03,
    "4": 0o04,
    "5": 0o05,
    "6": 0o06,
    "7": 0o07,
    "8": 0o10,
    "9": 0o11,
    "\n": 0o12, "\r": 0o12,
    "\b": 0o13,
    " ": 0o14,
    "i": 0o15,  "&": 0o15,
    "p": 0o16,  "'": 0o16,
    "-": 0o17,
    "+": 0o20,
    "|": 0o21,  "\\": 0o21,
    "#": 0o22,
    # CASE 0o23
    "A": 0o24,
    "B": 0o25,
    "C": 0o26,
    "D": 0o27,
    "E": 0o30,
    "F": 0o31,
    "G": 0o32,
    "H": 0o33,
    "I": 0o34,
    "J": 0o35,
    "K": 0o36,
    "L": 0o37,
    "M": 0o40,
    "N": 0o41,
    "O": 0o42,
    "P": 0o43,
    "Q": 0o44,
    "R": 0o45,
    "S": 0o46,
    "T": 0o47,
    "U": 0o50,
    "V": 0o51,
    "W": 0o52,
    "X": 0o53,
    "Y": 0o54,
    "Z": 0o55,
    # META 0o56
    "→": 0o57,
    "?": 0o60,
    "=": 0o61,
    "u": 0o62,  "%": 0o62,
    ",": 0o63,
    ".": 0o64,
    "⊟": 0o65,  "@": 0o65,  "$": 0o65,
    "[": 0o66,
    "_": 0o67,  #@
    "\"": 0o70, "^": 0o70,
    "„": 0o71,  ";": 0o71,
    "<": 0o72,
    ">": 0o73,
    "]": 0o74,
    "ˣ": 0o75,  "*": 0o75,
    ":": 0o76,
    "ʸ": 0o77,  "y": 0o77
}

def debug(message):
    yield
    print(message)


def pee():
    global labels
    return labels["p"]


def zero(symbol):
    return 0


def error(symbol):
    raise Exception(f"Undefined symbol: {symbol}")


def core(value):
    global image, labels
    if value is None:
        raise Exception(f"No value for {pee():04o}")
    if image:
        debug(f"Store {value:04o} at {pee():04o}")
        image[pee()] = value & 0o7777
    labels["p"] = pee() + 1
    return None


def string(data, store):
    if len(data) == 0:
        return None
    if len(data) == 1:
        return store((chartab[data] << 6) | chartab[" "])
    store((chartab[data[0:1]] << 6) | chartab[data[1:2]])
    return string(data[2:], store)


def foo(value):
    return value


def sum(x1, x2):
    sum = x1 + x2
    sum = sum + (sum >> 12)
    return sum & 0o7777


def negate(x):
    return x ^ 0o7777


def parse(line, store=foo):
    global undefined
    line = re.sub(r'\[.*$', '', line)

    m = re.compile(r'^([^[]*)\[(.*)').match(line)
    if m:
        parse(m.group(1), store)
        parse(m.group(2), store)
        return None

    line = line.strip()
    if line == "":
        return None

    m = re.compile(r'^[ \t]*[⊟@$]([0-7]+)').match(line)
    if m:
        labels["p"] = int(m.group(1), 8)
        debug(f"address {pee():04o}")
        return None
    m = re.compile(r'^#([0-9][A-Z])(.*)').match(line)
    if m:
        label = m.group(1)
        debug(f"label {label} = {pee():04o}")
        labels[label] = pee()
        oldpee = pee()
        parse(m.group(2), core)
        if pee() == oldpee:
            core(0)
        return None
    m = re.compile(r'^([0-9][A-Z])=(.*)').match(line)
    if m:
        symbol = m.group(1)
        debug(f"symbol {symbol} = {m.group(2)}")
        value = parse(m.group(2))
        symtab[symbol] = value
        return None
    m = re.compile(r'^([^ "]+)[ \t]+(.*)').match(line)
    if m:
        value = parse(m.group(1))
        debug(f"composition {m.group(1)}={value:04o} with {m.group(2)}")
        return store(value | parse(m.group(2)))

    m = re.compile(r'^-(.*)').match(line)
    if m:
        debug(f"negate {m.group(1)}")
        value = negate(parse(m.group(1)))
        debug(f"negate {value}")
        return store(value)
    m = re.compile(r'^([^+]*)\+(.*)').match(line)
    if m:
        debug(f"addition {m.group(1)} {m.group(2)}")
        value = sum(parse(m.group(1)), parse(m.group(2)))
        return store(value)
    m = re.compile(r'^([^-]*)-(.*)').match(line)
    if m:
        debug(f"subtraction {m.group(1)} {m.group(2)}")
        value = sum(parse(m.group(1)), negate(parse(m.group(2))))
        return store(value)
    m = re.compile(r'^([^|\\]*)\|(.*)').match(line)
    if m:
        value = (parse(m.group(1)) << 9) | parse(m.group(2))
        return store(value)
    m = re.compile(r'^([^;]*);(.*)').match(line)
    if m:
        value = (parse(m.group(1)) << 11) | parse(m.group(2))
        return store(value)
    m = re.compile(r'^"([^„]*)„').match(line)
    if m:
        return string(m.group(1), store)
    m = re.compile(r'^([0-7]+)([^A-Z]|$)').match(line)
    if m:
        number = int(m.group(1), 8)
        debug(f"number {number}")
        return store(number)
    m = re.compile(r'^([0-9][A-Z]|[A-Z][A-Z][A-Z]|[uip&])').match(line)
    if m:
        symbol = m.group(1)
        if symbol in symtab:
            value = symtab[symbol]
            debug(f"symbol {symbol} is {value:04o}")
            return store(value)
        elif symbol in labels:
            value = labels[symbol]
            debug(f"label {symbol} is {value:04o}")
            return store(value)
        else:
            store(0)
            return undefined(symbol)
    raise Exception(f"Syntax error: {line}")


if __name__ == "__main__":
    global undefined, image, labels
    debug("PASS 1")
    labels = {}
    labels["p"] = 0
    undefined = zero
    image = None
    with open(sys.argv[1]) as f:
        for line in f:
            parse(line, core)
    debug("PASS 2")
    labels["p"] = 0
    undefined = error
    image = [None] * memsize
    with open(sys.argv[1]) as f:
        for line in f:
            parse(line, core)
    for i in range(memsize):
        if image[i] is not None:
            print(f"{i:04o} {image[i]:04o}")
