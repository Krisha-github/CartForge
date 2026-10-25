#!/usr/bin/env bash
# CartForge smoke test: drives the real program with scripted input and checks
# both its output and the resulting database state.
#
# WARNING: reloads database/schema.sql, i.e. DROPS the `cartforge` database.
#
# Usage (from the project root, after `make`):
#   CARTFORGE_DB_USER=root CARTFORGE_DB_PASSWORD=secret ./tests/smoke_test.sh

set -u
cd "$(dirname "$0")/.."

HOST="${CARTFORGE_DB_HOST:-127.0.0.1}"
PORT="${CARTFORGE_DB_PORT:-3306}"
USER_="${CARTFORGE_DB_USER:-root}"
export MYSQL_PWD="${CARTFORGE_DB_PASSWORD:-}"
BIN=./cartforge

sql()   { mysql -h"$HOST" -P"$PORT" -u"$USER_" -N -B "$@" 2>/dev/null; }
q()     { sql cartforge -e "$1"; }                       # single value query
run()   { printf '%s\n' "$@" | $BIN 2>&1; }              # feed menu input to the app
reset() { sql < database/schema.sql; sql < database/sample_data.sql; }

PASS=0; FAIL=0
check() {  # check "description" <command...>
    local desc="$1"; shift
    if "$@" >/dev/null 2>&1; then PASS=$((PASS+1)); echo "  PASS  $desc"
    else FAIL=$((FAIL+1)); echo "  FAIL  $desc"; fi
}
has()    { grep -qF -- "$2" <<<"$1"; }                   # has "$output" "text"
eq()     { [ "$1" = "$2" ]; }

[ -x "$BIN" ] || { echo "Build first: make"; exit 1; }
reset || { echo "Could not reach MySQL / load schema"; exit 1; }

echo "== Cart and checkout =="
out=$(run 8 1 2 2   9 1   0)
check "add to cart + cart total is 1598.00"           has "$out" "Cart total: 1598.00"

out=$(run 11 1   0)
check "order placed successfully"                     has "$out" "Order placed successfully. Order ID: 1"
check "order total is exact (1598.00)"                has "$out" "Total amount: 1598.00"
check "stock deducted 25 -> 23"                       eq "$(q 'SELECT stock_quantity FROM products WHERE product_id=2')" 23
check "cart emptied after checkout"                   eq "$(q 'SELECT COUNT(*) FROM cart_items WHERE user_id=1')" 0
check "one order + one order_item stored"             eq "$(q 'SELECT COUNT(*) FROM orders')/$(q 'SELECT COUNT(*) FROM order_items')" "1/1"

out=$(run 11 1   0)
check "empty cart cannot be ordered"                  has "$out" "Cart is empty"
check "no extra order created"                        eq "$(q 'SELECT COUNT(*) FROM orders')" 1

echo "== Stock rules =="
out=$(run 8 2 1 11   0)
check "cannot add more than stock to cart"            has "$out" "Not enough stock"

run 8 2 1 5 >/dev/null                                   # user 2 carts 5 Laptops (stock 10)
q "UPDATE products SET stock_quantity = 2 WHERE product_id = 1" >/dev/null
out=$(run 11 2   0)
check "checkout rejected when stock dropped"          has "$out" "Insufficient stock for 'Laptop': requested 5, available 2"
check "rollback: stock unchanged"                     eq "$(q 'SELECT stock_quantity FROM products WHERE product_id=1')" 2
check "rollback: no order created"                    eq "$(q 'SELECT COUNT(*) FROM orders')" 1
check "rollback: cart still intact"                   eq "$(q 'SELECT quantity FROM cart_items WHERE user_id=2 AND product_id=1')" 5
q "UPDATE products SET stock_quantity = 10 WHERE product_id = 1; DELETE FROM cart_items" >/dev/null

echo "== Security =="
evil="x'); DROP TABLE users; --"
run 1 "$evil" 10 5   0 >/dev/null
check "SQL-injection text stored literally"           eq "$(q "SELECT COUNT(*) FROM products WHERE product_name = \"x'); DROP TABLE users; --\"")" 1
check "users table survived"                          eq "$(q 'SELECT COUNT(*) FROM users')" 3
out=$(run 3 "%"   0)
check "LIKE wildcard in search is escaped"            has "$out" "(no records)"
out=$(run 6 "Eve" "krisha@example.com"   0)
check "duplicate e-mail rejected"                     has "$out" "already registered"
out=$(run 6 "Eve" "not-an-email" "eve@example.com"   0)
check "invalid e-mail re-prompted"                    has "$out" "valid e-mail"

echo "== Delete vs archive =="
evil_id=$(q "SELECT product_id FROM products WHERE product_name LIKE 'x%'")
out=$(run 5 "$evil_id"   0)
check "never-ordered product is hard deleted"         has "$out" "Product deleted successfully"
out=$(run 5 2   0)
check "ordered product is archived, not deleted"      has "$out" "archived instead of deleted"
check "archived row still exists"                     eq "$(q 'SELECT is_active FROM products WHERE product_id=2')" 0
out=$(run 2   0)
check "archived product hidden from listing"          bash -c '! grep -qF "Wireless Mouse" <<<"$0"' "$out"
out=$(run 12 1   0)
check "order history still shows archived product"    has "$out" "Wireless Mouse"
q "UPDATE products SET price = 1 WHERE product_id = 2" >/dev/null
out=$(run 12 1   0)
check "history keeps price-at-purchase snapshot"      has "$out" "799.00"

echo "== Input handling =="
out=$(run abc 99 "" 2   0)
check "bad menu input is re-prompted (no crash/loop)" has "$out" "Please enter a whole number between 0 and 15"
check "valid choice still works afterwards"           has "$out" "Product"
out=$($BIN </dev/null 2>&1); code=$?
check "end of input exits cleanly"                    eq "$code" 0
check "end of input prints message"                   has "$out" "Input closed"

echo
echo "Result: $PASS passed, $FAIL failed"
[ "$FAIL" -eq 0 ]
