#!/usr/bin/env bash
# fetch-amissl.sh - the AmiSSL 5.27 SDK (only its headers are used) for
# wget's https part (src/wget_tls.c). Not kept in git: make downloads it
# into third_party/AmiSSL/ the first time it is needed.
set -e
cd "$(dirname "$0")"
[ -d AmiSSL/Developer/include ] && exit 0
curl -sfL -o AmiSSL-5.27-SDK.lha \
  https://github.com/jens-maus/amissl/releases/download/5.27/AmiSSL-5.27-SDK.lha
lha xq AmiSSL-5.27-SDK.lha
rm -f AmiSSL-5.27-SDK.lha
echo "third_party/AmiSSL ready"
