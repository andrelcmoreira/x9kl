from argparse import ArgumentParser
from sys import argv
from enum import Enum

# https://github.com/torvalds/linux/blob/master/include/uapi/linux/input-event-codes.h
class KeyCode(Enum):
    KEY_0 = 11
    KEY_1 = 2
    KEY_2 = 3
    KEY_3 = 4
    KEY_4 = 5
    KEY_5 = 6
    KEY_6 = 7
    KEY_7 = 8
    KEY_8 = 9
    KEY_9 = 10
    KEY_A = 30
    KEY_B = 48
    KEY_C = 46
    KEY_D = 32
    KEY_E = 18
    KEY_F = 33
    KEY_G = 34
    KEY_H = 35
    KEY_I = 23
    KEY_J = 36
    KEY_K = 37
    KEY_L = 38
    KEY_M = 50
    KEY_N = 49
    KEY_O = 24
    KEY_P = 25
    KEY_Q = 16
    KEY_R = 19
    KEY_S = 31
    KEY_T = 20
    KEY_U = 22
    KEY_V = 47
    KEY_W = 17
    KEY_X = 45
    KEY_Y = 21
    KEY_Z = 44
    KEY_ENTER = 28
    KEY_SPACE = 57
    KEY_MINUS = 12
    KEY_COMMA = 51
    KEY_DOT = 52
    KEY_SEMICOLON = 39
    KEY_EQUAL = 117
    KEY_RO = 89
    KEY_GRAVE = 41
    KEY_102ND = 86
    KEY_RIGHTBRACE = 27
    KEY_BACKSLASH = 43


# 'n' = normal
# 's' = shift
# 'c' = capslock
# 'a' = altgr
KEYMAP = {
    KeyCode.KEY_0.value: { 'n': '0', 's': ')', 'c': '0', 'a': '' },
    KeyCode.KEY_1.value: { 'n': '1', 's': '!', 'c': '1', 'a': '' },
    KeyCode.KEY_2.value: { 'n': '2', 's': '@', 'c': '2', 'a': '' },
    KeyCode.KEY_3.value: { 'n': '3', 's': '#', 'c': '3', 'a': '' },
    KeyCode.KEY_4.value: { 'n': '4', 's': '$', 'c': '4', 'a': '' },
    KeyCode.KEY_5.value: { 'n': '5', 's': '%', 'c': '5', 'a': '' },
    KeyCode.KEY_6.value: { 'n': '6', 's': ' ', 'c': '6', 'a': '' },
    KeyCode.KEY_7.value: { 'n': '7', 's': '&', 'c': '7', 'a': '' },
    KeyCode.KEY_8.value: { 'n': '8', 's': '*', 'c': '8', 'a': '' },
    KeyCode.KEY_9.value: { 'n': '9', 's': '(', 'c': '9', 'a': '' },
    KeyCode.KEY_A.value: { 'n': 'a', 's': 'A', 'c': 'A', 'a': '' },
    KeyCode.KEY_B.value: { 'n': 'b', 's': 'B', 'c': 'B', 'a': '' },
    KeyCode.KEY_C.value: { 'n': 'c', 's': 'C', 'c': 'C', 'a': '' },
    KeyCode.KEY_D.value: { 'n': 'd', 's': 'D', 'c': 'D', 'a': '' },
    KeyCode.KEY_E.value: { 'n': 'e', 's': 'E', 'c': 'E', 'a': '' },
    KeyCode.KEY_F.value: { 'n': 'f', 's': 'F', 'c': 'F', 'a': '' },
    KeyCode.KEY_G.value: { 'n': 'g', 's': 'G', 'c': 'G', 'a': '' },
    KeyCode.KEY_H.value: { 'n': 'h', 's': 'H', 'c': 'H', 'a': '' },
    KeyCode.KEY_I.value: { 'n': 'i', 's': 'I', 'c': 'I', 'a': '' },
    KeyCode.KEY_J.value: { 'n': 'j', 's': 'J', 'c': 'J', 'a': '' },
    KeyCode.KEY_K.value: { 'n': 'k', 's': 'K', 'c': 'K', 'a': '' },
    KeyCode.KEY_L.value: { 'n': 'l', 's': 'L', 'c': 'L', 'a': '' },
    KeyCode.KEY_M.value: { 'n': 'm', 's': 'M', 'c': 'M', 'a': '' },
    KeyCode.KEY_N.value: { 'n': 'n', 's': 'N', 'c': 'N', 'a': '' },
    KeyCode.KEY_O.value: { 'n': 'o', 's': 'O', 'c': 'O', 'a': '' },
    KeyCode.KEY_P.value: { 'n': 'p', 's': 'P', 'c': 'P', 'a': '' },
    KeyCode.KEY_Q.value: { 'n': 'q', 's': 'Q', 'c': 'Q', 'a': '/' },
    KeyCode.KEY_R.value: { 'n': 'r', 's': 'R', 'c': 'R', 'a': '' },
    KeyCode.KEY_S.value: { 'n': 's', 's': 'S', 'c': 'S', 'a': '' },
    KeyCode.KEY_T.value: { 'n': 't', 's': 'T', 'c': 'T', 'a': '' },
    KeyCode.KEY_U.value: { 'n': 'u', 's': 'U', 'c': 'U', 'a': '' },
    KeyCode.KEY_V.value: { 'n': 'v', 's': 'V', 'c': 'V', 'a': '' },
    KeyCode.KEY_W.value: { 'n': 'w', 's': 'W', 'c': 'W', 'a': '?' },
    KeyCode.KEY_X.value: { 'n': 'x', 's': 'X', 'c': 'X', 'a': '' },
    KeyCode.KEY_Y.value: { 'n': 'y', 's': 'Y', 'c': 'Y', 'a': '' },
    KeyCode.KEY_Z.value: { 'n': 'z', 's': 'Z', 'c': 'Z', 'a': '' },
    KeyCode.KEY_SPACE.value: { 'n': ' ', 's': ' ', 'c': ' ', 'a': '' },
    KeyCode.KEY_MINUS.value: { 'n': '-', 's': '_', 'c': '-', 'a': '' },
    KeyCode.KEY_ENTER.value: { 'n': '\n', 's': '\n', 'c': '\n', 'a': '\n'},
    KeyCode.KEY_COMMA.value: { 'n': ',', 's': '<', 'c': ',', 'a': '' },
    KeyCode.KEY_DOT.value: { 'n': '.', 's': '>', 'c': '.', 'a': '' },
    KeyCode.KEY_SEMICOLON.value: { 'n': ';', 's': ':', 'c': ';', 'a': '' },
    KeyCode.KEY_EQUAL.value: { 'n': '=', 's': '+', 'c': '=', 'a': '' },
    KeyCode.KEY_RO.value: { 'n': '/', 's': '?', 'c': '/', 'a': '' },
    KeyCode.KEY_GRAVE.value: { 'n': '\'', 's': '"', 'c': '\'', 'a': '' },
    KeyCode.KEY_102ND.value: { 'n': '\\', 's': '|', 'c': '\\', 'a': '' },
    KeyCode.KEY_RIGHTBRACE.value: { 'n': '[', 's': '{', 'c': '[', 'a': '' },
    KeyCode.KEY_BACKSLASH.value: { 'n': ']', 's': '}', 'c': ']', 'a': '' }
}


