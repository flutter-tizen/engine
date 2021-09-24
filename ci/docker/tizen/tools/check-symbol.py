#!/usr/bin/env python
# Copyright 2021 Samsung Electronics Co., Ltd. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import os
import subprocess
import sys


class Symbol:
    def __init__(self, addr, type, name):
        self.addr = addr
        self.type = type
        self.name = name

    def __str__(self):
        return '{} {} {}'.format(self.addr, self.type, self.name)

    @staticmethod
    def parse(line):
        return Symbol(line[:8].strip(), line[9], line[11:].strip())


def check_symbol(sofile, allowlist):
    if not os.access(sofile, os.R_OK):
        raise Exception('{} is not readable'.format(sofile))
    if not os.access(allowlist, os.R_OK):
        raise Exception('{} is not readable'.format(allowlist))

    try:
        symbols_raw = subprocess.check_output(
            ['nm', '-gDC', sofile]).decode('utf-8').splitlines()
        symbols = [Symbol.parse(line) for line in symbols_raw]
    except subprocess.CalledProcessError as e:
        raise Exception('nm failed: {}'.format(e))

    with open(allowlist, 'r') as f:
        allowlist = [line.strip() for line in f.readlines()]

    not_allowed = []
    for symbol in symbols:
        if symbol.addr:
            continue
        if symbol.name.startswith('FlutterEngine'):
            continue
        if symbol.name in allowlist:
            continue
        not_allowed.append(symbol)

    if not_allowed:
        print('Symbols not allowed ({}):'.format(sofile))
        for symbol in not_allowed:
            print(symbol)
        return False

    return True


def main():
    import optparse
    parser = optparse.OptionParser(usage='%prog [options] [sofile ..]')
    parser.add_option('--allowlist', dest='allowlist',
                      help='Path to the allowlist file')
    (options, args) = parser.parse_args()

    if not options.allowlist:
        print('--allowlist is required')
        return 1
    if not args:
        print('sofile is required')
        return 1

    for sofile in args:
        if not check_symbol(sofile, options.allowlist):
            return 1

    return 0


if __name__ == '__main__':
    sys.exit(main())
