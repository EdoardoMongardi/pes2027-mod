"""Renumber the skill cards in two tables of Konami's Android dt870 from Android to iOS 11.1.1.

The mod installs Konami's Android dt870 on iOS. Two of its tables carry skill card numbers,
and Android and iOS number the skill cards differently:

  Android 11.0.1 has 72 skill cards (0..71). iOS 11.1.1 has 74 (0..73): it inserts one card
  before Android 25 and one before Android 67. So Android card a is iOS card
  a + (a >= 25) + (a >= 67), the same rule as the server's iosSkillId(a, true) in
  core/ios_compat.hpp for real skills. The two iOS-only cards are 25 and 68 (parameter labels
  143 and 142); no Android table has them.

PlayerVariationPrSkill.bin (the featured special skills of box and banner previews)
  16-byte records: u64 card id, u32 with three 7-bit skill ids at bits 0, 7 and 14, u32 zero.
  The client (LinkPesdb::ImportPlayerVariationPrSkill 0x104ba0eec, UtilityMyClub::
  GetPRCardSkillList 0x1048958c0) shows no skill for any id of 74 or more. Android marks an
  empty slot with 72, iOS with 74, so on iOS every Android empty slot read as card 72
  ("Fighting Spirit") and every id from 25 up as another skill. The empty marker must not go
  through iosSkillId: that maps 72 to 25, a Dummy card. Records keep their order (the client
  binary-searches them by card id).

PlayerSkill.bin (skill priorities; above 0 means special; read by the substitution screen)
  iOS: 74 records of 4 bytes, bits 0..6 = skill id, bits 7..11 = priority (LinkPesdb::
  ImportPlayerSkillData 0x104ba0fb8, looked up by id in CommonWork::GetPlayerSkillData
  0x104b963c0). Android: 72 records of 104 bytes (4 bytes once compact_legacy_records has run),
  numbered the Android way. The two iOS-only records are copied from Konami's own iOS table
  (dt200 PlayerSkill.bin records 25 and 68). The result is byte-identical to Konami's iOS dt200
  table.

renumber_featured_skills and renumber_player_skills are table steps for
tools/repair_ios_tables.py: decoded table bytes -> (new bytes, info). A table already in iOS
numbering is returned unchanged, so running the repair again changes nothing.
"""
import struct

ANDROID_SKILL_COUNT = 72
IOS_SKILL_COUNT = 74
ANDROID_PR_EMPTY = 72          # Android "no featured skill" marker
IOS_PR_EMPTY = 74              # iOS marker (the client skips any id of 74 or more)
IOS_ONLY_SKILLS = (25, 68)     # iOS cards with no Android counterpart

# Konami's iOS dt200 PlayerSkill.bin records 25 and 68, verbatim: id 25 priority 30, id 68 priority 29.
IOS_ONLY_PLAYER_SKILL_WORDS = {25: 0x00000F19, 68: 0x00000EC4}

PR_SKILL_RECORD = 16
PR_SKILL_MAX_RECORDS = 22500   # LinkPesdb::ImportPlayerVariationPrSkill reads at most 0x57e4 records
PLAYER_SKILL_ANDROID_STRIDE = 104
PLAYER_SKILL_IOS_STRIDE = 4


def android_to_ios_skill(a):
    """Android skill card number -> iOS skill card number (real skills only)."""
    if not isinstance(a, int) or not 0 <= a < ANDROID_SKILL_COUNT:
        raise ValueError("not an Android skill card number: %r" % (a,))
    return a + (a >= 25) + (a >= 67)


ANDROID_TO_IOS = tuple(android_to_ios_skill(a) for a in range(ANDROID_SKILL_COUNT))


def android_to_ios_pr_slot(value):
    """One 7-bit featured-skill slot: an Android skill or the empty marker -> the iOS value."""
    if value == ANDROID_PR_EMPTY:
        return IOS_PR_EMPTY
    return android_to_ios_skill(value)  # raises on 73..127, which Android never writes


def pr_slots(word):
    """The three 7-bit slots of a PlayerVariationPrSkill.bin record, in file order."""
    return [(word >> s) & 0x7F for s in (0, 7, 14)]


def pr_skill_numbering(raw):
    """'android', 'ios' or 'unknown' for a decoded PlayerVariationPrSkill.bin."""
    if len(raw) % PR_SKILL_RECORD:
        raise ValueError("PlayerVariationPrSkill.bin size %d is not a multiple of 16" % len(raw))
    seen = set()
    for off in range(0, len(raw), PR_SKILL_RECORD):
        seen.update(pr_slots(struct.unpack_from("<I", raw, off + 8)[0]))
    if any(v > IOS_PR_EMPTY for v in seen):
        return "unknown"            # neither platform writes these
    if seen & {73, IOS_PR_EMPTY}:
        return "ios"                # Android has no 73 and never 74 (its last value is 72)
    if ANDROID_PR_EMPTY in seen:
        return "android"            # 72 without 73 or 74: Android's empty marker
    return "unknown"                # no empty slot anywhere: cannot tell


