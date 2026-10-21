Efootxi iOS display and termination diagnostics
=============================================

The Android gateway returned Chinese country labels while the installed iOS font
replacement covers Latin and Vietnamese. The iOS response adapter now substitutes
ISO 3166 display names, preserving country codes, privacy actions, region lists,
and request IDs. `ios/efootxi/country_names.json` contains factual ISO names
generated from pycountry 26.2.16 (Debian iso-codes), with common display aliases.
This does not change an existing account's selected country.

The native client concatenates its CDN root and `/prd` with `banner_file_name`.
An absolute loopback URL therefore became
`/prdhttp://127.0.0.1:8085/livedata/efootxi-home-players.jpg` and was incorrectly
passed to the encrypted gate decoder. The adapter emits a CDN-relative path and
serves the two local JPEGs through both native and legacy routes. Unknown image
GETs return 404 without attempting AES decoding.

Remove the earlier adapter's forced Film, MobileHDR, gamma, saturation, contrast,
and midpoint switches. Keep unrelated command-line switches, save the previous
file in diagnostics, and use the game's cooked renderer defaults. Visual results
still require device verification.

Keep the preceding launch under `previous-run` so reopening the app cannot erase
the fatal request, Python traceback, or native stack. Imported `exit`, `_exit`, and
`abort` calls in the main executable write `termination.log` before continuing the
original termination. Signals retain their existing handler. These cover only
in-process events; an iOS jetsam/kill may have no in-process record. A background
lifecycle event does not identify a swipe, a crash, or a memory kill.

The supplied session reaches LOGIN and MAINMENU successfully but contains no
initial club-selection command or corresponding fatal report. The available old
PES crash reports concern startup shader compilation. No club-selection fix can
be established from those reports. This change does not reset the save or select
a club on the user's behalf.

Validation: regression tests cover label/policy preservation, native and malformed
banner routes, exact image bytes and MIME, unknown GETs, native live-data hashes,
request/response logs, and traversal rejection. The local private gateway is also
exercised without changing phone data.
