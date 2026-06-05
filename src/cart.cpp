#include "cart.h"
#include <iostream>
#include <string>
using namespace std;

Cart::Cart(MYSQL *connection)
{
    conn = connection;
}

void Cart::addToCart()
{
    int userId, productId, quantity;

    cout << "Enter user ID: ";
    cin >> userId;
    cout << "Enter product ID: ";
    cin >> productId;
    cout << "Enter quantity: ";
    cin >> quantity;

    string query =
        "INSERT INTO cart_items (user_id, product_id, quantity) VALUES (" +
        to_string(userId) + ", " + to_string(productId) + ", " + to_string(quantity) + ") "
                                                                                       "ON DUPLICATE KEY UPDATE quantity = quantity + " +
        to_string(quantity);

    if (mysql_query(conn, query.c_str()) == 0)
        cout << "Item added to cart.\n";
    else
        cout << "Error: " << mysql_error(conn) << endl;
}

void Cart::viewCart()
{
    int userId;

    cout << "Enter user ID: ";
    cin >> userId;

    string query =
        "SELECT c.cart_id, p.product_name, c.quantity, p.price, "
        "(c.quantity * p.price) AS subtotal "
        "FROM cart_items c "
        "JOIN products p ON c.product_id = p.product_id "
        "WHERE c.user_id = " +
        to_string(userId);

    if (mysql_query(conn, query.c_str()) != 0)
    {
        cout << "Error: " << mysql_error(conn) << endl;
        return;
    }

    MYSQL_RES *res = mysql_store_result(conn);
    MYSQL_ROW row;

    cout << "\nCart ID | Product | Qty | Price | Subtotal\n";
    cout << "------------------------------------------\n";

    while ((row = mysql_fetch_row(res)))
    {
        cout << row[0] << " | " << row[1] << " | " << row[2] << " | " << row[3] << " | " << row[4] << endl;
    }

    mysql_free_result(res);
}

void Cart::removeFromCart()
{
    int cartId;

    cout << "Enter cart item ID to remove: ";
    cin >> cartId;

    string query = "DELETE FROM cart_items WHERE cart_id = " + to_string(cartId);

    if (mysql_query(conn, query.c_str()) == 0)
        cout << "Item removed from cart.\n";
    else
        cout << "Error: " << mysql_error(conn) << endl;
}