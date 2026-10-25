#!/usr/bin/env bash
# CartForge concurrency test: proves two buyers cannot oversell the last item.
#
# WARNING: reloads database/schema.sql, i.e. DROPS the `cartforge` database.
#
# Part A (race):      two real app processes check out the LAST item at the same
#                     time, many rounds. Exactly one may succeed every round.
# Part B (lock wait): a SQL session holds the row lock for 3 s; the real app's
#                     checkout must visibly WAIT, then be rejected (stock gone).
#
# Usage (from the project root, after `make`):
#   CARTFORGE_DB_USER=root CARTFORGE_DB_PASSWORD=secret ./tests/concurrency_test.sh

set -u
cd "$(dirname "$0")/.."

HOST="${CARTFORGE_DB_HOST:-127.0.0.1}"
PORT="${CARTFORGE_DB_PORT:-3306}"
USER_="${CARTFORGE_DB_USER:-root}"
export MYSQL_PWD="${CARTFORGE_DB_PASSWORD:-}"
BIN=./cartforge
ROUNDS="${ROUNDS:-25}"

sql() { mysql -h"$HOST" -P"$PORT" -u"$USER_" -N -B "$@" 2>/dev/null; }
q()   { sql cartforge -e "$1"; }

[ -x "$BIN" ] || { echo "Build first: make"; exit 1; }
sql < database/schema.sql && sql < database/sample_data.sql || { echo "Could not reach MySQL"; exit 1; }

TMP=$(mktemp -d); trap 'rm -rf "$TMP"' EXIT
fail=0

echo "== Part A: $ROUNDS rounds, 2 simultaneous checkouts of the last Laptop =="
bad=0
for i in $(seq 1 "$ROUNDS"); do
    q "DELETE FROM orders; DELETE FROM cart_items;
       UPDATE products SET stock_quantity = 1 WHERE product_id = 1;
       INSERT INTO cart_items (user_id, product_id, quantity) VALUES (1,1,1),(2,1,1);" >/dev/null
    printf '11\n1\n0\n' | $BIN > "$TMP/a.txt" 2>&1 &
    printf '11\n2\n0\n' | $BIN > "$TMP/b.txt" 2>&1 &
    wait
    ok=$(cat "$TMP/a.txt" "$TMP/b.txt" | grep -c "Order placed successfully")
    rej=$(cat "$TMP/a.txt" "$TMP/b.txt" | grep -c "Insufficient stock")
    stock=$(q "SELECT stock_quantity FROM products WHERE product_id=1")
    orders=$(q "SELECT COUNT(*) FROM orders")
    if [ "$ok" != 1 ] || [ "$rej" != 1 ] || [ "$stock" != 0 ] || [ "$orders" != 1 ]; then
        bad=$((bad+1)); echo "  round $i: ok=$ok rejected=$rej stock=$stock orders=$orders  <-- WRONG"
    fi
done
if [ "$bad" -eq 0 ]; then echo "  PASS  all $ROUNDS rounds: exactly 1 order, 1 rejection, stock 0 (never negative)"
else echo "  FAIL  $bad of $ROUNDS rounds violated the invariant"; fail=1; fi

echo "== Part B: row lock held by another session =="
q "DELETE FROM orders; DELETE FROM cart_items;
   UPDATE products SET stock_quantity = 1 WHERE product_id = 1;
   INSERT INTO cart_items (user_id, product_id, quantity) VALUES (2,1,1);" >/dev/null

# Session A: grabs the Laptop row lock, holds it 3 s, buys the last unit, commits.
sql cartforge -e "START TRANSACTION;
                  SELECT stock_quantity FROM products WHERE product_id = 1 FOR UPDATE;
                  SELECT SLEEP(3);
                  UPDATE products SET stock_quantity = stock_quantity - 1 WHERE product_id = 1;
                  COMMIT;" >/dev/null &
sleep 1                                               # let session A take the lock

start=$(date +%s.%N)
out=$(printf '11\n2\n0\n' | $BIN 2>&1)
end=$(date +%s.%N)
elapsed=$(awk -v s="$start" -v e="$end" 'BEGIN{printf "%.1f", e-s}')
wait

waited=$(awk -v t="$elapsed" 'BEGIN{print (t >= 1.5) ? "yes" : "no"}')
if [ "$waited" = yes ];                                  then echo "  PASS  checkout waited for the lock (${elapsed}s)"; else echo "  FAIL  did not wait (${elapsed}s)"; fail=1; fi
if grep -qF "Insufficient stock" <<<"$out";              then echo "  PASS  after waiting it saw stock 0 and was rejected"; else echo "  FAIL  expected rejection"; fail=1; fi
if [ "$(q 'SELECT stock_quantity FROM products WHERE product_id=1')" = 0 ] && [ "$(q 'SELECT COUNT(*) FROM orders')" = 0 ]; then
    echo "  PASS  stock is 0 (not negative), no order was created"
else echo "  FAIL  inconsistent final state"; fail=1; fi

echo
[ "$fail" -eq 0 ] && echo "Concurrency test: ALL PASSED" || echo "Concurrency test: FAILURES"
exit "$fail"
