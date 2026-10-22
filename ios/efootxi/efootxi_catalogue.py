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

LABELS.update({
    91000: 'Cầu thủ đặc biệt - Special Players',
    91001: 'Big Time - Siêu sao',
    91002: 'Show Time - Phần 1',
    91003: 'Show Time - Phần 2',
    91004: 'Show Time - Phần 3',
    91005: 'Epic - Phần 1',
    91006: 'Epic - Phần 2',
    91007: 'Epic - Phần 3',
})


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


# Link-up IDs are the last two CoachBooster slots (native CoachLink rows 1..32).
# The special-coach catalogue shipped two coaches whose second link-up pointed at
# an unreleased CoachLink row, which the client renders as "Dummy". Values below
# match the CoachLink centre-piece/key-man roles of the real cards:
#   Conte 16-17:    Over-the-Top Pass C (7, Build Up -> Prolific Winger)
#                   1-2 Cut-in B        (12, Prolific Winger -> Fox in the Box)
#   Mourinho 11-12: Shadow Link A       (17, centre forward -> Classic No. 10)
#                   1-2 Cut-in C        (13, Prolific Winger -> Deep-lying Forward)
COACH_REVISION = 'ios-coach-linkup-v1'
COACH_LINKUPS = {
    17609097478250: ((7, 24), (7, 12)),
    17608560707469: ((17, 28), (17, 13)),
}


def fix_coach_linkups(target):
    """Idempotent: only rewrites the exact known-bad slots, keeps boosters/serials/locks."""
    database = Path(target) / 'outputs/efootball/local-club.sqlite3'
    with sqlite3.connect(database.as_uri()+'?mode=rw', uri=True) as connection:
        if connection.execute('SELECT 1 FROM meta WHERE key=?', (COACH_REVISION,)).fetchone():
            return []
        changed = []
        rows = connection.execute('SELECT coach_id, serial, json FROM coaches WHERE coach_id IN (%s)'
                                  % ','.join('?'*len(COACH_LINKUPS)), tuple(COACH_LINKUPS)).fetchall()
        pending = []
        for coach_id, serial, raw in rows:
            coach = json.loads(raw)
            abilities = coach.get('ability_id')
            bad, good = COACH_LINKUPS[coach_id]
            if isinstance(abilities, list) and len(abilities) == 4 and tuple(abilities[2:]) == bad:
                coach['ability_id'] = abilities[:2] + list(good)
                pending.append((json.dumps(coach, ensure_ascii=False, separators=(',', ':')), coach_id, serial))
                changed.append({'coach_id': coach_id, 'serial': serial, 'before': list(bad), 'after': list(good)})
        if pending:
            backup = database.with_name('before-ios-coach-linkup.sqlite3')
            if not backup.exists():
                with sqlite3.connect(backup) as destination:
                    connection.backup(destination)
        with connection:
            connection.executemany('UPDATE coaches SET json=? WHERE coach_id=? AND serial=?', pending)
            connection.execute('INSERT OR REPLACE INTO meta VALUES(?,?)', (COACH_REVISION, json.dumps(changed)))
        return changed


# The factory seed gifts every special card (~13.5k) into the main list, which
# makes CMD_LOGIN ~14 MB and the club lists slow. Members should draw instead.
# Untouched gifts are removed from the owned list (receipts stay, so they are
# never re-gifted); every removed card that no pack offers is published in new
# finite "Kho cầu thủ" packs so no card becomes unobtainable.
TRIM_REVISION = 'ios-trim-factory-gifts-v1'
GIFT_SOURCE = 'efootxi:reserve-gift:v1'
ARCHIVE_FIRST_ID = 92000
ARCHIVE_SIZE = 500
POSITION_GROUPS = (('GK', 'Thủ môn', (0,)), ('DF', 'Hậu vệ', (1, 2, 3)),
                   ('MF', 'Tiền vệ', (4, 5, 6, 7, 8)), ('FW', 'Tiền đạo', (9, 10, 11, 12)))
TYPE_GROUPS = (('featured', 'Featured', ('Featured',)), ('trending', 'Trending', ('Trending',)),
               ('special', 'Epic & Huyền thoại', ('Epic', 'Legendary', 'Highlight', 'BigTime', 'ShowTime')))


def _references(value, out):
    if isinstance(value, dict):
        ident = value.get('gameplayer_id')
        if isinstance(ident, dict) and 'id' in ident and 'serial' in ident:
            out.add((ident['id'], ident['serial']))
        for item in value.values():
            _references(item, out)
    elif isinstance(value, list):
        for item in value:
            _references(item, out)


def _untouched(player):
    return (not any(player.get(k, 0) for k in ('games', 'goal', 'assist', 'red_card'))
            and player.get('is_lock') != 'YES'
            and not any(v != 74 for v in player.get('additional_skill_id') or [])
            and not any(player.get('additional_position') or [])
            and not player.get('_efootxi_skill_revision')
            and not player.get('_training_auto_allocation'))


