#!/bin/sh
# interop.sh <veracrypt A> <veracrypt B>: volumes made by A must open with B
# and the other way round, for every cipher and every KDF.
A=$1 B=$2
PW=abcdefghijklmnopqrstuvwxyz0123 PW2=zyxwvutsrqponmlkjihgfedcba9876
C="--text --non-interactive --keyfiles= --random-source=/dev/urandom"
fail=0 n=0
one() { # one <maker> <opener> <encryption> <hash>
	rm -f t.hc
	$1 $C --create t.hc --size=1M --volume-type=normal --encryption=$3 --hash=$4 \
		--filesystem=none --password=$PW --pim=1 >/dev/null || { echo "FAIL create $3/$4"; fail=1; return; }
	r=ok
	$2 $C -C t.hc --password=$PW --pim=1 --new-password=$PW2 --new-pim=1 --new-keyfiles= >/dev/null || r=FAIL
	$1 $C -C t.hc --password=$PW2 --pim=1 --new-password=$PW --new-pim=1 --new-keyfiles= >/dev/null || r=FAIL
	$2 $C -C t.hc --password=wrongwrongwrongwrongwrong --pim=1 --new-password=$PW2 --new-pim=1 --new-keyfiles= >/dev/null 2>&1 && r=FAIL
	echo "$r $3/$4"; [ $r = ok ] || fail=1; n=$((n + 1))
}
for pair in "$A|$B" "$B|$A"; do
	mk=${pair%|*} op=${pair#*|}
	for s in AES/sha512 Serpent/sha256 Twofish/blake2s Camellia/whirlpool Kuznyechik/streebog \
		AES-Twofish-Serpent/sha512 Serpent-Twofish-AES/whirlpool \
		Kuznyechik-Serpent-Camellia/streebog Camellia-Kuznyechik/argon2; do
		one "$mk" "$op" ${s%/*} ${s#*/}
	done
done
rm -f t.hc
echo "$n interop cases"
exit $fail