def convert_pr_skill_table(raw):
    """Android-numbered PlayerVariationPrSkill.bin -> iOS-numbered, same records, order and size."""
    kind = pr_skill_numbering(raw)
    if kind != "android":
        raise ValueError("PlayerVariationPrSkill.bin is not Android-numbered (looks %s)" % kind)
    count = len(raw) // PR_SKILL_RECORD
    if count > PR_SKILL_MAX_RECORDS:
        raise ValueError("%d records; the iOS client reads only %d" % (count, PR_SKILL_MAX_RECORDS))
    out = bytearray(raw)
    previous = -1
    for off in range(0, len(raw), PR_SKILL_RECORD):
        card = struct.unpack_from("<q", raw, off)[0]
        if card <= previous:
            raise ValueError("card ids must stay strictly increasing (the client binary-searches them)")
        previous = card
        word = struct.unpack_from("<I", raw, off + 8)[0]
        new = word & ~0x1FFFFF
        for shift, value in zip((0, 7, 14), pr_slots(word)):
            new |= android_to_ios_pr_slot(value) << shift
        struct.pack_into("<I", out, off + 8, new)
    return bytes(out)


def _player_skill_words(raw):
    """Decoded PlayerSkill.bin -> (numbering, list of 32-bit words)."""
    if len(raw) == ANDROID_SKILL_COUNT * PLAYER_SKILL_ANDROID_STRIDE:
        stride, numbering = PLAYER_SKILL_ANDROID_STRIDE, "android"
    elif len(raw) == ANDROID_SKILL_COUNT * PLAYER_SKILL_IOS_STRIDE:
        stride, numbering = PLAYER_SKILL_IOS_STRIDE, "android"   # compacted by compact_legacy_records
    elif len(raw) == IOS_SKILL_COUNT * PLAYER_SKILL_IOS_STRIDE:
        stride, numbering = PLAYER_SKILL_IOS_STRIDE, "ios"
    else:
        raise ValueError("unexpected PlayerSkill.bin size %d" % len(raw))
    words = [struct.unpack_from("<I", raw, i)[0] for i in range(0, len(raw), stride)]
    expected = ANDROID_SKILL_COUNT if numbering == "android" else IOS_SKILL_COUNT
    if [w & 0x7F for w in words] != list(range(expected)):
        raise ValueError("PlayerSkill.bin records are not numbered 0..%d in order" % (expected - 1))
    return numbering, words


def player_skill_numbering(raw):
    return _player_skill_words(raw)[0]


def convert_player_skill_table(raw):
    """Android PlayerSkill.bin (104- or 4-byte records) -> iOS: 74 records of 4 bytes, by iOS id."""
    numbering, words = _player_skill_words(raw)
    if numbering != "android":
        raise ValueError("PlayerSkill.bin is already in the iOS layout")
    by_id = dict(IOS_ONLY_PLAYER_SKILL_WORDS)
    for a, word in enumerate(words):
        by_id[android_to_ios_skill(a)] = (word & ~0x7F) | android_to_ios_skill(a)
    if sorted(by_id) != list(range(IOS_SKILL_COUNT)):
        raise AssertionError("internal: the iOS ids do not cover 0..73")
    return b"".join(struct.pack("<I", by_id[k]) for k in range(IOS_SKILL_COUNT))


# ---------------------------------------------------------------- table steps (tools/repair_ios_tables.py)

def renumber_featured_skills(raw):
    """PlayerVariationPrSkill.bin table step. Returns (raw, counts)."""
    kind = pr_skill_numbering(raw)
    if kind == "ios":
        return raw, {}
    new = convert_pr_skill_table(raw)
    counts = {"records": len(raw) // PR_SKILL_RECORD, "skills_renumbered": 0, "skills_kept": 0, "empty_slots": 0}
    for off in range(0, len(raw), PR_SKILL_RECORD):
        for value in pr_slots(struct.unpack_from("<I", raw, off + 8)[0]):
            if value == ANDROID_PR_EMPTY:
                counts["empty_slots"] += 1
            elif android_to_ios_skill(value) != value:
                counts["skills_renumbered"] += 1
            else:
                counts["skills_kept"] += 1
    return new, counts


def renumber_player_skills(raw):
    """PlayerSkill.bin table step. Returns (raw, info)."""
    if player_skill_numbering(raw) == "ios":
        return raw, {}
    new = convert_player_skill_table(raw)
    return new, {"before": len(raw), "after": len(new), "records": len(new) // PLAYER_SKILL_IOS_STRIDE}
