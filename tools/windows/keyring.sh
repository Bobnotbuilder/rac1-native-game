# The keys pacman checks MSYS2's packages with, for tools/windows/toolchain.ps1 (run with MSYS2's bash, before
# MSYS2's first start, which then leaves the keyring alone). The same trust as `pacman-key --init; --populate msys2`
# (the keys of msys2-trusted fully trusted, those of msys2-revoked out), without gpg-agent: pacman-key needs it to
# make and use a local signing key, and on some Windows systems MSYS2's gpg cannot reach its agent (it waits forever).
set -e
export PATH=/usr/bin
G=/etc/pacman.d/gnupg
K=/usr/share/pacman/keyrings
rm -rf "$G.new"
mkdir -p "$G.new"
chmod 700 "$G.new"
# (no-autostart: gpg starts no agent, and no dirmngr for a key server, where it does not need them)
printf '%s\n' no-greeting no-permission-warning no-autostart lock-never 'keyserver-options timeout=10' \
    > "$G.new/gpg.conf"
gpg --homedir "$G.new" --batch --quiet --import "$K/msys2.gpg"
for key in $(cat "$K/msys2-revoked"); do
    gpg --homedir "$G.new" --batch --yes --quiet --delete-keys "$key" 2>/dev/null || true
done
# (ultimate ownertrust, 6: a key so trusted makes valid the keys it certified, as the local signature does)
sed 's/:[0-9]*:$/:6:/' "$K/msys2-trusted" | gpg --homedir "$G.new" --batch --quiet --import-ownertrust
gpg --homedir "$G.new" --batch --quiet --check-trustdb
rm -rf "$G"
mv "$G.new" "$G"
