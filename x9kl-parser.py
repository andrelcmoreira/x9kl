from argparse import ArgumentParser
from sys import argv
from enum import Enum

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
    KEY_D = 42
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


#'''
#{KEY_COMMA, {',', '<', '\0', handle_key}},
#{KEY_DOT, {'.', '>', '\0', handle_key}},
#{KEY_MINUS, {'-', '_', '\0', handle_key}},
#{KEY_SEMICOLON, {';', ':', '\0', handle_key}},
#{KEY_EQUAL, {'=', '+', '\0', handle_key}},
#{KEY_SPACE, {' ', '\0', '\0', handle_key}},
#{KEY_ENTER, {'\0', '\0', '\0', handle_enter}},
#{KEY_BACKSPACE, {'\0', '\0', '\0', handle_backspace}},
#{KEY_CAPSLOCK, {'\0', '\0', '\0', handle_capslock}},
#{KEY_DELETE, {'\0', '\0', '\0', handle_delete}},
#{KEY_RIGHT, {'\0', '\0', '\0', handle_arrow}},
#{KEY_LEFT, {'\0', '\0', '\0', handle_arrow}},
#{KEY_LEFTSHIFT, {'\0', '\0', '\0', handle_shift}},
#{KEY_RO, {'/', '?', '\0', handle_key}},
#{KEY_GRAVE, {'\'', '\"', '\0', handle_key}},
#{KEY_102ND, {'\\', '|', '\0', handle_key}},
#{KEY_RIGHTBRACE, {'[', '{', '\0', handle_key}},
#{KEY_RIGHTALT, {'\0', '\0', '\0', handle_altgr}},
#{KEY_BACKSLASH, {']', '}', '\0', handle_key}}};
#'''
KEYMAP = {
    KeyCode.KEY_ENTER.value: '\n',
    KeyCode.KEY_A.value: 'a',
    KeyCode.KEY_B.value: 'b',
    KeyCode.KEY_C.value: 'c',
    KeyCode.KEY_D.value: 'd',
    KeyCode.KEY_E.value: 'e',
    KeyCode.KEY_F.value: 'f',
    KeyCode.KEY_G.value: 'g',
    KeyCode.KEY_H.value: 'h',
    KeyCode.KEY_I.value: 'i',
    KeyCode.KEY_J.value: 'j',
    KeyCode.KEY_K.value: 'k',
    KeyCode.KEY_L.value: 'l',
    KeyCode.KEY_L.value: 'l',
    KeyCode.KEY_M.value: 'm',
    KeyCode.KEY_N.value: 'n',
    KeyCode.KEY_O.value: 'o',
    KeyCode.KEY_P.value: 'p',
    KeyCode.KEY_Q.value: 'q',
    KeyCode.KEY_R.value: 'r',
    KeyCode.KEY_S.value: 's',
    KeyCode.KEY_T.value: 't',
    KeyCode.KEY_U.value: 'u',
    KeyCode.KEY_V.value: 'v',
    KeyCode.KEY_W.value: 'w',
    KeyCode.KEY_X.value: 'x',
    KeyCode.KEY_Y.value: 'y',
    KeyCode.KEY_Z.value: 'z',
    KeyCode.KEY_SPACE.value: ' ',
    KeyCode.KEY_MINUS.value: '-'
}


class LogEntry:

    def __init__(self, date, keys):
        self.date = date
        self.keys = keys

    def __str__(self):
        s = '[%02d:%02d:%02d] ' % (self.date[0], self.date[1], self.date[2])

        for i in range(0, len(self.keys)):
            if (i % 2) == 0:
                pass # TODO: read flags
            else:
                s += KEYMAP[self.keys[i]]

        return s


def sort_keys(keys):
    sorted_keys = []

    for i in range(0, len(keys)):
        if (i % 2) == 0:
            sorted_keys.insert(i + 1, keys[i])
        else:
            sorted_keys.insert(i - 1, keys[i])

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