def trim_factory_gifts(target):
    database = Path(target) / 'outputs/efootball/local-club.sqlite3'
    with sqlite3.connect(database.as_uri() + '?mode=rw', uri=True) as connection:
        if connection.execute('SELECT 1 FROM meta WHERE key=?', (TRIM_REVISION,)).fetchone():
            return {'already_applied': True}
        tables = {r[0] for r in connection.execute("SELECT name FROM sqlite_master WHERE type='table'")}
        if not {'gameplayers', 'managed_cards', 'managed_members', 'efootxi_special_pools', 'efootxi_reserve_gifts'} <= tables:
            return {'skipped': 'schema'}
        keep = set()
        for (raw,) in connection.execute('SELECT json FROM squads'):
            _references(json.loads(raw), keep)
        removable = []
        for pid, serial, raw in connection.execute('SELECT id,serial,json FROM gameplayers WHERE source=?', (GIFT_SOURCE,)):
            if (pid, serial) not in keep and _untouched(json.loads(raw)):
                removable.append((pid, serial))
        if not removable:
            connection.execute('INSERT OR REPLACE INTO meta VALUES(?,?)', (TRIM_REVISION, json.dumps({'removed': 0})))
            connection.commit()
            return {'removed': 0}
        backup = database.with_name('before-ios-trim-gifts.sqlite3')
        if not backup.exists():
            with sqlite3.connect(backup) as destination:
                connection.backup(destination)
        import special_pools
        offered = {r[0] for r in connection.execute('SELECT DISTINCT card_id FROM managed_members WHERE weight>0')}
        removed_cards = sorted({str(pid) for pid, _ in removable})
        groups = {}
        for cid in removed_cards:
            if cid in offered:
                continue
            row = connection.execute('SELECT card_type,payload FROM managed_cards WHERE card_id=?', (cid,)).fetchone()
            if not row or row[0] in ('Standard', 'UNKNOWN'):
                continue
            payload = json.loads(row[1])
            position = (payload.get('parameters') or [12])[0]
            tkey = next((k for k, _, kinds in TYPE_GROUPS if row[0] in kinds), 'special')
            pkey = next((k for k, _, codes in POSITION_GROUPS if position in codes), 'FW')
            if tkey == 'special':
                pkey = 'ALL'  # under a hundred cards: one pack, not four tiny ones
            groups.setdefault((tkey, pkey), []).append((-(payload.get('_overall') or 0), cid, row[0]))
        pools, agent_id = [], ARCHIVE_FIRST_ID
        while connection.execute('SELECT 1 FROM agents WHERE agent_id=?', (agent_id,)).fetchone():
            agent_id += 100
        for tkey, tname, _ in TYPE_GROUPS:
            for pkey, pname, _ in (POSITION_GROUPS if tkey != 'special' else (('ALL', 'Tất cả vị trí', ()),)):
                cards = sorted(groups.get((tkey, pkey), []))
                parts = [cards[i:i + ARCHIVE_SIZE] for i in range(0, len(cards), ARCHIVE_SIZE)]
                for n, part in enumerate(parts, 1):
                    suffix = ' %d' % n if len(parts) > 1 else ''
                    pools.append({'pool_key': 'ios-archive-%s-%s-%02d' % (tkey, pkey.lower(), n), 'agent_id': agent_id,
                                  'title': 'Kho cầu thủ · %s · %s%s' % (tname, pname, suffix), 'category': 'ARCHIVE',
                                  'generation': 'ios-archive-v1', 'member_card_ids': [c for _, c, _ in part],
                                  'cover_card_ids': [c for _, c, _ in part[:3]],
                                  'member_categories': {c: k for _, c, k in part}})
                    agent_id += 1
        with connection:
            for pool in pools:
                special_pools._put_pool(connection, pool)
            def meta(key, default):
                row = connection.execute('SELECT value FROM meta WHERE key=?', (key,)).fetchone()
                return json.loads(row[0]) if row else default
            active, prices = meta('league_position_agent_ids', []), meta('agent_price_list', [])
            for pool in pools:
                if pool['agent_id'] not in active:
                    active.append(pool['agent_id'])
                prices.append({'agent_id': pool['agent_id'], 'discount_limit_count': 0, 'discount_use_count': 0,
                               'price_list': [{'multiple_count': count, 'price': {'can_use_coin': 'YES', 'can_use_gp': 'NO',
                                   'coin_price': coin, 'coin_price_base': coin, 'gp_price': 0, 'gp_price_base': 0}}
                                   for count, coin in ((1, 100), (10, 900))]})
            for pid, serial in removable:
                for table in ('gameplayers', 'warehouse', 'careerplan'):
                    if table in tables:
                        connection.execute('DELETE FROM %s WHERE id=? AND serial=?' % table, (pid, serial))
            report = {'removed': len(removable), 'kept_in_squads': len(keep), 'new_packs': len(pools),
                      'cards_in_new_packs': sum(len(p['member_card_ids']) for p in pools)}
            for key, value in (('league_position_agent_ids', active), ('agent_price_list', prices),
                               (TRIM_REVISION, report)):
                connection.execute('INSERT OR REPLACE INTO meta VALUES(?,?)', (key, json.dumps(value)))
        return report
