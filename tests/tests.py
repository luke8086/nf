#!/usr/bin/env python3
#
# Copyright (c) 2015 luke8086.
# Distributed under the terms of GPL-2 License.
#

"""
nf test suite
"""

import os
import subprocess
import sys
import unittest

class MainTestCase(unittest.TestCase):

    def _test_one(self, test):
        """Execute a single nf test"""

        nf_path = os.path.join(os.path.dirname(sys.argv[0]), '..', 'build', 'nf')

        proc = subprocess.Popen([nf_path], stdin = subprocess.PIPE,
            stdout = subprocess.PIPE, stderr = subprocess.PIPE)

        code = test.get('in', '')
        exp = test['out']

        #print('testing: ', code)
        out, _ = proc.communicate(input = code.encode())
        out = out.decode()

        if out == exp:
            return

        message = "\n".join((
            "Invalid output",
            "Evaluated code:  %s" % repr(code),
            "Expected output: %s" % repr(exp),
            "Actual output:   %s" % repr(out)
        ))
            
        raise AssertionError(message)

    def _test_many(self, tests):
        """Execute multiple nf tests"""

        for test in tests:
            self._test_one(test)

    def test_stack(self):
        """Test basic stack functions"""

        self._test_many((
            { 'in': '1 2 3 .s', 'out': '1 2 3\n', },
            { 'in': '1 2 dup .s', 'out': '1 2 2\n', },
            { 'in': '1 2 drop .s', 'out': '1\n', },
            { 'in': '1 2 3 swap .s', 'out': '1 3 2\n', },
            { 'in': '1 2 3 over .s', 'out': '1 2 3 2\n', },
            { 'in': '1 2 3 rot .s', 'out': '2 3 1\n', },
        ))

    def test_arith(self):
        """Test arithmetic operators"""

        self._test_many((
            { 'in': '5 2 + .s', 'out': '7\n' },
            { 'in': '5 2 - .s', 'out': '3\n' },
            { 'in': '5 2 * .s', 'out': '10\n' },
            { 'in': '5 2 / .s', 'out': '2\n' },
            { 'in': '5 2 % .s', 'out': '1\n' },
        ))

    def test_bool(self):
        """Test boolean operators"""

        self._test_many((
            { 'in': '5 ! .s', 'out': '0\n' },
            { 'in': '0 ! .s', 'out': '1\n' },
            { 'in': '3 5 && .s', 'out': '1\n' },
            { 'in': '3 0 && .s', 'out': '0\n' },
            { 'in': '0 0 && .s', 'out': '0\n' },
            { 'in': '3 5 || .s', 'out': '1\n' },
            { 'in': '0 5 || .s', 'out': '1\n' },
            { 'in': '0 0 || .s', 'out': '0\n' },
        ))

    def test_bitwise(self):
        """Test bitwise operators"""

        self._test_many((
            { 'in': '3 5 & .s', 'out': '1\n' },
            { 'in': '3 5 | .s', 'out': '7\n' },
            { 'in': '3 5 ^ .s', 'out': '6\n' },
            { 'in': '5 ~ 15 & .s', 'out': '10\n' },
        ))

    def test_comparison(self):
        """Test comparison operators"""

        self._test_many((
            { 'in': '3 4 == .s', 'out': '0\n' },
            { 'in': '4 4 == .s', 'out': '1\n' },
            { 'in': '3 5 != .s', 'out': '1\n' },
            { 'in': '3 3 != .s', 'out': '0\n' },
            { 'in': '3 5 < .s', 'out': '1\n' },
            { 'in': '3 3 < .s', 'out': '0\n' },
            { 'in': '3 2 < .s', 'out': '0\n' },
            { 'in': '3 2 > .s', 'out': '1\n' },
            { 'in': '3 3 > .s', 'out': '0\n' },
            { 'in': '3 5 > .s', 'out': '0\n' },
            { 'in': '3 5 <= .s', 'out': '1\n' },
            { 'in': '3 3 <= .s', 'out': '1\n' },
            { 'in': '3 2 <= .s', 'out': '0\n' },
            { 'in': '3 2 >= .s', 'out': '1\n' },
            { 'in': '3 3 >= .s', 'out': '1\n' },
            { 'in': '3 5 >= .s', 'out': '0\n' },
        ))

    def test_compiler(self):
        """Test function compiler"""

        self._test_many((
            {
                'in': ': 2 + ; 5 exec .s exec .s',
                'out': '7\n9\n'
            },
            {
                'in': ': 2 + ; "add-two" def .s 5 add-two add-two .s',
                'out': '\n9\n'
            },
            {
                'in': ': 2 + ; "x" def 5 x .s drop ' \
                      ': 10 + ; "x" def 5 x .s',
                'out': '7\n15\n'
            },
        ))

    def test_cond(self):
        """Test conditional expressions"""

        self._test_many((
            {
                'in': ': 5 if 7 8 9 else 10 then 99 100 ; exec .s',
                'out': '7 8 9 99 100\n'
            },
            {
                'in': ': 0 if 8 else 10 20 30 then 99 100 ; exec .s',
                'out': '10 20 30 99 100\n'
            },
            {
                'in': '5 if 7 8 9 else 10 then 99 100 .s',
                'out': '7 8 9 99 100\n'
            },
            {
                'in': '0 if 8 else 10 20 30 then 99 100 .s',
                'out': '10 20 30 99 100\n'
            },
            {
                'in': ': 2 == if 1 then 10 ; "one-if-two" def ' \
                      '2 one-if-two .s 5 one-if-two .s',
                'out': '1 10\n1 10 10\n'
            },
            {
                'in': ': 2 % ! if 1 else 0 then ; "even" def ' \
                      '4 even 5 even 6 even 7 even .s',
                'out': '1 0 1 0\n'
            }
        ))

    def test_loops(self):
        """Test loop expressions"""

        self._test_many([
            {
                'in': ': 5 do dup while .s 1 - repeat ; exec',
                'out': '5\n4\n3\n2\n1\n',
            },
            {
                'in': '5 do dup while .s 1 - repeat',
                'out': '5\n4\n3\n2\n1\n',
            },
            {
                'in': ': 0 do .s 1 + dup 5 == until ; exec',
                'out': '0\n1\n2\n3\n4\n',
            },
            {
                'in': '0 do .s 1 + dup 5 == until',
                'out': '0\n1\n2\n3\n4\n',
            }
        ])

if __name__ == '__main__':
    unittest.main()
