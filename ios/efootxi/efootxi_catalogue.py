"""Restore the shipped league pools hidden by the weekly special-pool rollout.

Only the known 7000..7029 managed pools with real members are re-enabled.
Consumed cards, draw receipts, account state and retired weekly pools survive.
"""
import copy
import json
from pathlib import Path
import sqlite3

REVISION = 'ios-league-catalogue-v1'
LABELS = {7000: 'Arsenal - All Stars'}
for first, league in ((7001, 'Premier League'), (7006, 'La Liga'), (7010, 'Serie A'),
                      (7014, 'European Clubs'), (7018, 'France & Europe'),
                      (7022, 'World Cup Legends'), (7026, 'Global Clubs')):
    positions = ('FW', 'AMF / LMF / RMF', 'CMF / DMF', 'DF', 'GK') if first == 7001 else ('FW', 'MF', 'DF', 'GK')
    LABELS.update({first+i: league+' - '+position for i, position in enumerate(positions)})


def restore(connection):
    """Single transaction; repeating this migration does not reset later choices."""
    if connection.execute('SELECT 1 FROM meta WHERE key=?', (REVISION,)).fetchone():
        return []
    rows = connection.execute('''SELECT a.agent_id FROM managed_agents a
        JOIN agents b ON b.agent_id=a.agent_id
        WHERE a.agent_id BETWEEN 7000 AND 7029 AND a.mode='LEAGUE_POSITION'
        AND EXISTS (SELECT 1 FROM managed_members m JOIN managed_cards c ON c.card_id=m.card_id
                    WHERE m.agent_id=a.agent_id AND m.weight>0)
        ORDER BY a.agent_id''').fetchall()
    ids = [r[0] for r in rows]
    if not ids:
        return []
    def meta(key, default):
        row = connection.execute('SELECT value FROM meta WHERE key=?', (key,)).fetchone()
        return json.loads(row[0]) if row else default
    active = meta('league_position_agent_ids', [])
    prices = meta('agent_price_list', [])
    # Reuse a real native purchase schema, never a fabricated captured reward.
    template = next((p for p in prices if p.get('agent_id') in active and p.get('price_list')), None)
    if template is None:
        raise ValueError('No valid agent price template for restored pools')
    with connection:
        for aid in ids:
            connection.execute('UPDATE managed_agents SET enabled=1 WHERE agent_id=?', (aid,))
            if aid not in active:
                active.append(aid)
            if not any(p.get('agent_id') == aid for p in prices):
                price = copy.deepcopy(template)
                price.update(agent_id=aid, discount_limit_count=0, discount_use_count=0)
                prices.append(price)
        for key, value in (('league_position_agent_ids', active), ('agent_price_list', prices), (REVISION, ids)):
            connection.execute('INSERT OR REPLACE INTO meta VALUES(?,?)', (key, json.dumps(value)))
    return ids


def install(target):
    database = Path(target) / 'outputs/efootball/local-club.sqlite3'
    # Open only an existing gateway database; never create an empty account.
    with sqlite3.connect(database.as_uri()+'?mode=rw', uri=True) as connection:
        if connection.execute('SELECT 1 FROM meta WHERE key=?', (REVISION,)).fetchone():
            return []
        backup = database.with_name('before-ios-league-catalogue.sqlite3')
        if not backup.exists():
            with sqlite3.connect(backup) as destination:
                connection.backup(destination)
        return restore(connection)


def active_count(target):
    database = Path(target) / 'outputs/efootball/local-club.sqlite3'
    with sqlite3.connect(database.as_uri()+'?mode=ro', uri=True) as connection:
        row = connection.execute("SELECT value FROM meta WHERE key='league_position_agent_ids'").fetchone()
        return len(json.loads(row[0])) if row else None
