#include "order.h"
#include "db_connection.h"
#include "utils.h"

#include <climits>
#include <iostream>
#include <string>
#include <vector>

using namespace std;

namespace
{
    struct OrderLine
    {
        string productId;
        string name;
        string quantity;
        string price; // DECIMAL kept as text -> exact money arithmetic in SQL
    };
}

OrderManager::OrderManager(MYSQL *connection)
{
    conn = connection;
}

// ---------------------------------------------------------------------------
// Transaction-safe checkout
//
//   1. START TRANSACTION
//   2. Lock the user's cart rows                        (SELECT ... FOR UPDATE)
//   3. Lock each product row, lowest product_id first   (SELECT ... FOR UPDATE)
//   4. Validate every line against the locked stock
//   5. Insert the order and its order_items (price snapshot)
//   6. Deduct stock
//   7. Compute the order total in SQL
//   8. Clear the cart
//   9. COMMIT          - any failure above => ROLLBACK
//
// Products are always locked in ascending product_id order, so two checkouts
// with overlapping products queue up instead of deadlocking. A second buyer of
// the last item waits on the row lock, then sees the reduced stock and is
// rejected - overselling is impossible.
// ---------------------------------------------------------------------------
OrderManager::Result OrderManager::attemptOrder(const string &userId)
{
    Result r;

    auto abort = [&](Outcome outcome, const string &message, unsigned int errCode = 0) -> Result
    {
        mysql_query(conn, "ROLLBACK");
        r.outcome = (errCode == dberr::DEADLOCK) ? Outcome::Deadlock : outcome;
        r.message = message;
        return r;
    };

    if (mysql_query(conn, "START TRANSACTION") != 0)
        return abort(Outcome::Error, string("Could not start transaction: ") + mysql_error(conn));

    // 2. Lock the cart rows (unique index (user_id, product_id) yields them in product_id order).
    Statement cartStmt(conn,
                       "SELECT product_id, quantity FROM cart_items "
                       "WHERE user_id = ? ORDER BY product_id FOR UPDATE");
    if (!cartStmt.execute({userId}))
        return abort(Outcome::Error, "Error reading cart: " + cartStmt.errorMessage(), cartStmt.errorCode());

    vector<vector<string>> cartRows = cartStmt.fetchAll();
    if (cartRows.empty())
        return abort(Outcome::Rejected, "Cart is empty. Cannot place order.");

    // 3 + 4. Lock each product row in order, then validate it.
    Statement productStmt(conn,
                          "SELECT product_name, price, stock_quantity, is_active "
                          "FROM products WHERE product_id = ? FOR UPDATE");
    vector<OrderLine> lines;

    for (const auto &cartRow : cartRows)
    {
        if (!productStmt.execute({cartRow[0]}))
            return abort(Outcome::Error, "Error locking product: " + productStmt.errorMessage(),
                         productStmt.errorCode());

        vector<string> p;
        if (!productStmt.fetch(p))
            return abort(Outcome::Rejected, "Product " + cartRow[0] + " no longer exists.");

        long long wanted = stoll(cartRow[1]);
        long long stock = stoll(p[2]);

        if (p[3] != "1")
            return abort(Outcome::Rejected,
                         "'" + p[0] + "' is no longer available. Remove it from the cart.");

        if (stock < wanted)
            return abort(Outcome::Rejected,
                         "Insufficient stock for '" + p[0] + "': requested " + cartRow[1] +
                             ", available " + p[2] + ".");

        lines.push_back({cartRow[0], p[0], cartRow[1], p[1]});
    }

    // 5. Create the order and its line items.
    Statement orderStmt(conn, "INSERT INTO orders (user_id) VALUES (?)");
    if (!orderStmt.execute({userId}))
        return abort(Outcome::Error, "Error creating order: " + orderStmt.errorMessage(),
                     orderStmt.errorCode());
    r.orderId = to_string(orderStmt.insertId());

    Statement itemStmt(conn,
                       "INSERT INTO order_items (order_id, product_id, quantity, price_at_purchase) "
                       "VALUES (?, ?, ?, ?)");
    Statement stockStmt(conn,
                        "UPDATE products SET stock_quantity = stock_quantity - CAST(? AS SIGNED) "
                        "WHERE product_id = ?");

    for (const auto &line : lines)
    {
        if (!itemStmt.execute({r.orderId, line.productId, line.quantity, line.price}))
            return abort(Outcome::Error, "Error inserting order item: " + itemStmt.errorMessage(),
                         itemStmt.errorCode());

        // 6. Deduct inventory (the CHECK constraint is a second safety net).
        if (!stockStmt.execute({line.quantity, line.productId}))
            return abort(Outcome::Error, "Error updating stock: " + stockStmt.errorMessage(),
                         stockStmt.errorCode());
    }

    // 7. Total computed by MySQL from the saved line items (exact DECIMAL maths).
    Statement totalStmt(conn,
                        "UPDATE orders SET total_amount = "
                        "(SELECT IFNULL(SUM(quantity * price_at_purchase), 0) "
                        " FROM order_items WHERE order_id = ?) "
                        "WHERE order_id = ?");
    if (!totalStmt.execute({r.orderId, r.orderId}))
        return abort(Outcome::Error, "Error computing total: " + totalStmt.errorMessage(),
                     totalStmt.errorCode());

    // 8. Empty the cart.
    Statement clearStmt(conn, "DELETE FROM cart_items WHERE user_id = ?");
    if (!clearStmt.execute({userId}))
        return abort(Outcome::Error, "Error clearing cart: " + clearStmt.errorMessage(),
                     clearStmt.errorCode());

    // Read the total back while still inside the transaction.
    Statement readTotal(conn, "SELECT total_amount FROM orders WHERE order_id = ?");
    vector<string> row;
    if (readTotal.execute({r.orderId}) && readTotal.fetch(row))
        r.total = row[0];

    // 9. Commit.
    if (mysql_query(conn, "COMMIT") != 0)
        return abort(Outcome::Error, string("Commit failed: ") + mysql_error(conn),
                     mysql_errno(conn));

    r.outcome = Outcome::Success;
    return r;
}

