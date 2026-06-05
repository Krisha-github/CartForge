#include "order.h"
#include <iostream>
#include <string>
#include <vector>
using namespace std;

struct CartOrderItem
{
    int productId;
    int quantity;
    int stock;
    double price;
};

OrderManager::OrderManager(MYSQL *connection)
{
    conn = connection;
}

void OrderManager::placeOrder()
{
    int userId;
    cout << "Enter user ID: ";
    cin >> userId;

    if (mysql_query(conn, "START TRANSACTION") != 0)
    {
        cout << "Could not start transaction: " << mysql_error(conn) << endl;
        return;
    }

    string cartQuery =
        "SELECT c.product_id, c.quantity, p.stock_quantity, p.price "
        "FROM cart_items c "
        "JOIN products p ON c.product_id = p.product_id "
        "WHERE c.user_id = " +
        to_string(userId) + " FOR UPDATE";

    if (mysql_query(conn, cartQuery.c_str()) != 0)
    {
        cout << "Error checking cart: " << mysql_error(conn) << endl;
        mysql_query(conn, "ROLLBACK");
        return;
    }

    MYSQL_RES *res = mysql_store_result(conn);
    MYSQL_ROW row;

    vector<CartOrderItem> items;
    double totalAmount = 0;

    while ((row = mysql_fetch_row(res)))
    {
        CartOrderItem item;
        item.productId = stoi(row[0]);
        item.quantity = stoi(row[1]);
        item.stock = stoi(row[2]);
        item.price = stod(row[3]);

        if (item.stock < item.quantity)
        {
            cout << "Insufficient stock for product ID " << item.productId << endl;
            mysql_free_result(res);
            mysql_query(conn, "ROLLBACK");
            cout << "Transaction rolled back.\n";
            return;
        }

        totalAmount += item.quantity * item.price;
        items.push_back(item);
    }

    mysql_free_result(res);

    if (items.empty())
    {
        cout << "Cart is empty. Cannot place order.\n";
        mysql_query(conn, "ROLLBACK");
        return;
    }

    string orderQuery =
        "INSERT INTO orders (user_id, total_amount) VALUES (" +
        to_string(userId) + ", " + to_string(totalAmount) + ")";

    if (mysql_query(conn, orderQuery.c_str()) != 0)
    {
        cout << "Error creating order: " << mysql_error(conn) << endl;
        mysql_query(conn, "ROLLBACK");
        return;
    }

    int orderId = (int)mysql_insert_id(conn);

    for (auto item : items)
    {
        string itemQuery =
            "INSERT INTO order_items (order_id, product_id, quantity, price_at_purchase) VALUES (" +
            to_string(orderId) + ", " +
            to_string(item.productId) + ", " +
            to_string(item.quantity) + ", " +
            to_string(item.price) + ")";

        if (mysql_query(conn, itemQuery.c_str()) != 0)
        {
            cout << "Error inserting order item: " << mysql_error(conn) << endl;
            mysql_query(conn, "ROLLBACK");
            return;
        }

        string stockQuery =
            "UPDATE products SET stock_quantity = stock_quantity - " +
            to_string(item.quantity) +
            " WHERE product_id = " + to_string(item.productId);

        if (mysql_query(conn, stockQuery.c_str()) != 0)
        {
            cout << "Error updating stock: " << mysql_error(conn) << endl;
            mysql_query(conn, "ROLLBACK");
            return;
        }
    }

    string clearCartQuery = "DELETE FROM cart_items WHERE user_id = " + to_string(userId);

    if (mysql_query(conn, clearCartQuery.c_str()) != 0)
    {
        cout << "Error clearing cart: " << mysql_error(conn) << endl;
        mysql_query(conn, "ROLLBACK");
        return;
    }

    if (mysql_query(conn, "COMMIT") != 0)
    {
        cout << "Commit failed: " << mysql_error(conn) << endl;
        mysql_query(conn, "ROLLBACK");
        return;
    }

    cout << "Order placed successfully. Order ID: " << orderId << endl;
    cout << "Total amount: " << totalAmount << endl;
}

void OrderManager::viewOrderHistory()
{
    int userId;
    cout << "Enter user ID: ";
    cin >> userId;

    string query =
        "SELECT o.order_id, o.order_date, p.product_name, oi.quantity, oi.price_at_purchase "
        "FROM orders o "
        "JOIN order_items oi ON o.order_id = oi.order_id "
        "JOIN products p ON oi.product_id = p.product_id "
        "WHERE o.user_id = " +
        to_string(userId) +
        " ORDER BY o.order_date DESC";

    if (mysql_query(conn, query.c_str()) != 0)
    {
        cout << "Error: " << mysql_error(conn) << endl;
        return;
    }

    MYSQL_RES *res = mysql_store_result(conn);
    MYSQL_ROW row;

    cout << "\nOrder ID | Date | Product | Qty | Price\n";
    cout << "----------------------------------------\n";

    while ((row = mysql_fetch_row(res)))
    {
        cout << row[0] << " | " << row[1] << " | " << row[2] << " | " << row[3] << " | " << row[4] << endl;
    }

    mysql_free_result(res);
}

void OrderManager::lowStockReport()
{
    int threshold;
    cout << "Enter low stock threshold: ";
    cin >> threshold;

    string query =
        "SELECT product_id, product_name, stock_quantity "
        "FROM products WHERE stock_quantity < " +
        to_string(threshold);

    if (mysql_query(conn, query.c_str()) != 0)
    {
        cout << "Error: " << mysql_error(conn) << endl;
        return;
    }

    MYSQL_RES *res = mysql_store_result(conn);
    MYSQL_ROW row;

    cout << "\nLow Stock Products\n";
    cout << "------------------\n";

    while ((row = mysql_fetch_row(res)))
    {
        cout << row[0] << " | " << row[1] << " | Stock: " << row[2] << endl;
    }

    mysql_free_result(res);
}

void OrderManager::totalSalesReport()
{
    string query = "SELECT IFNULL(SUM(total_amount), 0) FROM orders";

    if (mysql_query(conn, query.c_str()) != 0)
    {
        cout << "Error: " << mysql_error(conn) << endl;
        return;
    }

    MYSQL_RES *res = mysql_store_result(conn);
    MYSQL_ROW row = mysql_fetch_row(res);

    cout << "Total sales: " << row[0] << endl;

    mysql_free_result(res);
}

void OrderManager::topSellingProducts()
{
    string query =
        "SELECT p.product_name, SUM(oi.quantity) AS total_sold "
        "FROM order_items oi "
        "JOIN products p ON oi.product_id = p.product_id "
        "GROUP BY p.product_id, p.product_name "
        "ORDER BY total_sold DESC";

    if (mysql_query(conn, query.c_str()) != 0)
    {
        cout << "Error: " << mysql_error(conn) << endl;
        return;
    }

    MYSQL_RES *res = mysql_store_result(conn);
    MYSQL_ROW row;

    cout << "\nTop Selling Products\n";
    cout << "--------------------\n";

    while ((row = mysql_fetch_row(res)))
    {
        cout << row[0] << " | Sold: " << row[1] << endl;
    }

    mysql_free_result(res);
}