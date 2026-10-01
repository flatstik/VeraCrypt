#!/bin/sh
# official-volumes.sh <veracrypt>: open and re-key the Tests/*.hc volumes,
# which were created by little-endian VeraCrypt (password "test").
fail=0
for v in Tests/test.*.hc; do
	cp "$v" t.hc
	if $1 --text --non-interactive --keyfiles= --pim=0 --random-source=/dev/urandom -C t.hc \
		--password=test --new-password=test2 --new-pim=0 --new-keyfiles= >/dev/null; then
		echo "ok $v"
	else
		echo "FAIL $v"; fail=1
	fi
done
rm -f t.hc
exit $fail
