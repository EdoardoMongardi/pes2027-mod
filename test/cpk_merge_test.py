import struct
import sys
import unittest
from pathlib import Path
sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'tools'))
from merge_cpk import encode_utf, read_cpk, merge
from cpk_toc import utf_table


def fixture(entries):
    columns = [('DirName', 10), ('FileName', 10), ('FileSize', 4),
               ('ExtractSize', 4), ('FileOffset', 6), ('ID', 4)]
    rows = []
    data = bytearray()
    for identity, (name, content) in enumerate(entries):
        rows.append(dict(DirName='common', FileName=name, FileSize=len(content),
                         ExtractSize=len(content), FileOffset=2048 + len(data), ID=identity))
        data += content.ljust(2048, b'\0')
    toc = encode_utf('CpkTocInfo', columns, rows)
    header = encode_utf('CpkHeader', [('TocOffset', 6), ('ContentOffset', 6),
                                    ('Align', 2), ('Files', 4), ('ContentSize', 6)],
                        [dict(TocOffset=2048, ContentOffset=4096, Align=2048, Files=len(rows), ContentSize=len(data))])
    result = (b'CPK ' + b'\0' * 4 + struct.pack('<Q', len(header)) + header).ljust(2048, b'\0')
    result += (b'TOC ' + b'\0' * 4 + struct.pack('<Q', len(toc)) + toc).ljust(2048, b'\0')
    return result + data


class CpkMergeTest(unittest.TestCase):
    def test_union_preserves_uniforms_and_uses_new_player_table(self):
        baseline = fixture([('uniform.bin', b'original kit'), ('players.dat', b'old players')])
        overlay = fixture([('players.dat', b'new players'), ('new-kit.bin', b'new kit')])
        result, report = merge(baseline, overlay)
        header, _, _, files = read_cpk(result)
        self.assertEqual(header['Files'], 3)
        self.assertEqual(files['common', 'uniform.bin'][1], b'original kit')
        self.assertEqual(files['common', 'players.dat'][1], b'new players')
        self.assertEqual(files['common', 'new-kit.bin'][1], b'new kit')
        ids = [r['ID'] for r, _ in files.values()]
        self.assertEqual(len(ids), len(set(ids)))
        self.assertEqual(files['common', 'uniform.bin'][0]['ID'], 0)
        self.assertEqual(files['common', 'players.dat'][0]['ID'], 1)
        self.assertEqual(report['baseline_only_files_preserved'], [('common', 'uniform.bin')])

    def test_utf_optional_columns_and_strings_round_trip(self):
        rows = [{'name': 'áo đấu', 'count': 7, 'optional': None},
                {'name': 'Zidane', 'count': 13, 'optional': None}]
        packet = encode_utf('Test', [('name', 10), ('count', 6), ('optional', 4)], rows)
        self.assertEqual(utf_table(packet), rows)


if __name__ == '__main__':
    unittest.main()
