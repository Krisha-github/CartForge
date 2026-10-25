#include "product.h"
#include "db_connection.h"
#include "utils.h"

#include <climits>
#include <iostream>
#include <string>

using namespace std;

Product::Product(MYSQL *connection)
{
    conn = connection;
}

void Product::addProduct()
{
    string name = util::readText("Enter product name: ", 100);
    string price = util::readMoney("Enter price: ");
    int stock = util::readInt("Enter stock quantity: ", 0, 1000000);

    Statement stmt(conn,
                   "INSERT INTO products (product_name, price, stock_quantity) VALUES (?, ?, ?)");

    if (stmt.execute({name, price, to_string(stock)}))
        cout << "Product added successfully (ID " << stmt.insertId() << ").\n";
    else
        cout << "Error: " << stmt.errorMessage() << endl;
}

void Product::viewProducts()
{
    Statement stmt(conn,
                   "SELECT product_id, product_name, price, stock_quantity "
                   "FROM products WHERE is_active = 1 ORDER BY product_id");

    if (!stmt.execute())
    {
        cout << "Error: " << stmt.errorMessage() << endl;
        return;
    }

    util::printTable({"ID", "Product", "Price", "Stock"}, stmt.fetchAll());
}

void Product::searchProduct()
{
    string keyword = util::readText("Enter product keyword: ", 100);

    Statement stmt(conn,
                   "SELECT product_id, product_name, price, stock_quantity "
                   "FROM products "
                   "WHERE is_active = 1 AND product_name LIKE CONCAT('%', ?, '%') ESCAPE '|' "
                   "ORDER BY product_id");

    if (!stmt.execute({util::escapeLike(keyword)}))
    {
        cout << "Error: " << stmt.errorMessage() << endl;
        return;
    }

    util::printTable({"ID", "Product", "Price", "Stock"}, stmt.fetchAll());
}

void Product::updateStock()
{
    int productId = util::readInt("Enter product ID: ", 1, INT_MAX);
    int stock = util::readInt("Enter new stock quantity: ", 0, 1000000);

    Statement stmt(conn,
                   "UPDATE products SET stock_quantity = ? "
                   "WHERE product_id = ? AND is_active = 1");

    if (!stmt.execute({to_string(stock), to_string(productId)}))
        cout << "Error: " << stmt.errorMessage() << endl;
    else if (stmt.affectedRows() == 0)
        cout << "No active product with ID " << productId << ".\n";
    else
        cout << "Stock updated successfully.\n";
}

void Product::deleteProduct()
{
    int productId = util::readInt("Enter product ID to delete: ", 1, INT_MAX);
    string id = to_string(productId);

    if (!rowExists(conn, "SELECT 1 FROM products WHERE product_id = ? AND is_active = 1", {id}))
    {
        cout << "No active product with ID " << productId << ".\n";
        return;
    }

    // Try a real delete first. If the product was ever ordered, the foreign key
    // (ON DELETE RESTRICT on order_items) blocks it - then we archive instead,
    // so past orders and sales reports are never damaged.
    Statement del(conn, "DELETE FROM products WHERE product_id = ?");
    if (del.execute({id}))
    {
        cout << "Product deleted successfully.\n";
        return;
    }

    if (del.errorCode() != dberr::ROW_IS_REFERENCED)
    {
        cout << "Error: " << del.errorMessage() << endl;
        return;
    }

    if (mysql_query(conn, "START TRANSACTION") != 0)
    {
        cout << "Error: " << mysql_error(conn) << endl;
        return;
    }

    Statement clearCarts(conn, "DELETE FROM cart_items WHERE product_id = ?");
    Statement archive(conn, "UPDATE products SET is_active = 0 WHERE product_id = ?");

    if (clearCarts.execute({id}) && archive.execute({id}) && mysql_query(conn, "COMMIT") == 0)
    {
        cout << "Product has order history, so it was archived instead of deleted.\n"
                "It no longer appears in listings or carts; past orders are unchanged.\n";
    }
    else
    {
        mysql_query(conn, "ROLLBACK");
        cout << "Error archiving product.\n";
    }
}
