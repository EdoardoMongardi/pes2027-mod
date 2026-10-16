#!/usr/bin/env bash
# Isolated PES services on Ubuntu 24.04; does not reconfigure or restart GTA.
set -euo pipefail
HOST="${1:?usage: sudo bash setup_ec2.sh <public-ip-or-domain>}"
[[ "$EUID" == 0 ]] || { echo 'Run with sudo.' >&2; exit 1; }
[[ "$HOST" =~ ^[a-zA-Z0-9.-]+$ ]] || { echo 'Invalid host.' >&2; exit 1; }
DIR=/opt/pes2027-online
CONF=/etc/pes2027-online
SRC="$(cd "$(dirname "$0")" && pwd)"
PRIV_IP="$(ip -4 route get 1.1.1.1 | awk '{for(i=1;i<=NF;i++)if($i=="src"){print $(i+1);exit}}')"
PUBLIC_IP="$(getent ahostsv4 "$HOST" | awk 'NR==1{print $1}')"
[[ -n "$PRIV_IP" && -n "$PUBLIC_IP" ]] || { echo 'Cannot resolve IP addresses.' >&2; exit 1; }
if systemctl is-active --quiet coturn; then
  echo 'Existing coturn is running; configure separate TURN ports first.' >&2
  exit 1
fi
export NEEDRESTART_MODE=l
apt-get update -qq
DEBIAN_FRONTEND=noninteractive apt-get install -y python3-venv coturn openssl
# The new package may auto-start its default instance; PES uses its own unit/config.
systemctl stop coturn
systemctl disable coturn
id pes2027 >/dev/null 2>&1 || useradd --system --home-dir "$DIR" --shell /usr/sbin/nologin pes2027
install -d -m 755 "$DIR"
install -d -m 750 -o pes2027 -g pes2027 "$DIR/logs"
install -d -m 750 -o root -g pes2027 "$CONF"
install -m 644 "$SRC/pes_online_server.py" "$SRC/requirements.txt" "$DIR/"
python3 -m venv "$DIR/venv"
"$DIR/venv/bin/pip" install --quiet -r "$DIR/requirements.txt"
if [[ ! -f "$CONF/server.env" ]]; then
  umask 077
  printf 'PES2027_TURN_SECRET=%s\nPES2027_ACCESS_TOKEN=%s\nPES2027_ADMIN_TOKEN=%s\n' \
    "$(openssl rand -hex 32)" "$(openssl rand -hex 32)" "$(openssl rand -hex 32)" > "$CONF/server.env"
fi
chmod 600 "$CONF/server.env"
set -a
source "$CONF/server.env"
set +a
if [[ ! -f "$CONF/cert.pem" ]]; then
  SAN="DNS:$HOST"
  [[ "$HOST" =~ ^[0-9.]+$ ]] && SAN="IP:$HOST"
  openssl req -x509 -newkey rsa:2048 -nodes -days 365 -subj "/CN=$HOST" -addext "subjectAltName=$SAN" \
    -keyout "$CONF/key.pem" -out "$CONF/cert.pem" >/dev/null 2>&1
fi
chown root:pes2027 "$CONF/key.pem" "$CONF/cert.pem"
chmod 640 "$CONF/key.pem"
chmod 644 "$CONF/cert.pem"
cat > "$CONF/turnserver.conf" <<CONF
listening-port=3478
tls-listening-port=5349
listening-ip=$PRIV_IP
relay-ip=$PRIV_IP
external-ip=$PUBLIC_IP/$PRIV_IP
min-port=49160
max-port=49259
fingerprint
use-auth-secret
static-auth-secret=$PES2027_TURN_SECRET
realm=pes2027
cert=$CONF/cert.pem
pkey=$CONF/key.pem
no-cli
no-multicast-peers
no-tcp-relay
denied-peer-ip=0.0.0.0-0.255.255.255
denied-peer-ip=10.0.0.0-10.255.255.255
denied-peer-ip=127.0.0.0-127.255.255.255
denied-peer-ip=169.254.0.0-169.254.255.255
denied-peer-ip=172.16.0.0-172.31.255.255
denied-peer-ip=192.168.0.0-192.168.255.255
denied-peer-ip=::1
denied-peer-ip=fc00::-fdff:ffff:ffff:ffff:ffff:ffff:ffff:ffff
user-quota=4
total-quota=32
max-bps=625000
bps-capacity=2500000
simple-log
log-file=stdout
CONF
chmod 640 "$CONF/turnserver.conf"
chown root:pes2027 "$CONF/turnserver.conf"
cat > /etc/systemd/system/pes2027-turn.service <<UNIT
[Unit]
Description=PES2027 isolated TURN relay
After=network-online.target
[Service]
User=pes2027
Group=pes2027
RuntimeDirectory=pes2027-turn
ExecStart=/usr/bin/turnserver -c $CONF/turnserver.conf --pidfile=/run/pes2027-turn/turnserver.pid
Restart=on-failure
MemoryMax=160M
TasksMax=64
NoNewPrivileges=true
ProtectSystem=strict
ProtectHome=true
PrivateTmp=true
ReadWritePaths=/run/pes2027-turn
[Install]
WantedBy=multi-user.target
UNIT
cat > /etc/systemd/system/pes2027-online.service <<UNIT
[Unit]
Description=PES2027 experimental Friend Match API 1.1
After=network-online.target
[Service]
User=pes2027
Group=pes2027
WorkingDirectory=$DIR
EnvironmentFile=$CONF/server.env
ExecStart=$DIR/venv/bin/python $DIR/pes_online_server.py --port 8443 --cert $CONF/cert.pem --key $CONF/key.pem --turn-host $HOST --log-dir $DIR/logs
Restart=on-failure
MemoryMax=192M
TasksMax=64
NoNewPrivileges=true
ProtectSystem=strict
ProtectHome=true
PrivateTmp=true
ReadWritePaths=$DIR/logs
[Install]
WantedBy=multi-user.target
UNIT
export PES2027_HOST="$HOST"
python3 - <<'PY'
import hashlib,json,os,pathlib,ssl
p=pathlib.Path('/etc/pes2027-online')
der=ssl.PEM_cert_to_DER_cert((p/'cert.pem').read_text())
cfg={'server':'https://'+os.environ['PES2027_HOST']+':8443','enabled':True,
     'token':os.environ['PES2027_ACCESS_TOKEN'],'cert_sha256':hashlib.sha256(der).hexdigest()}
(p/'client.json').write_text(json.dumps(cfg))
(p/'client.json').chmod(0o600)
PY
systemctl daemon-reload
systemctl enable pes2027-online pes2027-turn
systemctl restart pes2027-online pes2027-turn
sleep 2
systemctl is-active --quiet pes2027-online
systemctl is-active --quiet pes2027-turn
curl --fail --silent --insecure https://127.0.0.1:8443/health
printf '\nPES services ready; private IPA config: /etc/pes2027-online/client.json\n'
