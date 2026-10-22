import importlib.util
import json
from pathlib import Path
import sqlite3
import unittest

spec = importlib.util.spec_from_file_location('catalogue', Path(__file__).resolve().parents[1]/'ios/efootxi/efootxi_catalogue.py')
catalogue = importlib.util.module_from_spec(spec)
spec.loader.exec_module(catalogue)


class CatalogueTests(unittest.TestCase):
    def fixture(self):
        c = sqlite3.connect(':memory:')
        self.addCleanup(c.close)
        c.executescript('''CREATE TABLE meta(key TEXT PRIMARY KEY,value TEXT);
            CREATE TABLE agents(agent_id INTEGER PRIMARY KEY,json TEXT);
            CREATE TABLE managed_agents(agent_id INTEGER PRIMARY KEY,name TEXT,enabled INTEGER,replacement INTEGER,mode TEXT);
            CREATE TABLE managed_cards(card_id TEXT PRIMARY KEY,payload TEXT);
            CREATE TABLE managed_members(agent_id INTEGER,card_id TEXT,weight INTEGER);
            CREATE TABLE managed_consumed(agent_id INTEGER,card_id TEXT);
            CREATE TABLE managed_draw_receipts(rqid INTEGER,result TEXT);''')
        c.executemany('INSERT INTO meta VALUES(?,?)',[
            ('league_position_agent_ids','[91000]'),
            ('agent_price_list',json.dumps([{'agent_id':91000,'price_list':[{'multiple_count':1,'price':{'coin_price':100}}]}]))])
        for aid in (7000,7001,7002,91000,91008):
            c.execute('INSERT INTO agents VALUES(?,?)',(aid,'{}'))
            c.execute('INSERT INTO managed_agents VALUES(?,?,?,0,?)',(aid,str(aid),int(aid==91000),'LEAGUE_POSITION'))
        c.execute("INSERT INTO managed_cards VALUES('123','{}')")
        c.executemany("INSERT INTO managed_members VALUES(?,'123',1)",[(7000,),(7001,),(91008,)])
        c.execute("INSERT INTO managed_consumed VALUES(7000,'123')")
        c.execute("INSERT INTO managed_draw_receipts VALUES(100,'original reward')")
        c.commit()
        return c

    def test_restore_preserves_draws_and_retired_weeklies_and_skips_empty(self):
        c = self.fixture()
        self.assertEqual(catalogue.restore(c),[7000,7001])
        self.assertEqual(json.loads(c.execute("SELECT value FROM meta WHERE key='league_position_agent_ids'").fetchone()[0]),[91000,7000,7001])
        self.assertEqual(c.execute('SELECT agent_id FROM managed_agents WHERE enabled=1 ORDER BY agent_id').fetchall(),[(7000,),(7001,),(91000,)])
        self.assertEqual(c.execute('SELECT * FROM managed_consumed').fetchall(),[(7000,'123')])
        self.assertEqual(c.execute('SELECT * FROM managed_draw_receipts').fetchall(),[(100,'original reward')])
        self.assertEqual(len(json.loads(c.execute("SELECT value FROM meta WHERE key='agent_price_list'").fetchone()[0])),3)
        self.assertEqual(catalogue.restore(c),[])

    def test_missing_prices_leaves_catalogue_unchanged(self):
        c = self.fixture()
        c.execute("UPDATE meta SET value='[]' WHERE key='agent_price_list'")
        c.commit()
        with self.assertRaisesRegex(ValueError,'price template'):
            catalogue.restore(c)
        self.assertEqual(c.execute('SELECT enabled FROM managed_agents WHERE agent_id=7000').fetchone()[0],0)


if __name__ == '__main__':
    unittest.main()