CAPS_MASK = 0x01
SHIFT_MASK = 0x02
ALTGR_MASK = 0x04


class LogEntry:

    def __init__(self, date, keys):
        self.date = date
        self.keys = keys

    def __str__(self):
        s = '[%02d:%02d:%02d] ' % (self.date[0], self.date[1], self.date[2])

        for i in range(0, len(self.keys) - 1, 2):
            flags = self.keys[i]
            key = self.keys[i + 1]

            if flags & CAPS_MASK: s += KEYMAP[key]['c']
            elif flags & SHIFT_MASK: s += KEYMAP[key]['s']
            elif flags & ALTGR_MASK: s += KEYMAP[key]['a']
            else: s += KEYMAP[key]['n']

        return s


def sort_keys(keys):
    sorted_keys = []

    for i in range(0, len(keys), 2):
        sorted_keys.append(keys[i+1])
        sorted_keys.append(keys[i])

    return sorted_keys


def parse_keys(log_file):
    with open(log_file, 'rb') as f:
        content = f.read()
        keys = sort_keys(list(content))

        buffer = []
        for key in keys:
            if key == KeyCode.KEY_ENTER.value:
                log_entry = LogEntry(buffer[0:3], buffer[4:])
                buffer.clear()
                yield log_entry
            else:
                buffer.append(key)


def main(log_file):
    for key in parse_keys(log_file):
        print(key)


def parse_args():
    parser = ArgumentParser(prog=argv[0])

    parser.add_argument('-f', '--log-file', help='file containing x9kl logs')

    # no arguments provided
    if not len(argv):
        parser.print_help()
        return None

    return parser.parse_args()


if __name__ == "__main__":
    args = parse_args()

    if args:
        main(args.log_file)