void OrderManager::placeOrder()
{
    int userId = util::readInt("Enter user ID: ", 1, INT_MAX);
    string uid = to_string(userId);

    if (!rowExists(conn, "SELECT 1 FROM users WHERE user_id = ?", {uid}))
    {
        cout << "User " << userId << " does not exist.\n";
        return;
    }

    const int maxAttempts = 3;
    Result result;
    for (int attempt = 1; attempt <= maxAttempts; ++attempt)
    {
        result = attemptOrder(uid);
        if (result.outcome != Outcome::Deadlock)
            break;
        cout << "Deadlock detected, retrying (" << attempt << "/" << maxAttempts << ")...\n";
    }

    if (result.outcome == Outcome::Success)
    {
        cout << "Order placed successfully. Order ID: " << result.orderId << endl;
        cout << "Total amount: " << result.total << endl;
    }
    else
    {
        cout << result.message << endl;
        cout << "Transaction rolled back. Nothing was changed.\n";
    }
}

void OrderManager::viewOrderHistory()
{
    int userId = util::readInt("Enter user ID: ", 1, INT_MAX);

    Statement stmt(conn,
                   "SELECT o.order_id, o.order_date, p.product_name, oi.quantity, "
                   "oi.price_at_purchase, (oi.quantity * oi.price_at_purchase) AS line_total "
                   "FROM orders o "
                   "JOIN order_items oi ON o.order_id = oi.order_id "
                   "JOIN products p ON oi.product_id = p.product_id "
                   "WHERE o.user_id = ? "
                   "ORDER BY o.order_date DESC, o.order_id DESC, oi.order_item_id");

    if (!stmt.execute({to_string(userId)}))
    {
        cout << "Error: " << stmt.errorMessage() << endl;
        return;
    }

    util::printTable({"Order ID", "Date", "Product", "Qty", "Price", "Line total"}, stmt.fetchAll());

    Statement spent(conn,
                    "SELECT COUNT(*), IFNULL(SUM(total_amount), 0) FROM orders WHERE user_id = ?");
    vector<string> row;
    if (spent.execute({to_string(userId)}) && spent.fetch(row))
        cout << "\nOrders: " << row[0] << "   Lifetime spend: " << row[1] << endl;
}

void OrderManager::lowStockReport()
{
    int threshold = util::readInt("Enter low stock threshold: ", 0, 1000000);

    Statement stmt(conn,
                   "SELECT product_id, product_name, stock_quantity "
                   "FROM products WHERE is_active = 1 AND stock_quantity < ? "
                   "ORDER BY stock_quantity, product_id");

    if (!stmt.execute({to_string(threshold)}))
    {
        cout << "Error: " << stmt.errorMessage() << endl;
        return;
    }

    cout << "\nLow Stock Products (stock below " << threshold << ")";
    util::printTable({"ID", "Product", "Stock"}, stmt.fetchAll());
}

void OrderManager::totalSalesReport()
{
    Statement stmt(conn, "SELECT COUNT(*), IFNULL(SUM(total_amount), 0) FROM orders");

    vector<string> row;
    if (!stmt.execute() || !stmt.fetch(row))
    {
        cout << "Error: " << stmt.errorMessage() << endl;
        return;
    }

    cout << "Orders placed: " << row[0] << endl;
    cout << "Total sales: " << row[1] << endl;
}

void OrderManager::topSellingProducts()
{
    Statement stmt(conn,
                   "SELECT p.product_name, SUM(oi.quantity) AS units_sold, "
                   "SUM(oi.quantity * oi.price_at_purchase) AS revenue "
                   "FROM order_items oi "
                   "JOIN products p ON oi.product_id = p.product_id "
                   "GROUP BY p.product_id, p.product_name "
                   "ORDER BY units_sold DESC, revenue DESC "
                   "LIMIT 10");

    if (!stmt.execute())
    {
        cout << "Error: " << stmt.errorMessage() << endl;
        return;
    }

    cout << "\nTop Selling Products";
    util::printTable({"Product", "Units sold", "Revenue"}, stmt.fetchAll());
}
